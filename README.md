# Syntonic

**AppKit for C.** You have a C program that deserves to be a Mac app, and every
route from here asks you to write the interface in some other language. Syntonic
wraps AppKit class by class behind pure C23 headers — Objective-C with ARC on
the inside, one C type per AppKit class, one C function per method or property —
so the window, the toolbar and the sidebar are the real AppKit ones, and the
code around them is still C. Not a cross-platform toolkit, not a redrawn
imitation of Mac controls, and not the Objective-C runtime with the type
checking switched off.

macOS only. The floor is macOS 26. No Xcode.app required.

## The problem

AppKit is reachable only from Objective-C and Swift. A C developer who wants a
Mac-quality app today has four options.

1. **Call the Objective-C runtime by hand.** It works, but the compiler checks
   nothing and a wrong type crashes at run time.
2. **Use a cross-platform C toolkit.** [NAppGUI](https://github.com/frang75/nappgui_src)
   and [libui-ng](https://github.com/libui-ng/libui-ng) draw real native
   controls, but they stop at what Windows, macOS and Linux share: no toolbar,
   no sidebar, no status item, no outline view, no sheets — verified in their
   sources.
3. **Use a thin wrapper.** [Silicon.h](https://github.com/EimaMei/Silicon) and
   [mac_load](https://github.com/hidefromkgb/mac_load) cover a slice of Cocoa
   and describe themselves as unfinished; Silicon.h has not changed since
   December 2024.
4. **Write the user interface in Objective-C over the C core.** This works and
   needs no dependency. It costs a second language and its idioms across the
   app, it leaves every hand-off at the C seam unchecked, and it is closed to
   languages that reach Mac APIs only through a C FFI.

The fourth is the honest comparison, so here is the claim against it: Syntonic
gives you **one language across the app**, **ownership and threading rules
checked in debug builds at that seam**, and **a surface any C FFI can bind**.

Languages with a C FFI feel the same gap. [Zig's](https://github.com/mitchellh/zig-objc)
and Odin's Objective-C support stops at the runtime layer, and a published
[Odin account of subclassing NSView](https://blog.spacegirl.nl/an-adventure-into-objc-odin/)
needs raw class registration and instance-variable tricks. Rust and Go have
serious AppKit bindings — cacao, darwinkit — and both have gone quiet. Nothing
gives C, or the languages that build on C, broad and checked access to AppKit.

**What Syntonic promises is anything AppKit can do, not parity with Swift.**
SwiftUI, WidgetKit and App Intents have no Objective-C interface a C library can
reach, and never will through this door.

## The example

This is `examples/hello_window.c`, verbatim — an application, a titled window,
and a delegate that quits when the window closes.

<!-- examples/hello_window.c -->

```c
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
```

## Build and use

**Prerequisites.** macOS 26 or newer, the Command Line Tools
(`xcode-select --install`), [CMake](https://cmake.org) 3.28+ and
[just](https://github.com/casey/just). **Xcode.app is not needed** — not to
build, not to bundle, not to sign.

**Build and run the example.**

```sh
git clone https://github.com/<owner>/syntonic.git
cd syntonic
just build              # configures and builds the library, the example and the tests
./build/hello_window    # a window appears; close it to quit
```

A bare executable has no Dock icon and no menu bar. `just bundle` turns any
built executable into a signed `.app` that does:

```sh
just bundle build/hello_window "Hello Window" dev.kaino.syntonic.hello
open "build/Hello Window.app"
```

**Use it from CMake.** v0 is consumed as source — there is no package, no
static-library download, no Homebrew formula. Drop the directory into your tree
and add two lines:

```cmake
add_subdirectory(third_party/syntonic)
target_link_libraries(my_app PRIVATE syntonic)
```

**Use it without CMake.** One command compiles your app and the library
together:

```sh
clang -std=c23 -fobjc-arc -mmacos-version-min=26.0 \
  -Isyntonic/include my_app.c syntonic/src/*.m \
  -framework AppKit -framework Foundation -o my_app
```

`just --list` shows the rest: `just test` runs the CTest suites, `just test-san`
runs them under AddressSanitizer and UndefinedBehaviorSanitizer, `just leaks`
runs the leak gate, `just lint-headers` and `just check-c11` hold the public
headers to their rules.

## Conventions

The wrappers are hand-written, and they are all written the same way.
[`docs/conventions.md`](docs/conventions.md) is the contract — naming, ownership,
boundary types, callbacks, threading, availability, misuse checks, and a
step-by-step for wrapping a new class. In brief:

- **Naming.** Every public identifier starts with `ns_`. A function's name is
  its AppKit selector, snake_cased: `-[NSWindow makeKeyAndOrderFront:]` becomes
  `ns_window_make_key_and_order_front`.
- **Ownership.** A function named `create_` or `copy_` returns a reference you
  own and end with `ns_release` (or `ns_string_free` for a string). **Every
  other function returns a borrowed reference**, valid while its owner holds it;
  keep one longer with `ns_retain`.
- **Callbacks.** A delegate protocol becomes a struct of function pointers plus
  a `void *context`, installed with `ns_<class>_set_callbacks`. Every member is
  optional, and an unset member behaves exactly as an unimplemented method.
- **Threading.** Everything is main-thread only, with two named exceptions:
  `ns_main_thread_dispatch`, which is how another thread reaches AppKit, and
  `ns_string_free`.
- **Checked in debug.** A call from the wrong thread, a null where the header
  says `_Nonnull`, a handle of the wrong class, and an Objective-C exception
  raised inside a call each report the class and the function and stop the
  process. Under `NDEBUG` all of it compiles out.

The headers that ship today are [`ns_base.h`](include/syntonic/ns_base.h),
[`ns_application.h`](include/syntonic/ns_application.h),
[`ns_window.h`](include/syntonic/ns_window.h) and
[`ns_view_controller.h`](include/syntonic/ns_view_controller.h). They are the
readable specification; each one names the AppKit selector behind every
function.

## What it costs you today

Wrapping by hand buys the checking and charges for it. v0 has no sugar layer,
so the friction is visible:

- **Long names.** A function is its full selector, so a window comes from
  `ns_window_create_with_content_rect_style_mask_backing_defer`. Predictable,
  and long.
- **Manual ownership.** Every `create_` and `copy_` return is an `ns_release` or
  an `ns_string_free` you write yourself, in the right place, including on the
  paths where AppKit ends the process.
- **Hand-written upcasts.** Passing a button where a view is wanted is a call:
  `ns_button_as_view(button)`. C has no implicit upcast, and the checking is the
  reason the header refuses the raw pointer.
- **Callback structs.** A delegate is a struct you fill in, a `context` pointer
  whose lifetime is yours, and an install call — not a closure.
- **No activation-policy or menu-bar wrapper yet.** That is why the example is
  quit by closing its window rather than by Command-Q.

A sugar header that shortens the common paths is planned and deliberately not in
v0; the friction is being recorded first, so it shortens the paths people
actually hit.

## Status

**v0, in progress, published early on purpose.** The build, test, bundle and
lint tooling is in place, the conventions are written, and the first wrappers —
`NSApplication`, `NSWindow`, `NSViewController` and the shared kernel — are
done. Nothing is packaged: you consume the source.

Next: views and layout, controls, the toolbar, the sidebar and outline view, the
menu bar, and a twin app built twice, once in Swift and once in C, to hold the C
one to the Swift one's quality.

Deferred, and named as deferred: a header generator, the sugar header, sheets,
drag and drop, menu bar extras, popovers, custom-drawn views, document-based
apps, notarization and dmg packaging, ready-made bindings for Zig, Odin, Nim and
C3, and a documentation site.

**If you want this, say so.** An issue describing the app you would build, the
class you need first, or the language you would bind from is worth more right
now than a pull request.

## License

MIT. See [LICENSE](LICENSE).
