/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <glib.h>

/* Experimental per-view protocol; no installed API or new exported symbols. */
#define GTK_ENHANCED_LIST_POOL_VERSION "gtk4-enhanced-list-pool-version"
#define GTK_ENHANCED_LIST_POOL_BUDGET "gtk4-enhanced-list-pool-budget-v1"
#define GTK_ENHANCED_LIST_POOL_APPLIED "gtk4-enhanced-list-pool-applied-v1"
#define GTK_ENHANCED_ITEM_EXTENTS_VERSION "gtk4-enhanced-item-extents-version"
#define GTK_ENHANCED_ITEM_EXTENTS_DEFAULT "gtk4-enhanced-item-extents-default-v1"
#define GTK_ENHANCED_ITEM_EXTENTS_MAX 16384

typedef struct {
  guint length;
  int *minimum;
  int *natural;
} GtkEnhancedItemExtents;

static inline gboolean
gtk_enhanced_item_extents_reset (GtkEnhancedItemExtents *cache, guint length)
{
  g_clear_pointer (&cache->minimum, g_free);
  g_clear_pointer (&cache->natural, g_free);
  cache->length = 0;
  if (length == 0 || length > GTK_ENHANCED_ITEM_EXTENTS_MAX)
    return FALSE;
  cache->length = length;
  cache->minimum = g_new0 (int, length);
  cache->natural = g_new0 (int, length);
  return TRUE;
}

static inline void
gtk_enhanced_item_extents_record (GtkEnhancedItemExtents *cache, guint position,
                                  int minimum, int natural)
{
  if (position >= cache->length || minimum <= 0 || natural < minimum)
    return;
  cache->minimum[position] = minimum;
  cache->natural[position] = natural;
}

static inline int
gtk_enhanced_item_extents_range (const GtkEnhancedItemExtents *cache,
                                 guint position, guint count,
                                 int fallback, gboolean natural)
{
  gint64 total = 0;
  if (position >= cache->length || count > cache->length - position)
    return MIN ((gint64) count * fallback, G_MAXINT);
  const int *values = natural ? cache->natural : cache->minimum;
  for (guint i = position; i < position + count; i++)
    total += values[i] > 0 ? values[i] : fallback;
  return MIN (total, G_MAXINT);
}

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
