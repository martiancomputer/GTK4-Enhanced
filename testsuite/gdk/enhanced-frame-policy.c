/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "config.h"
#include "gdk/gdkenhancedframepolicyprivate.h"
#include "gdk/gdkframeclockidleprivate.h"
#include "gdk/gdkframeclockprivate.h"

static void
test_arithmetic (void)
{
  const guint rates[] = { 30, 60, 120, 144, 240, 360 };
  for (guint i = 0; i < G_N_ELEMENTS (rates); i++)
    {
      GdkEnhancedFramePolicy policy = { 0 };
      gint64 start = 1000000;
      gdk_enhanced_frame_policy_sync (&policy, rates[i]);
      for (guint n = 1; n <= rates[i]; n++)
        {
          gdk_enhanced_frame_policy_advance (&policy, n == 1 ? start : policy.deadline);
          g_assert_cmpuint (policy.deadline, ==,
                          start + ((gint64) n * G_NSEC_PER_SEC + rates[i] - 1) / rates[i]);
        }
      g_assert_cmpuint (policy.deadline, ==, start + G_NSEC_PER_SEC);
    }
}

static void
test_reset_and_stall (void)
{
  GdkEnhancedFramePolicy policy = { 0 };
  gdk_enhanced_frame_policy_sync (&policy, 144);
  gdk_enhanced_frame_policy_advance (&policy, G_GUINT64_CONSTANT (1000000000));
  gdk_enhanced_frame_policy_advance (&policy, G_GUINT64_CONSTANT (9000000000));
  g_assert_cmpuint (policy.deadline, ==, G_GUINT64_CONSTANT (9006944445));
  gdk_enhanced_frame_policy_sync (&policy, 30);
  g_assert_cmpuint (policy.deadline, ==, 0);
  gdk_enhanced_frame_policy_advance (&policy, G_GUINT64_CONSTANT (10000000000));
  g_assert_cmpuint (policy.deadline, ==, G_GUINT64_CONSTANT (10033333334));
  gdk_enhanced_frame_policy_sync (&policy, 0);
  g_assert_cmpuint (policy.fps, ==, 0);
  g_assert_cmpuint (policy.deadline, ==, 0);
  gdk_enhanced_frame_policy_sync (&policy, 29);
  g_assert_cmpuint (policy.fps, ==, 0);
  gdk_enhanced_frame_policy_sync (&policy, 361);
  g_assert_cmpuint (policy.fps, ==, 0);
  gdk_enhanced_frame_policy_sync (&policy, G_MAXUINT);
  g_assert_cmpuint (policy.fps, ==, 0);
  gdk_enhanced_frame_policy_sync (&policy, 360);
  gdk_enhanced_frame_policy_advance (&policy, G_MAXUINT64 - 1);
  g_assert_cmpuint (policy.deadline, ==, G_MAXUINT64);
}

typedef struct
{
  guint frames;
  gint64 previous;
} ClockProbe;

static void
on_update (GdkFrameClock *clock,
           ClockProbe    *probe)
{
  gint64 now = gdk_frame_clock_get_frame_time (clock);
  g_assert_cmpint (now, >=, probe->previous);
  probe->previous = now;
  probe->frames++;
}

static gboolean
quit_loop (gpointer data)
{
  g_main_loop_quit (data);
  return G_SOURCE_REMOVE;
}

static void
run_for (guint milliseconds)
{
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);
  g_timeout_add (milliseconds, quit_loop, loop);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);
}

/* Real scheduler, no display, window, renderer, or synthetic UI input. This
 * checks lifecycle/limits, not physical presentation or exact timer latency. */
static void
test_clock_lifecycle (void)
{
  GdkFrameClock *clock = _gdk_frame_clock_idle_new ();
  ClockProbe probe = { 0 };
  g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (clock),
    GDK_ENHANCED_FRAME_POLICY_VERSION_KEY)), ==, 1);
  g_signal_connect (clock, "update", G_CALLBACK (on_update), &probe);
  g_object_set_data (G_OBJECT (clock), GDK_ENHANCED_FRAME_POLICY_LIMIT_KEY, GUINT_TO_POINTER (30));
  gdk_frame_clock_start (clock);
  gdk_frame_clock_begin_updating (clock);
  run_for (150);
  g_assert_cmpuint (probe.frames, >=, 2);
  g_assert_cmpuint (probe.frames, <=, 6);
  gdk_frame_clock_end_updating (clock);
  guint stopped = probe.frames;
  run_for (60);
  g_assert_cmpuint (probe.frames, ==, stopped);

  g_object_set_data (G_OBJECT (clock), GDK_ENHANCED_FRAME_POLICY_LIMIT_KEY, NULL);
  gdk_frame_clock_begin_updating (clock);
  run_for (100);
  g_assert_cmpuint (probe.frames, >, stopped);
  gdk_frame_clock_end_updating (clock);
  gdk_frame_clock_stop (clock);
  guint frozen = probe.frames;
  gdk_frame_clock_begin_updating (clock);
  run_for (60);
  g_assert_cmpuint (probe.frames, ==, frozen);
  gdk_frame_clock_end_updating (clock);
  gdk_frame_clock_start (clock);
  gpointer weak_clock = clock;
  g_object_add_weak_pointer (G_OBJECT (clock), &weak_clock);
  gdk_frame_clock_stop (clock);
  g_object_unref (clock);
  g_assert_null (weak_clock);
}

typedef struct
{
  guint updates;
  guint flushes;
  guint resumes;
  guint paints;
} InputProbe;

static gboolean
request_input (gpointer data)
{
  gdk_frame_clock_request_phase (data, GDK_FRAME_CLOCK_PHASE_FLUSH_EVENTS);
  return G_SOURCE_REMOVE;
}

static void
on_input_update (GdkFrameClock *clock, InputProbe *probe)
{
  probe->updates++;
  if (probe->updates == 1)
    g_timeout_add (2, request_input, clock);
}

static void
on_flush (GdkFrameClock *clock, InputProbe *probe)
{
  probe->flushes++;
  /* Input was dispatched before the second limited rendering cycle. */
  g_assert_cmpuint (probe->updates, ==, 1);
  gdk_frame_clock_request_phase (clock, GDK_FRAME_CLOCK_PHASE_RESUME_EVENTS |
                                      GDK_FRAME_CLOCK_PHASE_PAINT);
}

static void
on_resume (GdkFrameClock *clock, InputProbe *probe)
{
  probe->resumes++;
  /* A full render cycle would have emitted update before resuming events. */
  g_assert_cmpuint (probe->updates, ==, 1);
  (void) clock;
}

static void
on_input_paint (GdkFrameClock *clock, InputProbe *probe)
{
  probe->paints++;
  g_assert_cmpuint (probe->updates, >=, 2);
  (void) clock;
}

static void
test_input_during_limit (void)
{
  GdkFrameClock *clock = _gdk_frame_clock_idle_new ();
  InputProbe probe = { 0 };
  g_object_set_data (G_OBJECT (clock), GDK_ENHANCED_FRAME_POLICY_LIMIT_KEY,
                     GUINT_TO_POINTER (30));
  g_signal_connect (clock, "update", G_CALLBACK (on_input_update), &probe);
  g_signal_connect (clock, "flush-events", G_CALLBACK (on_flush), &probe);
  g_signal_connect (clock, "resume-events", G_CALLBACK (on_resume), &probe);
  g_signal_connect (clock, "paint", G_CALLBACK (on_input_paint), &probe);
  gdk_frame_clock_presented (clock, -1, g_get_monotonic_time_ns (),
                             (G_NSEC_PER_SEC + 359) / 360);
  gdk_frame_clock_start (clock);
  gdk_frame_clock_begin_updating (clock);
  run_for (120);
  g_assert_cmpuint (probe.updates, >=, 2);
  g_assert_cmpuint (probe.updates, <=, 5);
  g_assert_cmpuint (probe.flushes, ==, 1);
  g_assert_cmpuint (probe.resumes, ==, 1);
  g_assert_cmpuint (probe.paints, ==, 1);
  gdk_frame_clock_end_updating (clock);
  gdk_frame_clock_stop (clock);
  g_object_unref (clock);
}

static void
test_scheduler_rates (void)
{
  const guint rates[] = { 30, 60, 120, 144, 240, 360 };
  GdkFrameClock *clock = _gdk_frame_clock_idle_new ();
  ClockProbe probe = { 0 };
  g_signal_connect (clock, "update", G_CALLBACK (on_update), &probe);
  /* Simulated backend feedback for scheduler coverage only; this is not a
   * physical monitor test or evidence of presented frames. */
  gdk_frame_clock_presented (clock, -1, g_get_monotonic_time_ns (),
                             (G_NSEC_PER_SEC + 359) / 360);
  gdk_frame_clock_start (clock);
  for (guint i = 0; i < G_N_ELEMENTS (rates); i++)
    {
      guint before = probe.frames;
      g_object_set_data (G_OBJECT (clock), GDK_ENHANCED_FRAME_POLICY_LIMIT_KEY,
                         GUINT_TO_POINTER (rates[i]));
      gdk_frame_clock_begin_updating (clock);
      run_for (120);
      gdk_frame_clock_end_updating (clock);
      guint frames = probe.frames - before;
      g_assert_cmpuint (frames, >=, 2);
      g_assert_cmpuint (frames, <=, (rates[i] * 120 + 999) / 1000 + 1);
    }
  gdk_frame_clock_stop (clock);
  g_object_unref (clock);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/enhanced-frame-policy/arithmetic", test_arithmetic);
  g_test_add_func ("/enhanced-frame-policy/reset-stall-overflow", test_reset_and_stall);
  g_test_add_func ("/enhanced-frame-policy/clock-lifecycle", test_clock_lifecycle);
  g_test_add_func ("/enhanced-frame-policy/input-during-limit", test_input_during_limit);
  g_test_add_func ("/enhanced-frame-policy/scheduler-rates", test_scheduler_rates);
  return g_test_run ();
}
