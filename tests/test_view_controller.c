/*
 * The view controller suite (U14): create, the view it was given, and the
 * window content view that comes from it (R7, R14).
 */

#include <stdbool.h>
#include <stddef.h>

#include "runner.h"
#include "support.h"
#include "syntonic/syntonic.h"

SYN_TEST(a_view_controller_reports_the_view_it_was_given) {
  syn_test_bootstrap();

  ns_view_controller *controller = ns_view_controller_create();
  SYN_ASSERT_STR_EQ(syn_test_class_name(controller), "NSViewController");

  const void *view = syn_test_view_create();
  ns_view_controller_set_view(controller, (ns_view *)(void *)view);
  SYN_ASSERT_MSG(ns_view_controller_view(controller) == (ns_view *)(void *)view,
                 "the controller reported a view other than the one it was "
                 "given");

  /* The controller retains the view, so the caller's release is correct (R7)
   * and the borrowed getter still reads. */
  ns_release(view);
  SYN_ASSERT_MSG(!syn_test_view_is_gone(), "the controller did not retain it");
  SYN_ASSERT_MSG(ns_view_controller_view(controller) == (ns_view *)(void *)view,
                 "the borrowed view did not survive the caller's release");

  ns_release(controller);
  SYN_WAIT_FOR(syn_test_view_is_gone(), 1000);
}

SYN_TEST(a_windows_content_view_is_its_content_view_controllers_view) {
  syn_test_bootstrap();

  const void *view = syn_test_view_create();
  ns_view_controller *controller = ns_view_controller_create();
  ns_view_controller_set_view(controller, (ns_view *)(void *)view);

  ns_window *window = ns_window_create_with_content_rect_style_mask_backing_defer(
      CGRectMake(0, 0, 400, 300),
      NS_WINDOW_STYLE_MASK_TITLED | NS_WINDOW_STYLE_MASK_CLOSABLE,
      NS_BACKING_STORE_BUFFERED, false);
  ns_window_set_content_view_controller(window, controller);

  SYN_ASSERT_MSG(ns_window_content_view(window) == (ns_view *)(void *)view,
                 "the window's content view is not the controller's view");

  ns_release(controller);
  ns_release(view);
  SYN_ASSERT_MSG(ns_window_content_view(window) == (ns_view *)(void *)view,
                 "the window's tree did not hold the controller and its view");

  ns_release(window);
  SYN_WAIT_FOR(syn_test_view_is_gone(), 1000);
}

SYN_TEST(a_view_controller_created_and_released_on_its_own_is_clean) {
  syn_test_bootstrap();
  ns_view_controller *controller = ns_view_controller_create();
  ns_release(controller);
}
