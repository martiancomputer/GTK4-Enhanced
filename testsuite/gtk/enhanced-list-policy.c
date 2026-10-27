/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "../../gtk/gtkenhancedlistpolicyprivate.h"

static void fallback (void)
{
  const guint invalid[] = {0, 1, 31, 513, G_MAXUINT};
  for (guint i = 0; i < G_N_ELEMENTS (invalid); i++)
    g_assert_cmpuint (gtk_enhanced_list_pool_size (invalid[i], 900, 56, 200), ==, 200);
  g_assert_cmpuint (gtk_enhanced_list_pool_size (96, 0, 56, 200), ==, 200);
  g_assert_cmpuint (gtk_enhanced_list_pool_size (96, 900, 0, 200), ==, 200);
}

static void extent_cache (void)
{
  GtkEnhancedItemExtents cache = { 0 };
  g_assert_true (gtk_enhanced_item_extents_reset (&cache, 100));
  g_assert_cmpint (gtk_enhanced_item_extents_range (&cache, 0, 100, 56, FALSE), ==, 5600);
  gtk_enhanced_item_extents_record (&cache, 3, 275, 280);
  gtk_enhanced_item_extents_record (&cache, 4, 56, 56);
  g_assert_cmpint (gtk_enhanced_item_extents_range (&cache, 0, 100, 56, FALSE), ==, 5819);
  g_assert_cmpint (gtk_enhanced_item_extents_range (&cache, 0, 100, 56, TRUE), ==, 5824);
  /* Losing the widget must not lose its measured section-header size. */
  gtk_enhanced_item_extents_record (&cache, 3, 0, 0);
  g_assert_cmpint (gtk_enhanced_item_extents_range (&cache, 3, 1, 56, FALSE), ==, 275);
  g_assert_cmpint (gtk_enhanced_item_extents_range (&cache, 0, G_MAXUINT, 56, FALSE), ==, G_MAXINT);
  g_assert_true (gtk_enhanced_item_extents_reset (&cache, 50));
  g_assert_cmpint (gtk_enhanced_item_extents_range (&cache, 3, 1, 56, FALSE), ==, 56);
  g_assert_false (gtk_enhanced_item_extents_reset (&cache, GTK_ENHANCED_ITEM_EXTENTS_MAX + 1));
  g_assert_null (cache.minimum);
  g_assert_null (cache.natural);
}

static void viewport (void)
{
  g_assert_cmpuint (gtk_enhanced_list_pool_size (96, 900, 56, 200), ==, 96);
  g_assert_cmpuint (gtk_enhanced_list_pool_size (96, 900, 12, 200), ==, 225);
  for (int size = 1; size <= 16000; size += 7)
    for (int row = 1; row <= 256; row += 7)
      {
        guint actual = gtk_enhanced_list_pool_size (96, size, row, 200);
        guint visible = (size + row - 1) / row;
        g_assert_cmpuint (actual, >=, visible);
        g_assert_cmpuint (actual, >=, 96);
        g_assert_cmpuint (actual, ==, MAX (96, visible * 3));
      }
  g_assert_cmpuint (gtk_enhanced_list_pool_size (96, G_MAXINT, 1, 200), ==, G_MAXINT);
}

static void grouped_extents (void)
{
  GtkEnhancedItemExtents cache = { 0 };
  const guint8 tags[] = { 0, 2, 1, 2, 1, 2, 1 };
  g_autoptr(GBytes) groups = g_bytes_new_static (tags, sizeof tags);
  g_assert_true (gtk_enhanced_item_extents_reset (&cache, G_N_ELEMENTS (tags)));
  gtk_enhanced_item_extents_set_groups (&cache, groups);
  gtk_enhanced_item_extents_record (&cache, 0, 900, 900);
  gtk_enhanced_item_extents_record (&cache, 1, 275, 280);
  gtk_enhanced_item_extents_record (&cache, 2, 56, 56);
  int upper = gtk_enhanced_item_extents_range (&cache, 0, 7, 56, FALSE);
  g_assert_cmpint (upper, ==, 900 + 3 * (275 + 56));
  /* Scrolling into previously unseen headings must not grow the scroll range. */
  gtk_enhanced_item_extents_record (&cache, 3, 275, 280);
  gtk_enhanced_item_extents_record (&cache, 5, 275, 280);
  g_assert_cmpint (gtk_enhanced_item_extents_range (&cache, 0, 7, 56, FALSE), ==, upper);
  g_assert_cmpint (gtk_enhanced_item_extents_range (&cache, 3, 4, 56, TRUE), ==, 2 * (280 + 56));
  /* Allocation and hit-testing must be inverse operations, even inside an
   * unmaterialized tile containing both tall and short items. Averaging a
   * tile's height incorrectly maps pixels within a heading to another item. */
  for (guint position = 0; position < G_N_ELEMENTS (tags); position++) {
    int prefix = gtk_enhanced_item_extents_range (&cache, 0, position, 56, FALSE);
    int expected = gtk_enhanced_item_extents_value (&cache, position, 56, FALSE);
    for (int y = 0; y < expected; y++) {
      int before, height;
      guint item = gtk_enhanced_item_extents_locate (&cache, 0, 7,
        prefix + position * 3 + y, 56, FALSE, 3, &before, &height);
      g_assert_cmpuint (item, ==, position);
      g_assert_cmpint (before, ==, prefix + position * 3);
      g_assert_cmpint (height, ==, expected);
    }
  }
  int before, height;
  g_assert_cmpuint (gtk_enhanced_item_extents_locate (&cache, 0, 7,
    -100, 56, FALSE, 3, &before, &height), ==, 0);
  g_assert_cmpuint (gtk_enhanced_item_extents_locate (&cache, 0, 7,
    G_MAXINT, 56, FALSE, 3, &before, &height), ==, 6);
  /* A malformed or obsolete classification must fall back safely. */
  g_autoptr(GBytes) invalid = g_bytes_new_static (tags, 1);
  gtk_enhanced_item_extents_set_groups (&cache, invalid);
  g_assert_null (cache.groups);
  g_assert_cmpint (gtk_enhanced_item_extents_range (&cache, 4, 1, 56, FALSE), ==, 56);
  gtk_enhanced_item_extents_set_groups (&cache, groups);
  gtk_enhanced_item_extents_reset (&cache, 0);
  g_assert_null (cache.groups);
  g_assert_null (cache.group_data);
}

static void replacement_anchor (void)
{
  /* Provisional row 1 must not become row 224 on a two-to-449 replacement. */
  g_assert_cmpuint (gtk_enhanced_replacement_anchor (1, 0, 449), ==, 1);
  g_assert_cmpuint (gtk_enhanced_replacement_anchor (200, 0, 450), ==, 200);
  g_assert_cmpuint (gtk_enhanced_replacement_anchor (25, 10, 4), ==, 13);
  g_assert_cmpuint (gtk_enhanced_replacement_anchor (25, 10, 0), ==, 10);
}

int main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/enhanced-list/fallback", fallback);
  g_test_add_func ("/enhanced-list/viewport", viewport);
  g_test_add_func ("/enhanced-list/extent-cache", extent_cache);
  g_test_add_func ("/enhanced-list/grouped-extents", grouped_extents);
  g_test_add_func ("/enhanced-list/replacement-anchor", replacement_anchor);
  return g_test_run ();
}
