#import <AppKit/AppKit.h>
#import <objc/message.h>
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

#include "../src/ns_internal.h"
#include "../src/syn_shims.h"
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
  /* -class, not object_getClassName: AppKit observes its own windows, and the
   * isa of a KVO'd object is a generated NSKVONotifying_ subclass that -class
   * deliberately hides. The suites want the class the API is about. */
  return class_getName([(__bridge id)(void *)handle class]);
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

/* ---- plain objects and checked shims for the kernel suite (U4) ---- */

/* Weak, so a C test can see the object go without holding it alive. Reading a
 * weak variable autoreleases the object it yields, so every read below sits in
 * a pool of its own - otherwise the read itself would keep the object past the
 * release under test. */
static __weak NSView *syn_last_view;

bool syn_test_is_main_thread(void) {
  return [NSThread isMainThread];
}

const void *syn_test_view_create(void) {
  @autoreleasepool {
    NSView *view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 10, 10)];
    syn_last_view = view;
    return CFBridgingRetain(view);
  }
}

const void *syn_test_button_create(void) {
  @autoreleasepool {
    return CFBridgingRetain(
        [[NSButton alloc] initWithFrame:NSMakeRect(0, 0, 10, 10)]);
  }
}

const void *syn_test_date_create(void) {
  @autoreleasepool {
    return CFBridgingRetain([NSDate date]);
  }
}

const void *syn_test_string_create(const char *utf8) {
  @autoreleasepool {
    return CFBridgingRetain([NSString stringWithUTF8String:utf8]);
  }
}

bool syn_test_view_is_gone(void) {
  @autoreleasepool {
    return syn_last_view == nil;
  }
}

void syn_test_expect_view(const void *handle) {
  NS_ENTER();
  (void)NS_IN(NSView, handle);
  NS_LEAVE();
}

void syn_test_expect_view_optional(const void *handle) {
  NS_ENTER();
  (void)NS_IN_OPT(NSView, handle);
  NS_LEAVE();
}

bool syn_test_string_in_is_nil(const char *utf8) {
  NS_ENTER();
  return NS_STRING_IN(utf8) == nil;
  NS_LEAVE();
}

bool syn_test_string_in_optional_is_nil(const char *utf8) {
  NS_ENTER();
  return NS_STRING_IN_OPT(utf8) == nil;
  NS_LEAVE();
}

char *syn_test_string_copy(const void *handle) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSString, handle));
  NS_LEAVE();
}

void syn_test_wrapper_raises(void) {
  NS_ENTER();
  syn_test_raise_in_wrapper();
  NS_LEAVE();
}

/* ---- the callback machinery (U14) ---- */

void syn_test_install_action(const void *handle, ns_action action,
                             void *context) {
  @autoreleasepool {
    NSControl *control = (__bridge NSControl *)(void *)handle;
    syn_install_action(control, action, context, ^(id target, SEL selector) {
      control.target = target;
      control.action = selector;
    });
  }
}

void syn_test_fire_action(const void *handle) {
  @autoreleasepool {
    NSControl *control = (__bridge NSControl *)(void *)handle;
    /* sendAction:to: rather than performClick:, so an off-screen control fires
     * synchronously and an uninstalled action is simply a no-op. */
    [control sendAction:control.action to:control.target];
  }
}

static id syn_test_delegate(const void *handle) {
  id object = (__bridge id)(void *)handle;
  /* NSApplication and NSWindow both answer -delegate; going through the
   * runtime says so without pretending the handle is one of the two. */
  return ((id (*)(id, SEL))objc_msgSend)(object, @selector(delegate));
}

bool syn_test_has_delegate(const void *handle) {
  @autoreleasepool {
    return syn_test_delegate(handle) != nil;
  }
}

bool syn_test_delegate_responds(const void *handle, const char *selector) {
  @autoreleasepool {
    id delegate = syn_test_delegate(handle);
    return delegate != nil &&
           [delegate respondsToSelector:sel_registerName(selector)];
  }
}

long syn_test_shim_live_count(void) {
#ifndef NDEBUG
  return syn_shim_live_count();
#else
  return -1; /* no counter in a release build; SYN_ASSERT_SHIMS steps aside */
#endif
}

/* A stand-in protocol. No protocol U14 wraps has a required member or a member
 * that returns anything, and six later units have both, so the suites drive
 * those halves of the machinery through this table. */
typedef struct syn_test_shim_callbacks {
  long (*number_of_rows)(void);
  const void *(*cell_view)(void);
} syn_test_shim_callbacks;

SYN_SHIM_TABLE(syn_test_shim_table, syn_test_shim_callbacks, "NSTestDelegate",
               SYN_SHIM_REQUIRED(syn_test_shim_callbacks, number_of_rows,
                                 "numberOfRows"),
               SYN_SHIM_OPTIONAL(syn_test_shim_callbacks, cell_view,
                                 "cellView"));

static long syn_test_no_rows(void) {
  return 0;
}

void syn_test_install_required_member_struct(const void *handle,
                                             bool set_required) {
  @autoreleasepool {
    syn_test_shim_callbacks callbacks = {0};
    if (set_required) callbacks.number_of_rows = syn_test_no_rows;
    id object = (__bridge id)(void *)handle;
    /* The assign block is a no-op: nothing dispatches to this stand-in, and
     * what is under test happens before the block runs. */
    SYN_SHIM_INSTALL(object, SynShim, syn_test_shim_table, &callbacks, NULL,
                     ^(id shim) { (void)shim; });
  }
}

void syn_test_shim_check_count(long count) {
  SYN_SHIM_CHECK_COUNT(syn_test_shim_table, number_of_rows, count);
}

void syn_test_shim_check_returned_handle(const void *handle) {
  SYN_SHIM_CHECK_HANDLE(syn_test_shim_table, cell_view, handle, NSView, true);
}

void syn_test_post_notification(const char *name, const void *object) {
  @autoreleasepool {
    /* AppKit registers a delegate for its own notifications when the slot is
     * assigned, and only for the ones respondsToSelector: claims, so posting
     * one is how a suite sees that cache from the outside (KTD8). */
    [NSNotificationCenter.defaultCenter
        postNotificationName:[NSString stringWithUTF8String:name]
                      object:(__bridge id)(void *)object];
  }
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
