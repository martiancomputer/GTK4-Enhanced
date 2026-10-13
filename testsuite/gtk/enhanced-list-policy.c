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
  return g_test_run ();
}
