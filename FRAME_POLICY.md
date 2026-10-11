# Experimental frame-rate ceiling (R1)

## Problem and boundary

Application-side snapshot suppression can disrupt layout, rendering and input
without controlling GDK's scheduler. GTK 4.24.1 already uses nanosecond timeouts,
monitor/presentation feedback, monotonic animation times and backend throttling.
Keep those mechanisms; add only a per-frame-clock ceiling on rendering work.
This does not force redraws or promise a physical refresh rate.

## Prototype interface and ownership

On the owning GTK main context, inspect GObject data on the window's
`GdkFrameClock`. `gtk4-enhanced-frame-policy-version` equals the integer pointer
value 1 when supported. Set `gtk4-enhanced-max-fps-v1` with `GUINT_TO_POINTER(fps)`.
Integer values 30 through 360 are accepted; zero, absent or invalid values use
the upstream scheduler. No object or heap allocation is passed or retained.
Request a redraw after changing the value; an already queued frame may retain
its previous timeout until dispatch. This is an experimental protocol, not an
installed public API. Consumers must capability-check and support stock GTK.

## Scheduling and input

The private idle clock combines the upstream minimum next frame time with an
absolute, rational nanosecond deadline. Carrying fractional periods avoids
accumulated rounding drift. Long stalls rebase the deadline instead of queuing
catch-up frames. The compositor/backend can always impose a lower rate.
Presentation timestamps and reported monitor refresh are never fabricated.

Flush/resume-events can run at upstream input cadence before the next limited
render cycle. That path leaves rendering requests intact, opens no frame or
timings record, and does not emit update/layout/paint. The next render cycle
retains normal phase ordering. Sources exist only while requested/updating and
while the clock is started; disposal destroys pending work. There is no polling
timer or application-specific code in GTK. Private state is constant-size per
clock. Zero/Default never takes the input-only extension path.

## Compatibility and scope

The public headers, library SONAME and exported API must match upstream 4.24.1.
The implementation is backend-neutral but initially verified on Linux only.
Windows/macOS, multi-monitor changes, VRR, renderer matrices, power costs and
physical 120/144/240/360 Hz presentation require separate validation. Upstream
accessibility and compositor controls are not bypassed. No additional dependency
beyond the approved upstream baseline upgrade is introduced. Modified GTK source
remains LGPL-2.1-or-later with original notices retained.

## Verification and rollback

`enhanced-frame-policy` is headless: arithmetic at every offered FPS, invalid
values, overflow, stall recovery, real main-loop clock lifecycle, idle/stop
cancellation and input flushing during a ceiling. These are scheduler tests,
not GUI tests or presentation benchmarks. Compare exported symbols and installed
public headers against the exact upstream tag; record tooling limitations.
For rollback, clear the object-data value or run the stock runtime. Consumer
builds and the isolated fork installation must remain separate from system GTK.
