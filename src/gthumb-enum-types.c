
/* Generated data (by glib-mkenums) */

#include <glib-object.h>


/* enumerations from "gth-image-list.h" */
#include "gth-image-list.h"
GType
gth_cursor_movement_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { GTH_CURSOR_MOVE_UP, "GTH_CURSOR_MOVE_UP", "up" },
      { GTH_CURSOR_MOVE_DOWN, "GTH_CURSOR_MOVE_DOWN", "down" },
      { GTH_CURSOR_MOVE_RIGHT, "GTH_CURSOR_MOVE_RIGHT", "right" },
      { GTH_CURSOR_MOVE_LEFT, "GTH_CURSOR_MOVE_LEFT", "left" },
      { GTH_CURSOR_MOVE_PAGE_UP, "GTH_CURSOR_MOVE_PAGE_UP", "page-up" },
      { GTH_CURSOR_MOVE_PAGE_DOWN, "GTH_CURSOR_MOVE_PAGE_DOWN", "page-down" },
      { GTH_CURSOR_MOVE_BEGIN, "GTH_CURSOR_MOVE_BEGIN", "begin" },
      { GTH_CURSOR_MOVE_END, "GTH_CURSOR_MOVE_END", "end" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GthCursorMovement", values);
  }
  return etype;
}

GType
gth_selection_change_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { GTH_SELCHANGE_NONE, "GTH_SELCHANGE_NONE", "none" },
      { GTH_SELCHANGE_SET, "GTH_SELCHANGE_SET", "set" },
      { GTH_SELCHANGE_SET_RANGE, "GTH_SELCHANGE_SET_RANGE", "set-range" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GthSelectionChange", values);
  }
  return etype;
}


/* Generated data ends here */

