/*
 * axdump - print the accessibility role and label tree of a running app's
 * front window (R23).
 *
 * `just ax-compare` runs this against both twins and diffs the two trees: the
 * C twin is accepted only when its roles and labels are the Swift twin's.
 *
 *     axdump <pid>
 *
 * This is a tool, not a wrapper. It talks to ApplicationServices directly,
 * because reading another process's accessibility tree is not something
 * Syntonic exposes and not something an app does to itself.
 *
 * IT NEEDS THE ACCESSIBILITY GRANT. The accessibility API is readable only
 * from a process the user has trusted in System Settings - Privacy & Security
 * - Accessibility, and that is true even for a process reading its own tree.
 * Without it this exits with SYNTONIC_AX_NOT_TRUSTED rather than printing an
 * empty tree that would diff clean against another empty tree.
 */

#include <ApplicationServices/ApplicationServices.h>

#import <Foundation/Foundation.h>

#include <stdio.h>
#include <stdlib.h>

enum { AXDUMP_MAX_DEPTH = 32 };

static NSString *axdump_copy_string(AXUIElementRef element,
                                    CFStringRef attribute) {
  CFTypeRef value = NULL;
  if (AXUIElementCopyAttributeValue(element, attribute, &value) !=
          kAXErrorSuccess ||
      value == NULL) {
    return nil;
  }
  NSString *string = nil;
  if (CFGetTypeID(value) == CFStringGetTypeID()) {
    string = [NSString stringWithString:(__bridge NSString *)value];
  }
  CFRelease(value);
  return string.length > 0 ? string : nil;
}

/* What a screen reader announces for the element: its title, or its
 * description when it has no title, or its text when it is a field. */
static NSString *axdump_label(AXUIElementRef element) {
  NSString *label = axdump_copy_string(element, kAXTitleAttribute);
  if (label == nil) {
    label = axdump_copy_string(element, kAXDescriptionAttribute);
  }
  if (label == nil) {
    label = axdump_copy_string(element, kAXValueAttribute);
  }
  return label == nil ? @"" : label;
}

static NSArray *axdump_children(AXUIElementRef element) {
  CFTypeRef value = NULL;
  if (AXUIElementCopyAttributeValue(element, kAXChildrenAttribute, &value) !=
          kAXErrorSuccess ||
      value == NULL) {
    return @[];
  }
  NSArray *children = @[];
  if (CFGetTypeID(value) == CFArrayGetTypeID()) {
    children = [NSArray arrayWithArray:(__bridge NSArray *)value];
  }
  CFRelease(value);
  return children;
}

static void axdump_walk(AXUIElementRef element, int depth) {
  if (depth > AXDUMP_MAX_DEPTH) {
    return;
  }
  NSString *role = axdump_copy_string(element, kAXRoleAttribute);
  printf("%*srole=%s label=%s\n", depth * 2, "",
         role == nil ? "" : role.UTF8String, axdump_label(element).UTF8String);
  for (id child in axdump_children(element)) {
    axdump_walk((__bridge AXUIElementRef)child, depth + 1);
  }
}

/* The app's main window, or its first window when it names no main one. */
static AXUIElementRef axdump_copy_front_window(AXUIElementRef application) {
  CFTypeRef window = NULL;
  if (AXUIElementCopyAttributeValue(application, kAXMainWindowAttribute,
                                    &window) == kAXErrorSuccess &&
      window != NULL) {
    return (AXUIElementRef)window;
  }
  CFTypeRef windows = NULL;
  if (AXUIElementCopyAttributeValue(application, kAXWindowsAttribute,
                                    &windows) != kAXErrorSuccess ||
      windows == NULL) {
    return NULL;
  }
  AXUIElementRef first = NULL;
  if (CFGetTypeID(windows) == CFArrayGetTypeID() &&
      CFArrayGetCount((CFArrayRef)windows) > 0) {
    first = (AXUIElementRef)CFRetain(
        CFArrayGetValueAtIndex((CFArrayRef)windows, 0));
  }
  CFRelease(windows);
  return first;
}

int main(int argc, const char *argv[]) {
  @autoreleasepool {
    if (argc != 2) {
      fprintf(stderr, "usage: axdump <pid>\n");
      return 2;
    }
    if (!AXIsProcessTrusted()) {
      fprintf(stderr,
              "axdump: SYNTONIC_AX_NOT_TRUSTED: this process may not read the "
              "accessibility API.\n"
              "  Grant Accessibility to this shell's host app in\n"
              "  System Settings - Privacy & Security - Accessibility, then "
              "run this again.\n");
      return 1;
    }

    pid_t pid = (pid_t)atoi(argv[1]);
    if (pid <= 0) {
      fprintf(stderr, "axdump: '%s' is not a pid.\n", argv[1]);
      return 2;
    }

    AXUIElementRef application = AXUIElementCreateApplication(pid);
    AXUIElementRef window = axdump_copy_front_window(application);
    if (window == NULL) {
      fprintf(stderr,
              "axdump: SYNTONIC_AX_NO_WINDOW: process %d has no window the "
              "accessibility API can see.\n",
              pid);
      CFRelease(application);
      return 1;
    }
    axdump_walk(window, 0);
    CFRelease(window);
    CFRelease(application);
  }
  return 0;
}
