/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <glib-object.h>
#include "gdkframeclockprivate.h"

/* Experimental object-data protocol, not installed API. All access is on the
 * owning main context. Zero/invalid values preserve the upstream scheduler. */
#define GDK_ENHANCED_FRAME_POLICY_VERSION_KEY "gtk4-enhanced-frame-policy-version"
#define GDK_ENHANCED_FRAME_POLICY_LIMIT_KEY "gtk4-enhanced-max-fps-v1"
#define GDK_ENHANCED_UNSYNC_VERSION_KEY "gtk4-enhanced-unsynchronized-version"
#define GDK_ENHANCED_UNSYNC_REQUEST_KEY "gtk4-enhanced-unsynchronized-v1"
#define GDK_ENHANCED_UNSYNC_AVAILABLE_KEY "gtk4-enhanced-unsynchronized-available-v1"

typedef struct
{
  gint64 frame_counter;
  gboolean pending;
  gboolean submitted;
} GdkEnhancedSubmission;

static void
gdk_enhanced_frame_policy_finish_submission (GdkFrameClock       *clock,
                                            GdkEnhancedSubmission *submission)
{
  if (!submission->pending)
    return;

  submission->pending = FALSE;
  if (submission->submitted)
    gdk_frame_clock_submitted (clock, submission->frame_counter, 0);
  else
    gdk_frame_clock_discarded (clock, submission->frame_counter);
}

/* The backend's end_frame() returns before the common draw-context wrapper
 * marks the frame OUTSTANDING. Complete it only in AFTER_PAINT, after every
 * context has returned. Coalesce presents sharing a clock/frame; one queued
 * buffer is enough to count the frame as submitted. No presentation timestamp
 * is fabricated, and no timer or per-frame closure is allocated. */
static inline void
gdk_enhanced_frame_policy_record_submission (GdkFrameClock *clock,
                                            gboolean       submitted)
{
  const char *key = "gtk4-enhanced-pending-submission-v1";
  GdkEnhancedSubmission *submission = g_object_get_data (G_OBJECT (clock), key);
  gint64 counter = gdk_frame_clock_get_frame_counter (clock);

  if (submission == NULL)
    {
      submission = g_new0 (GdkEnhancedSubmission, 1);
      g_object_set_data_full (G_OBJECT (clock), key, submission, g_free);
      g_signal_connect (clock, "after-paint",
                        G_CALLBACK (gdk_enhanced_frame_policy_finish_submission), submission);
    }

  if (!submission->pending || submission->frame_counter != counter)
    {
      submission->frame_counter = counter;
      submission->submitted = submitted;
    }
  else
    submission->submitted |= submitted;
  submission->pending = TRUE;
}

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
