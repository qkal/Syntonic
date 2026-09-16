/*
 * Syntonic - AppKit for C.
 *
 * Umbrella header: including this one header pulls in every public Syntonic
 * header. It is a non-mirror header (KTD18): it has no AppKit counterpart.
 *
 * The public surface is C23 and also compiles in C11 mode under clang's
 * fixed-enum extension (R2).
 */

#ifndef SYNTONIC_SYNTONIC_H
#define SYNTONIC_SYNTONIC_H

#include <os/availability.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* One line per public header, in alphabetical order; a new wrapper adds
 * its own in the same commit (R6). */
#include <syntonic/ns_application.h>
#include <syntonic/ns_base.h>
#include <syntonic/ns_button.h>
#include <syntonic/ns_color.h>
#include <syntonic/ns_control.h>
#include <syntonic/ns_font.h>
#include <syntonic/ns_grid_view.h>
#include <syntonic/ns_image.h>
#include <syntonic/ns_layout.h>
#include <syntonic/ns_menu.h>
#include <syntonic/ns_menu_bar.h>
#include <syntonic/ns_menu_item.h>
#include <syntonic/ns_outline_view.h>
#include <syntonic/ns_pop_up_button.h>
#include <syntonic/ns_scroll_view.h>
#include <syntonic/ns_stack_view.h>
#include <syntonic/ns_tab_view_controller.h>
#include <syntonic/ns_tab_view_item.h>
#include <syntonic/ns_table_column.h>
#include <syntonic/ns_table_view.h>
#include <syntonic/ns_text_field.h>
#include <syntonic/ns_view.h>
#include <syntonic/ns_view_controller.h>
#include <syntonic/ns_window.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The Syntonic library version as a static "major.minor.patch" string. Never
 * null, never freed by the caller. syntonic-owned.
 */
const char *ns_version_string(void) API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_SYNTONIC_H */
