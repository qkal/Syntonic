#import <AppKit/AppKit.h>
#import <objc/runtime.h>

#include <crt_externs.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "support.h"

/* A hung abort case fails the suite instead of hanging CTest. */
enum { SYN_ABORT_CASE_TIMEOUT_MS = 20000 };

/* One pool around the whole suite; the run loop pushes its own per iteration.
 * `main` lives here, not in runner.c, because runner.c is plain C and cannot
 * open an autorelease pool. */
int main(int argc, char **argv) {
  @autoreleasepool {
    return syn_test_main(argc, argv);
  }
}

void syn_test_bootstrap(void) {
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    NSApplication *app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyProhibited];
    [app finishLaunching];
  });
}

const void *syn_test_shared_application(void) {
  return (__bridge const void *)NSApp;
}

bool syn_test_activation_policy_is_prohibited(void) {
  return [NSApp activationPolicy] == NSApplicationActivationPolicyProhibited;
}

const char *syn_test_class_name(const void *handle) {
  if (handle == NULL) return NULL;
  return object_getClassName((__bridge id)(void *)handle);
}

double syn_test_now_ms(void) {
  return (double)clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW) / 1.0e6;
}

void syn_test_spin(double milliseconds) {
  NSDate *deadline = [NSDate dateWithTimeIntervalSinceNow:milliseconds / 1000.0];
  while ([deadline timeIntervalSinceNow] > 0) {
    @autoreleasepool {
      /* runMode: returns NO at once when nothing is attached to the loop, so
       * without the sleep this would spin the CPU instead of waiting. */
      if (![[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                                    beforeDate:deadline]) {
        [NSThread sleepForTimeInterval:0.001];
      }
    }
  }
}

void syn_test_schedule_flag(double delay_ms, bool *flag) {
  [NSTimer scheduledTimerWithTimeInterval:delay_ms / 1000.0
                                  repeats:NO
                                    block:^(NSTimer *timer) {
                                      (void)timer;
                                      *flag = true;
                                    }];
}

void syn_test_raise_in_wrapper(void) {
  syn_test_bootstrap();
  NSView *view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 10, 10)];
  [view addSubview:view]; /* NSInvalidArgumentException: can't add self */
}

static char *syn_copy_string(const char *text) {
  size_t size = strlen(text) + 1;
  char *copy = malloc(size);
  if (copy != NULL) memcpy(copy, text, size);
  return copy;
}

static bool syn_executable_path(char *out, size_t size) {
  char raw[PATH_MAX];
  uint32_t length = (uint32_t)sizeof raw;
  if (_NSGetExecutablePath(raw, &length) != 0) return false;
  char resolved[PATH_MAX];
  if (realpath(raw, resolved) == NULL) return false;
  if (strlcpy(out, resolved, size) >= size) return false;
  return true;
}

/* Reads the pipe to EOF or until the deadline passes, killing the child on
 * timeout. Returns a malloc'd NUL-terminated buffer. */
static char *syn_drain(int fd, pid_t pid, double deadline_ms, bool *timed_out) {
  size_t capacity = 4096;
  size_t length = 0;
  char *buffer = malloc(capacity);
  if (buffer == NULL) return NULL;

  for (;;) {
    double remaining = deadline_ms - syn_test_now_ms();
    if (remaining <= 0) {
      *timed_out = true;
      kill(pid, SIGKILL);
      break;
    }
    struct pollfd entry = {.fd = fd, .events = POLLIN, .revents = 0};
    int ready = poll(&entry, 1, (int)remaining);
    if (ready < 0) {
      if (errno == EINTR) continue;
      break;
    }
    if (ready == 0) continue;
    if (capacity - length < 1024) {
      char *grown = realloc(buffer, capacity * 2);
      if (grown == NULL) break;
      buffer = grown;
      capacity *= 2;
    }
    ssize_t got = read(fd, buffer + length, capacity - length - 1);
    if (got < 0) {
      if (errno == EINTR) continue;
      break;
    }
    if (got == 0) break; /* EOF: the child is gone */
    length += (size_t)got;
  }

  buffer[length] = '\0';
  return buffer;
}

syn_test_abort_result syn_test_expect_abort(const char *case_name) {
  syn_test_abort_result result = {0};

  char executable[PATH_MAX];
  if (!syn_executable_path(executable, sizeof executable)) {
    result.stderr_text = syn_copy_string("could not resolve the suite binary");
    return result;
  }

  int pipe_fds[2];
  if (pipe(pipe_fds) != 0) {
    result.stderr_text = syn_copy_string("pipe() failed");
    return result;
  }

  posix_spawn_file_actions_t actions;
  posix_spawn_file_actions_init(&actions);
  posix_spawn_file_actions_adddup2(&actions, pipe_fds[1], STDERR_FILENO);
  posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, "/dev/null",
                                   O_WRONLY, 0);
  posix_spawn_file_actions_addclose(&actions, pipe_fds[0]);
  posix_spawn_file_actions_addclose(&actions, pipe_fds[1]);

  char *argv[] = {executable, (char *)SYN_ABORT_CASE_FLAG, (char *)case_name,
                  NULL};
  pid_t pid = -1;
  int spawned = posix_spawn(&pid, executable, &actions, NULL, argv,
                            *_NSGetEnviron());
  posix_spawn_file_actions_destroy(&actions);
  close(pipe_fds[1]);

  if (spawned != 0) {
    close(pipe_fds[0]);
    result.stderr_text = syn_copy_string(strerror(spawned));
    return result;
  }

  result.stderr_text = syn_drain(pipe_fds[0], pid,
                                 syn_test_now_ms() + SYN_ABORT_CASE_TIMEOUT_MS,
                                 &result.timed_out);
  close(pipe_fds[0]);
  if (result.stderr_text == NULL) result.stderr_text = syn_copy_string("");

  int status = 0;
  while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
  }
  if (WIFSIGNALED(status)) {
    result.signal = WTERMSIG(status);
    result.aborted = !result.timed_out && result.signal == SIGABRT;
  } else if (WIFEXITED(status)) {
    result.exit_status = WEXITSTATUS(status);
  }
  return result;
}

void syn_test_abort_result_free(syn_test_abort_result *result) {
  if (result == NULL) return;
  free(result->stderr_text);
  result->stderr_text = NULL;
}

char *syn_test_check_abort(const char *case_name, const char *needle) {
  syn_test_abort_result result = syn_test_expect_abort(case_name);
  char *why = NULL;

  if (!result.aborted ||
      (needle != NULL && strstr(result.stderr_text, needle) == NULL)) {
    char outcome[64];
    if (result.timed_out) {
      snprintf(outcome, sizeof outcome, "a timeout after %d ms",
               SYN_ABORT_CASE_TIMEOUT_MS);
    } else if (result.signal != 0) {
      snprintf(outcome, sizeof outcome, "signal %d", result.signal);
    } else {
      snprintf(outcome, sizeof outcome, "exit status %d", result.exit_status);
    }

    const char *format =
        "abort case \"%s\": expected SIGABRT with \"%s\" on stderr, got %s\n"
        "--- child stderr ---\n%s--- end of child stderr ---";
    size_t size = strlen(format) + strlen(case_name) +
                  strlen(needle != NULL ? needle : "") + sizeof outcome +
                  strlen(result.stderr_text) + 1;
    why = malloc(size);
    if (why != NULL) {
      snprintf(why, size, format, case_name, needle != NULL ? needle : "",
               outcome, result.stderr_text);
    }
  }

  syn_test_abort_result_free(&result);
  return why;
}
