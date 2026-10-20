/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <glib-object.h>

/* Experimental object-data protocol, not installed API. All access is on the
 * owning main context. Zero/invalid values preserve the upstream scheduler. */
#define GDK_ENHANCED_FRAME_POLICY_VERSION_KEY "gtk4-enhanced-frame-policy-version"
#define GDK_ENHANCED_FRAME_POLICY_LIMIT_KEY "gtk4-enhanced-max-fps-v1"
#define GDK_ENHANCED_UNSYNC_VERSION_KEY "gtk4-enhanced-unsynchronized-version"
#define GDK_ENHANCED_UNSYNC_REQUEST_KEY "gtk4-enhanced-unsynchronized-v1"
#define GDK_ENHANCED_UNSYNC_AVAILABLE_KEY "gtk4-enhanced-unsynchronized-available-v1"

static inline gboolean
gdk_enhanced_frame_policy_unsynchronized (GObject *clock)
{
  guint fps = GPOINTER_TO_UINT (g_object_get_data (clock, GDK_ENHANCED_FRAME_POLICY_LIMIT_KEY));
  return fps >= 30 && fps <= 360 &&
         GPOINTER_TO_UINT (g_object_get_data (clock, GDK_ENHANCED_UNSYNC_REQUEST_KEY)) == 1 &&
         GPOINTER_TO_UINT (g_object_get_data (clock, GDK_ENHANCED_UNSYNC_AVAILABLE_KEY)) == 1;
}

typedef struct
{
  guint fps;
  guint remainder;
  guint64 deadline;
} GdkEnhancedFramePolicy;

static inline void
gdk_enhanced_frame_policy_sync (GdkEnhancedFramePolicy *policy,
                                guint                   fps)
{
  if (fps < 30 || fps > 360)
    fps = 0;

  if (policy->fps == fps)
    return;

  policy->fps = fps;
  policy->deadline = 0;
  policy->remainder = fps ? fps - 1 : 0;
}

static inline void
gdk_enhanced_frame_policy_advance (GdkEnhancedFramePolicy *policy,
                                   guint64                 now)
{
  guint64 interval;

  if (policy->fps == 0)
    return;

  interval = (G_NSEC_PER_SEC + policy->fps - 1) / policy->fps;
  /* Rebase after idle/stalls instead of queuing missed frames. Small dispatch
   * jitter keeps the absolute cadence rather than adding a delay every time. */
  if (policy->deadline == 0 ||
      (now > policy->deadline && now - policy->deadline >= interval))
    {
      policy->deadline = now;
      policy->remainder = policy->fps - 1;
    }

  interval = G_NSEC_PER_SEC / policy->fps;
  policy->remainder += G_NSEC_PER_SEC % policy->fps;
  if (policy->remainder >= policy->fps)
    {
      interval++;
      policy->remainder -= policy->fps;
    }
  policy->deadline = MIN (policy->deadline, G_MAXUINT64 - interval) + interval;
}
