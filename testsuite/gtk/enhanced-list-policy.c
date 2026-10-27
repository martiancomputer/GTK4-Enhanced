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

int main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/enhanced-list/fallback", fallback);
  g_test_add_func ("/enhanced-list/viewport", viewport);
  g_test_add_func ("/enhanced-list/extent-cache", extent_cache);
  return g_test_run ();
}
