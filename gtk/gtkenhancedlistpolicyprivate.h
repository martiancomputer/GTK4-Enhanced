/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <glib.h>

/* Experimental per-view protocol; no installed API or new exported symbols. */
#define GTK_ENHANCED_LIST_POOL_VERSION "gtk4-enhanced-list-pool-version"
#define GTK_ENHANCED_LIST_POOL_BUDGET "gtk4-enhanced-list-pool-budget-v1"
#define GTK_ENHANCED_LIST_POOL_APPLIED "gtk4-enhanced-list-pool-applied-v1"

static inline guint
gtk_enhanced_list_pool_size (guint requested, int viewport, int smallest_row,
                             guint upstream_default)
{
  if (requested < 32 || requested > 512 || viewport <= 0 || smallest_row <= 0)
    return upstream_default;

  /* Never under-realize a dense or high-DPI viewport. Retain at least three
   * screens of the smallest measured item; unknown geometry uses upstream. */
  guint64 visible = ((guint64) viewport + smallest_row - 1) / smallest_row;
  return MAX (requested, (guint) MIN (visible * 3, (guint64) G_MAXINT));
}
