/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <glib.h>
#include <string.h>

/* Experimental per-view protocol; no installed API or new exported symbols. */
#define GTK_ENHANCED_LIST_POOL_VERSION "gtk4-enhanced-list-pool-version"
#define GTK_ENHANCED_LIST_POOL_BUDGET "gtk4-enhanced-list-pool-budget-v1"
#define GTK_ENHANCED_LIST_POOL_APPLIED "gtk4-enhanced-list-pool-applied-v1"
#define GTK_ENHANCED_ITEM_EXTENTS_VERSION "gtk4-enhanced-item-extents-version"
#define GTK_ENHANCED_ITEM_EXTENTS_DEFAULT "gtk4-enhanced-item-extents-default-v1"
#define GTK_ENHANCED_ITEM_EXTENTS_GROUPS_VERSION "gtk4-enhanced-item-extents-groups-version"
#define GTK_ENHANCED_ITEM_EXTENTS_GROUPS "gtk4-enhanced-item-extents-groups-v1"
#define GTK_ENHANCED_ITEM_EXTENTS_MAX 16384
#define GTK_ENHANCED_REPLACEMENT_ANCHOR_VERSION "gtk4-enhanced-replacement-anchor-version"
#define GTK_ENHANCED_REPLACEMENT_ANCHOR "gtk4-enhanced-replacement-anchor-v1"

static inline guint
gtk_enhanced_replacement_anchor (guint old_position, guint start, guint added)
{
  return start + MIN (old_position - start, added > 0 ? added - 1 : 0);
}

typedef struct {
  guint length;
  int *minimum;
  int *natural;
  GBytes *groups;
  const guint8 *group_data;
  int group_minimum[256];
  int group_natural[256];
} GtkEnhancedItemExtents;

static inline gboolean
gtk_enhanced_item_extents_reset (GtkEnhancedItemExtents *cache, guint length)
{
  g_clear_pointer (&cache->minimum, g_free);
  g_clear_pointer (&cache->natural, g_free);
  g_clear_pointer (&cache->groups, g_bytes_unref);
  cache->group_data = NULL;
  memset (cache->group_minimum, 0, sizeof cache->group_minimum);
  memset (cache->group_natural, 0, sizeof cache->group_natural);
  cache->length = 0;
  if (length == 0 || length > GTK_ENHANCED_ITEM_EXTENTS_MAX)
    return FALSE;
  cache->length = length;
  cache->minimum = g_new0 (int, length);
  cache->natural = g_new0 (int, length);
  return TRUE;
}

static inline void
gtk_enhanced_item_extents_set_groups (GtkEnhancedItemExtents *cache, GBytes *groups)
{
  if (groups && g_bytes_get_size (groups) != cache->length)
    groups = NULL;
  if (groups == cache->groups)
    return;
  g_clear_pointer (&cache->groups, g_bytes_unref);
  cache->group_data = NULL;
  memset (cache->group_minimum, 0, sizeof cache->group_minimum);
  memset (cache->group_natural, 0, sizeof cache->group_natural);
  if (groups) {
    cache->groups = g_bytes_ref (groups);
    cache->group_data = g_bytes_get_data (groups, NULL);
  }
}

static inline void
gtk_enhanced_item_extents_record (GtkEnhancedItemExtents *cache, guint position,
                                  int minimum, int natural)
{
  if (position >= cache->length || minimum <= 0 || natural < minimum)
    return;
  cache->minimum[position] = minimum;
  cache->natural[position] = natural;
  /* Group zero is heterogeneous: use only the position's own measurement.
   * Other groups are caller-declared equal-height items at this width. Learn
   * one representative instead of treating unseen tall items as short rows. */
  guint group = cache->group_data ? cache->group_data[position] : 0;
  if (group && cache->group_minimum[group] == 0) {
    cache->group_minimum[group] = minimum;
    cache->group_natural[group] = natural;
  }
}

static inline int
gtk_enhanced_item_extents_value (const GtkEnhancedItemExtents *cache,
                                 guint position, int fallback, gboolean natural)
{
  if (position >= cache->length)
    return fallback;
  const int *values = natural ? cache->natural : cache->minimum;
  const int *group_values = natural ? cache->group_natural : cache->group_minimum;
  guint group = cache->group_data ? cache->group_data[position] : 0;
  int estimate = group ? group_values[group] : 0;
  return values[position] > 0 ? values[position] : estimate > 0 ? estimate : fallback;
}

static inline guint
gtk_enhanced_item_extents_locate (const GtkEnhancedItemExtents *cache,
                                  guint position, guint count, int offset,
                                  int fallback, gboolean natural, int spacing,
                                  int *before, int *height)
{
  gint64 start = 0;
  guint i;
  g_assert (count > 0);
  for (i = 0; i < count; i++) {
    *height = gtk_enhanced_item_extents_value (cache, position + i, fallback, natural);
    if (offset < start + *height + spacing || i == count - 1)
      break;
    start += *height + spacing;
  }
  *before = MIN (start, G_MAXINT);
  return i;
}

static inline int
gtk_enhanced_item_extents_range (const GtkEnhancedItemExtents *cache,
                                 guint position, guint count,
                                 int fallback, gboolean natural)
{
  gint64 total = 0;
  if (position >= cache->length || count > cache->length - position)
    return MIN ((gint64) count * fallback, G_MAXINT);
  for (guint i = position; i < position + count; i++)
    total += gtk_enhanced_item_extents_value (cache, i, fallback, natural);
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
