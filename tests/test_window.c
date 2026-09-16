/*
 * The window suite (U14): the full-form constructor, title, minimum size, the
 * content view controller, close, and the window delegate struct - including
 * F3, where closing only orders the window out and the last release tears the
 * tree down (R7, R14, F3, KTD7, KTD8).
 */

#include <objc/message.h>
#include <objc/runtime.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "runner.h"
#include "support.h"
#include "syntonic/syntonic.h"

enum { SYN_STANDARD_MASK = NS_WINDOW_STYLE_MASK_TITLED |
                           NS_WINDOW_STYLE_MASK_CLOSABLE |
                           NS_WINDOW_STYLE_MASK_MINIATURIZABLE |
                           NS_WINDOW_STYLE_MASK_RESIZABLE };

static ns_window *syn_window_create(void) {
  return ns_window_create_with_content_rect_style_mask_backing_defer(
      CGRectMake(0, 0, 400, 300), (ns_window_style_mask)SYN_STANDARD_MASK,
      NS_BACKING_STORE_BUFFERED, false);
}

static int syn_closes;
static int syn_late_closes;
static int syn_reentries;
static void *syn_close_context;
static const void *syn_close_sender;
static char syn_sender_class[64];

static void syn_reset(void) {
  syn_closes = syn_late_closes = syn_reentries = 0;
  syn_close_context = NULL;
  syn_close_sender = NULL;
  syn_sender_class[0] = '\0';
}

static void syn_on_will_close(void *context, ns_window *sender) {
  syn_closes++;
  syn_close_context = context;
  syn_close_sender = sender;
}

static void syn_on_late_close(void *context, ns_window *sender) {
  (void)sender;
  syn_late_closes++;
  syn_close_context = context;
}

/* ---- the constructor, the title and the minimum size (R7, R14) ---- */

SYN_TEST(a_window_reports_its_title_through_an_owned_copy) {
  syn_test_bootstrap();
  ns_window *window = syn_window_create();
  SYN_ASSERT_STR_EQ(syn_test_class_name(window), "NSWindow");

  ns_window_set_title(window, "Syntonic Twin");
  char *first = ns_window_copy_title(window);
  char *second = ns_window_copy_title(window);
  SYN_ASSERT_STR_EQ(first, "Syntonic Twin");
  SYN_ASSERT_MSG(first != second, "two copies of the title came back as one");

  ns_string_free(first);
  SYN_ASSERT_STR_EQ(second, "Syntonic Twin"); /* the other copy is untouched */
  ns_string_free(second);

  ns_release(window);
}

SYN_TEST(a_window_keeps_the_minimum_content_size_it_was_given) {
  syn_test_bootstrap();
  ns_window *window = syn_window_create();

  ns_window_set_content_min_size(window, CGSizeMake(320, 240));
  CGSize minimum = ns_window_content_min_size(window);
  SYN_ASSERT_MSG(minimum.width == 320 && minimum.height == 240,
                 "the minimum content size read back as %gx%g", minimum.width,
                 minimum.height);

  ns_window_set_content_size(window, CGSizeMake(500, 400));
  CGRect frame = ns_window_frame(window);
  SYN_ASSERT_MSG(frame.size.width == 500,
                 "the requested content width was not taken: %g",
                 frame.size.width);
  SYN_ASSERT_MSG(frame.size.height > 400,
                 "the frame is not the content plus its title bar: height %g",
                 frame.size.height);

  /* AppKit applies contentMinSize to a resize the user drives, not to a size
   * the program asks for; ns_window.h says so where a caller will read it.
   * Pinning it here is what stops a later unit from "fixing" the wrapper into
   * a clamp AppKit does not have. */
  ns_window_set_content_size(window, CGSizeMake(100, 100));
  SYN_ASSERT_MSG(ns_window_frame(window).size.width == 100,
                 "a programmatic size was clamped, which AppKit does not do");
  minimum = ns_window_content_min_size(window);
  SYN_ASSERT_MSG(minimum.width == 320 && minimum.height == 240,
                 "the minimum content size did not survive a resize");

  ns_release(window);
}

/* The window's own release, with nothing else in the picture, is what the leak
 * gate reads (F3). */
SYN_TEST(a_window_that_was_never_shown_releases_clean) {
  syn_test_bootstrap();
  ns_window *window = syn_window_create();
  ns_window_set_title(window, "never shown");
  ns_release(window);
}

/* ---- the delegate struct (R9, KTD8) ---- */

SYN_TEST(closing_a_window_fires_will_close_with_the_window_and_the_context) {
  syn_test_bootstrap();
  syn_reset();
  int context = 17;

  ns_window *window = syn_window_create();
  ns_window_callbacks callbacks = {.will_close = syn_on_will_close};
  ns_window_set_callbacks(window, &callbacks, &context);
  SYN_ASSERT_SHIMS(1);

  ns_window_make_key_and_order_front(window);
  syn_test_spin(50);
  ns_window_close(window);

  SYN_WAIT_FOR(syn_closes == 1, 1000);
  SYN_ASSERT_MSG(syn_close_sender == window,
                 "will_close got a sender that is not the window that closed");
  SYN_ASSERT_MSG(syn_close_context == &context, "the context did not arrive");

  ns_release(window);
  SYN_ASSERT_SHIMS(0);
}

SYN_TEST(an_unset_optional_member_is_a_method_the_delegate_does_not_implement) {
  syn_test_bootstrap();
  syn_reset();

  ns_window *window = syn_window_create();
  ns_window_callbacks empty = {0};
  ns_window_set_callbacks(window, &empty, NULL);

  /* KTD8: AppKit caches this answer when the slot is assigned, so the shim has
   * to be right from the moment it exists - which is what the assignment order
   * in syn_shim_install is for. */
  SYN_ASSERT_MSG(syn_test_has_delegate(window),
                 "a struct with every member unset is still an install");
  SYN_ASSERT_MSG(!syn_test_delegate_responds(window, "windowWillClose:"),
                 "an unset optional member is reported as implemented");

  ns_window_close(window);
  syn_test_spin(50);
  SYN_ASSERT_MSG(syn_closes == 0, "an unset member fired");

  ns_release(window);
}

SYN_TEST(a_null_struct_uninstalls_and_releases_the_shim) {
  syn_test_bootstrap();
  syn_reset();

  ns_window *window = syn_window_create();
  ns_window_callbacks callbacks = {.will_close = syn_on_will_close};
  ns_window_set_callbacks(window, &callbacks, NULL);
  SYN_ASSERT_SHIMS(1);

  ns_window_set_callbacks(window, NULL, NULL);
  SYN_ASSERT_MSG(!syn_test_has_delegate(window),
                 "a null struct left a delegate installed");
  SYN_ASSERT_SHIMS(0);

  ns_window_close(window);
  syn_test_spin(50);
  SYN_ASSERT_MSG(syn_closes == 0, "an uninstalled member fired");

  ns_release(window);
}

/* KTD8: the context lives until the struct is uninstalled or the object is
 * deallocated, whichever comes first, and ns_release is neither. */
SYN_TEST(ns_release_on_the_window_does_not_uninstall_the_struct) {
  syn_test_bootstrap();
  syn_reset();
  int context = 5;

  ns_window *window = syn_window_create();
  ns_retain(window); /* caller +2 */
  ns_window_callbacks callbacks = {.will_close = syn_on_will_close};
  ns_window_set_callbacks(window, &callbacks, &context);

  ns_release(window); /* caller +1: not an uninstall */
  SYN_ASSERT_SHIMS(1);
  SYN_ASSERT_MSG(syn_test_delegate_responds(window, "windowWillClose:"),
                 "ns_release uninstalled the struct");

  ns_window_close(window);
  SYN_WAIT_FOR(syn_closes == 1, 1000);
  SYN_ASSERT_MSG(syn_close_context == &context,
                 "the context did not survive an ns_release");

  ns_release(window); /* caller 0: the object goes, the shim with it */
  SYN_ASSERT_SHIMS(0);
}

/* KTD8: a callback may re-install its own struct while the shim's method is
 * still on the stack. The shim holds itself for the call, so releasing it from
 * the association here is not a use after free - which is what the sanitizer
 * gate reads - and the new member set is what fires next. */
static void syn_reinstalling_will_close(void *context, ns_window *sender) {
  syn_closes++;
  ns_window_callbacks next = {.will_close = syn_on_late_close};
  ns_window_set_callbacks(sender, &next, context);
  snprintf(syn_sender_class, sizeof syn_sender_class, "%s",
           syn_test_class_name(sender));
}

SYN_TEST(re_installing_from_inside_a_callback_changes_what_fires_afterwards) {
  syn_test_bootstrap();
  syn_reset();
  int context = 23;

  ns_window *window = syn_window_create();
  ns_window_callbacks callbacks = {.will_close = syn_reinstalling_will_close};
  ns_window_set_callbacks(window, &callbacks, &context);

  ns_window_make_key_and_order_front(window);
  syn_test_spin(50);
  ns_window_close(window);
  SYN_WAIT_FOR(syn_closes == 1, 1000);
  SYN_ASSERT_STR_EQ(syn_sender_class, "NSWindow");
  SYN_ASSERT_SHIMS(1);

  /* The second struct is the one installed now. */
  ns_window_make_key_and_order_front(window);
  syn_test_spin(50);
  ns_window_close(window);
  SYN_WAIT_FOR(syn_late_closes == 1, 1000);
  SYN_ASSERT_MSG(syn_closes == 1, "the replaced member fired again");
  SYN_ASSERT_MSG(syn_close_context == &context, "the context did not carry");

  ns_release(window);
  SYN_ASSERT_SHIMS(0);
}

/* F3 and KTD8: the window is closed from inside its own will_close and the
 * sender is released there too, then the caller's last reference goes. The
 * sender stays readable for the whole callback, close only orders the window
 * out, and releasing the window tears its view tree down. */
static const void *syn_window_to_release;

static void syn_closing_will_close(void *context, ns_window *sender) {
  (void)context;
  syn_closes++;
  if (syn_reentries == 0) {
    syn_reentries++;
    ns_window_close(sender); /* closing it from inside its own will_close */
  }
  if (syn_window_to_release != NULL) {
    /* One of the caller's two references, given back from inside the callback.
     * A nested close fires this member again, so the handle is cleared rather
     * than released twice. */
    ns_release(syn_window_to_release);
    syn_window_to_release = NULL;
  }
  snprintf(syn_sender_class, sizeof syn_sender_class, "%s",
           syn_test_class_name(sender));
}

SYN_TEST(f3_close_orders_out_and_the_last_release_tears_the_tree_down) {
  syn_test_bootstrap();
  syn_reset();

  const void *view = syn_test_view_create();
  ns_view_controller *controller = ns_view_controller_create();
  ns_view_controller_set_view(controller, (ns_view *)(void *)view);

  ns_window *window = syn_window_create();
  ns_window_set_content_view_controller(window, controller);
  ns_release(controller);
  ns_release(view); /* the window's tree is the only holder now */
  SYN_ASSERT_MSG(!syn_test_view_is_gone(),
                 "the window's tree did not hold the view");

  ns_retain(window); /* caller +2; the callback gives one back */
  syn_window_to_release = window;
  ns_window_callbacks callbacks = {.will_close = syn_closing_will_close};
  ns_window_set_callbacks(window, &callbacks, NULL);

  ns_window_make_key_and_order_front(window);
  syn_test_spin(50);
  ns_window_close(window);
  SYN_WAIT_FOR(syn_closes >= 1, 1000);
  SYN_ASSERT_STR_EQ(syn_sender_class, "NSWindow");
  SYN_ASSERT_MSG(!syn_test_view_is_gone(),
                 "close released the window: releasedWhenClosed is not off");

  ns_release(window); /* caller 0 */
  SYN_WAIT_FOR(syn_test_view_is_gone(), 1000);
  SYN_ASSERT_SHIMS(0);
}

/* ---- the case that aborts, run in a child (R10, KTD4) ---- */

static void *syn_create_a_window_off_the_main_thread(void *unused) {
  (void)unused;
  ns_window_create_with_content_rect_style_mask_backing_defer(
      CGRectMake(0, 0, 100, 100), NS_WINDOW_STYLE_MASK_TITLED,
      NS_BACKING_STORE_BUFFERED, false);
  return NULL;
}

SYN_ABORT_CASE(window_created_off_the_main_thread) {
  syn_test_bootstrap();
  pthread_t thread;
  if (pthread_create(&thread, NULL, syn_create_a_window_off_the_main_thread,
                     NULL) != 0) {
    fprintf(stderr, "could not start the background thread\n");
    return;
  }
  pthread_join(thread, NULL);
}

SYN_TEST(creating_a_window_off_the_main_thread_names_the_function) {
  SYN_ASSERT_ABORTS("window_created_off_the_main_thread",
                    "ns_window_create_with_content_rect_style_mask_backing_defer");
  SYN_ASSERT_ABORTS("window_created_off_the_main_thread", "main thread only");
}

/* ---- the geometry and state reads the twin comparison needs (U9) ---- */

/* setFrameOrigin: moves the window without resizing it, and the frame reads
 * the move back. The capture script pins both twins to one origin, which is
 * what makes a pixel comparison meaningful. */
SYN_TEST(setting_the_frame_origin_moves_the_window_and_keeps_its_size) {
  syn_test_bootstrap();
  ns_window *window = syn_window_create();
  ns_window_set_content_size(window, CGSizeMake(400, 300));
  CGRect before = ns_window_frame(window);

  ns_window_set_frame_origin(window, CGPointMake(200, 200));
  CGRect after = ns_window_frame(window);
  SYN_ASSERT_MSG(after.origin.x == 200 && after.origin.y == 200,
                 "the origin reads %g,%g, not 200,200", after.origin.x,
                 after.origin.y);
  SYN_ASSERT_MSG(after.size.width == before.size.width &&
                     after.size.height == before.size.height,
                 "moving the window changed its size");

  ns_release(window);
}

/* isVisible is what tells a capture script the window is up. An application
 * with the prohibited activation policy has visible windows on no screen. */
SYN_TEST(a_window_is_visible_between_ordering_front_and_closing) {
  syn_test_bootstrap();
  ns_window *window = syn_window_create();
  SYN_ASSERT_MSG(!ns_window_visible(window),
                 "a window is visible before anything ordered it front");

  ns_window_make_key_and_order_front(window);
  SYN_ASSERT_MSG(ns_window_visible(window),
                 "the window is not visible after being ordered front");

  ns_window_close(window);
  SYN_ASSERT_MSG(!ns_window_visible(window),
                 "close did not order the window out");
  ns_release(window);
}

/* The window server's id, which is what a window capture targets. It is
 * assigned when the window first goes on screen, so it is read after ordering
 * front, not before. */
SYN_TEST(a_window_that_has_been_ordered_front_reports_a_window_number) {
  syn_test_bootstrap();
  ns_window *window = syn_window_create();
  ns_window_make_key_and_order_front(window);
  syn_test_spin(50);
  SYN_ASSERT_MSG(ns_window_window_number(window) > 0,
                 "the window number reads %ld, so nothing could capture it",
                 ns_window_window_number(window));

  ns_window_close(window);
  ns_release(window);
}

/* State restoration would move and resize a window behind the program's back,
 * which is exactly what a pinned-geometry comparison cannot have. */
SYN_TEST(restorable_reads_back_both_ways) {
  syn_test_bootstrap();
  ns_window *window = syn_window_create();
  SYN_ASSERT_MSG(ns_window_restorable(window),
                 "a window does not start restorable, which AppKit's default "
                 "says it should");

  ns_window_set_restorable(window, false);
  SYN_ASSERT_MSG(!ns_window_restorable(window),
                 "turning restoration off did not read back");
  ns_window_set_restorable(window, true);
  SYN_ASSERT_MSG(ns_window_restorable(window),
                 "turning restoration back on did not read back");

  ns_release(window);
}

/* A window with no toolbar reports none; ns_toolbar_set_callbacks is what the
 * toolbar suite covers, and this is only the reader U9 added here. */
SYN_TEST(a_window_reports_the_toolbar_and_the_toolbar_style_it_was_given) {
  syn_test_bootstrap();
  ns_window *window = syn_window_create();
  SYN_ASSERT_MSG(ns_window_toolbar(window) == NULL,
                 "a fresh window already has a toolbar");
  SYN_ASSERT_MSG(ns_window_get_toolbar_style(window) ==
                     NS_WINDOW_TOOLBAR_STYLE_AUTOMATIC,
                 "a fresh window does not start on the automatic toolbar "
                 "style");

  ns_toolbar *toolbar = ns_toolbar_create_with_identifier(
      "dev.kaino.syntonic.test.window.toolbar");
  ns_window_set_toolbar(window, toolbar);
  ns_window_set_toolbar_style(window, NS_WINDOW_TOOLBAR_STYLE_UNIFIED);
  SYN_ASSERT_MSG(ns_window_toolbar(window) == toolbar,
                 "the window reports a different toolbar than the one set");
  SYN_ASSERT_MSG(ns_window_get_toolbar_style(window) ==
                     NS_WINDOW_TOOLBAR_STYLE_UNIFIED,
                 "the toolbar style did not read back as unified");

  ns_window_set_toolbar(window, NULL);
  SYN_ASSERT_MSG(ns_window_toolbar(window) == NULL,
                 "a null toolbar left one on the window");

  ns_release(toolbar);
  ns_release(window);
}

/* ---- the move and resize members (U12 gaps) ----
 *
 * Moving a window lays nothing out, so a program that reports its own geometry
 * has to be told; resizing is the same event on the other axis. Both are
 * optional members of the same struct as will_close. */

static int syn_moves;
static int syn_resizes;
static void *syn_move_context;
static const void *syn_move_sender;
static const void *syn_resize_sender;

static void syn_on_did_move(void *context, ns_window *sender) {
  syn_moves++;
  syn_move_context = context;
  syn_move_sender = sender;
}

static void syn_on_did_resize(void *context, ns_window *sender) {
  syn_resizes++;
  syn_move_context = context;
  syn_resize_sender = sender;
}

static void syn_reset_geometry(void) {
  syn_moves = syn_resizes = 0;
  syn_move_context = NULL;
  syn_move_sender = syn_resize_sender = NULL;
}

SYN_TEST(moving_and_resizing_fire_their_members_with_the_window_and_context) {
  syn_test_bootstrap();
  syn_reset_geometry();
  int context = 31;

  ns_window *window = syn_window_create();
  ns_window_callbacks callbacks = {.did_move = syn_on_did_move,
                                   .did_resize = syn_on_did_resize};
  ns_window_set_callbacks(window, &callbacks, &context);
  SYN_ASSERT_SHIMS(1);

  ns_window_set_frame_origin(window, CGPointMake(120, 140));
  SYN_WAIT_FOR(syn_moves >= 1, 1000);
  SYN_ASSERT_MSG(syn_move_sender == window,
                 "did_move got a sender that is not the window that moved");
  SYN_ASSERT_MSG(syn_move_context == &context,
                 "did_move did not get the context");
  SYN_ASSERT_MSG(syn_resizes == 0, "moving the window fired did_resize");

  /* The frame the callback reported on is the frame the window has. */
  CGRect moved = ns_window_frame(window);
  SYN_ASSERT_MSG(moved.origin.x == 120 && moved.origin.y == 140,
                 "the window did not keep the origin it was moved to");

  ns_window_set_content_size(window, CGSizeMake(520, 360));
  SYN_WAIT_FOR(syn_resizes >= 1, 1000);
  SYN_ASSERT_MSG(syn_resize_sender == window,
                 "did_resize got a sender that is not the window that "
                 "resized");
  SYN_ASSERT_MSG(syn_move_context == &context,
                 "did_resize did not get the context");

  /* setContentSize: holds the window's top-left corner, so a resize moves the
   * origin too and fires did_move alongside did_resize. */
  CGRect resized = ns_window_frame(window);
  SYN_ASSERT_MSG(resized.size.width == 520,
                 "the window did not take the content width it was given");

  ns_release(window);
  SYN_ASSERT_SHIMS(0);
}

SYN_TEST(unset_move_and_resize_members_are_methods_that_do_not_exist) {
  syn_test_bootstrap();
  syn_reset_geometry();

  ns_window *window = syn_window_create();
  ns_window_callbacks empty = {0};
  ns_window_set_callbacks(window, &empty, NULL);

  SYN_ASSERT_MSG(!syn_test_delegate_responds(window, "windowDidMove:"),
                 "an unset did_move is reported as implemented");
  SYN_ASSERT_MSG(!syn_test_delegate_responds(window, "windowDidResize:"),
                 "an unset did_resize is reported as implemented");

  ns_window_set_frame_origin(window, CGPointMake(60, 60));
  ns_window_set_content_size(window, CGSizeMake(320, 240));
  syn_test_spin(100);
  SYN_ASSERT_MSG(syn_moves == 0, "an unset did_move fired");
  SYN_ASSERT_MSG(syn_resizes == 0, "an unset did_resize fired");

  /* The two that are set answer yes, so the shim reports per member and not
   * per protocol (KTD8). */
  ns_window_callbacks both = {.did_move = syn_on_did_move,
                              .did_resize = syn_on_did_resize};
  ns_window_set_callbacks(window, &both, NULL);
  SYN_ASSERT_MSG(syn_test_delegate_responds(window, "windowDidMove:"),
                 "a set did_move is reported as absent");
  SYN_ASSERT_MSG(syn_test_delegate_responds(window, "windowDidResize:"),
                 "a set did_resize is reported as absent");
  SYN_ASSERT_MSG(!syn_test_delegate_responds(window, "windowWillClose:"),
                 "an unset will_close is reported as implemented");

  ns_window_set_callbacks(window, NULL, NULL);
  ns_release(window);
}

/* ---- makeFirstResponder: (U12 gaps, R19) ----
 *
 * initialFirstResponder is only consulted the first time a window becomes key,
 * so a window controller that shows a window and moves focus in the same
 * breath needs this one. */

/* -[NSWindow firstResponder] has no C function of its own - the twins never
 * read it - so this suite reaches it through the runtime rather than spelling
 * a wrapper nobody asked for, exactly as tests/test_split_view.c does. */
static const void *syn_first_responder(ns_window *window) {
  return (const void *)((id(*)(id, SEL))objc_msgSend)(
      (id)(void *)window, sel_getUid("firstResponder"));
}

SYN_TEST(make_first_responder_moves_focus_and_reports_that_it_did) {
  syn_test_bootstrap();
  ns_window *window = syn_window_create();
  ns_view *content = ns_window_content_view(window);
  SYN_ASSERT_MSG(content != NULL, "the window has no content view");

  ns_view *pane = ns_view_create_with_frame(CGRectMake(10, 10, 200, 100));
  ns_view_add_subview(content, pane);
  SYN_ASSERT_MSG(ns_window_make_first_responder(window, pane),
                 "the window refused to make the view first responder");
  SYN_ASSERT_MSG(syn_first_responder(window) == (const void *)pane,
                 "the view did not become the first responder");

  /* Null is the window itself, which AppKit accepts: the return is an answer,
   * not misuse (R12). */
  SYN_ASSERT_MSG(ns_window_make_first_responder(window, NULL),
                 "the window refused a null responder");
  SYN_ASSERT_MSG(syn_first_responder(window) == (const void *)window,
                 "a null responder did not leave the window itself focused");

  /* An editable field hands focus to the window's field editor rather than
   * taking it itself, which is AppKit's own behaviour and the reason this
   * test pins where focus landed and not only the BOOL. */
  ns_text_field *field = ns_text_field_create_with_string("focus me");
  ns_view_set_frame(ns_text_field_as_view(field), CGRectMake(10, 150, 200, 24));
  ns_view_add_subview(content, ns_text_field_as_view(field));
  SYN_ASSERT_MSG(ns_window_make_first_responder(window,
                                                ns_text_field_as_view(field)),
                 "the window refused to move focus into the field");
  SYN_ASSERT_MSG(syn_first_responder(window) != (const void *)window,
                 "focus stayed on the window instead of moving into the "
                 "field's editor");
  SYN_ASSERT_STR_EQ(syn_test_class_name(syn_first_responder(window)),
                    "NSTextView");

  ns_release(field);
  ns_release(pane);
  ns_release(window);
}
