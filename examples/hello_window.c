/*
 * The smallest Syntonic app: the shared application, one titled window, and a
 * delegate struct that shows the window at launch and quits when it closes.
 *
 * Build it with `just build`, then bundle and run it:
 *
 *     just bundle build/hello_window "Hello Window" dev.kaino.syntonic.hello
 *     open "build/Hello Window.app"
 */

#include <syntonic/syntonic.h>

/* The window is the context handed to ns_application_set_callbacks, borrowed
 * back here; main still owns it. */
static void on_launch(void *context, ns_application *application) {
  ns_window_make_key_and_order_front((ns_window *)context);
  ns_application_activate(application);
}

static bool quit_when_the_window_closes(void *context,
                                        ns_application *application) {
  (void)context;
  (void)application;
  return true;
}

/* On the way out, give back the reference main owns. */
static void on_terminate(void *context, ns_application *application) {
  (void)application;
  ns_release(context);
}

int main(void) {
  ns_application *application = ns_application_shared();

  /* create_ returns a reference this function owns (docs/conventions.md). */
  ns_window *window =
      ns_window_create_with_content_rect_style_mask_backing_defer(
          CGRectMake(0, 0, 480, 320),
          NS_WINDOW_STYLE_MASK_TITLED | NS_WINDOW_STYLE_MASK_CLOSABLE |
              NS_WINDOW_STYLE_MASK_MINIATURIZABLE |
              NS_WINDOW_STYLE_MASK_RESIZABLE,
          NS_BACKING_STORE_BUFFERED, false);
  ns_window_set_title(window, "Hello from C");

  static const ns_application_callbacks callbacks = {
      .did_finish_launching = on_launch,
      .should_terminate_after_last_window_closed = quit_when_the_window_closes,
      .will_terminate = on_terminate,
  };
  ns_application_set_callbacks(application, &callbacks, window);

  ns_application_run(application); /* does not return; on_terminate cleans up */
  return 0;
}
