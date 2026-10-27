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

## Opt-in unsynchronized rendering prototype (R1, experimental)

Correctness/performance gate: the existing ceiling preserves native pacing,
so it cannot measure rendering throughput above a compositor's frame-callback
cadence. Add a separate, default-off request; do not change ceiling semantics.
The hypothesis is that bounded render scheduling, monotonic animation times,
and nonblocking Vulkan presentation can produce additional real submissions
without requesting synthetic redraws or misreporting physical presentation.

Touched components: the private frame-policy protocol and idle clock, Wayland
frame requests, Vulkan swapchain capability reporting, and the consumer's
Performance settings. No public API, exported symbol, SONAME, dependency floor
or normal-mode behavior is to change. The initial supported path is focused
Wayland/Vulkan windows, an explicit 30..360 ceiling, and MAILBOX or IMMEDIATE
presentation. Other paths retain native pacing. Compositor/driver backpressure
can still prevent the requested render rate; screen refresh is not overridden.

Verification: extend headless arithmetic/scheduler/lifecycle and settings
coverage; compare dynamic exports; build the isolated library and consumer;
manually check real Vulkan submissions with the overlay, then idle, focus,
resize and synchronized fallback. No automated UI input. Rollback is to turn
the request off (or select Default), or to load the previous isolated runtime.
Do not infer physical high-refresh support from a renderer or overlay counter.

The ceiling protocol stays at version 1 for existing consumers. A separate
`gtk4-enhanced-unsynchronized-version` integer-pointer value 1 advertises the
experimental extension. Set `gtk4-enhanced-unsynchronized-v1` to integer-pointer
1 on the owning clock to request it; zero/absent disables it. GDK sets
`gtk4-enhanced-unsynchronized-available-v1` only for a Wayland Vulkan swapchain
using MAILBOX/IMMEDIATE. A valid explicit ceiling is mandatory. Consumers must
gate the request to their focused window. A suspended surface never opts out
of compositor throttling. The extension creates no permanent repaint source.

In throughput mode, frame completion is reported as submission, not physical
presentation. The display still scans out at its actual refresh rate. Turning
the request off restores the ordinary callback path on the next frame.

Local verification: the isolated shared library and consumer build passed;
the six headless GTK suites passed 88 subtests, including seven frame-policy
cases. The consumer's settings, shortcuts, presentation and layout suites
passed. Defined dynamic export names/kinds remain identical (5,510) and the
SONAME remains libgtk-4.so.1; this is not a full ABI-layout certification.
The consumer's headless help path also runs against stock GTK.

The user confirmed the experimental consumer path works on native
Wayland/Vulkan with an approximately 60 Hz output and a 120 FPS render limit.
No exact throughput or frame-time distribution was independently recorded.
This does not certify 120 Hz physical presentation, power consumption,
Windows/macOS behavior, mixed monitors, or the complete renderer matrix.
Existing upstream null-dereference warnings in array-test assertions were
not suppressed; the tests passed. No automated UI interaction was used.

## Verification and rollback

Stable heterogeneous lists can opt into `gtk4-enhanced-item-extents-default-v1`
after checking `gtk4-enhanced-item-extents-version == 1`. The value is their
default logical row estimate (1–4096 pixels). Measured minimum/natural item
heights survive recycling, so tall section headings do not repeatedly revert
to the visible rows' median. Entries reset on model changes and width changes.
The cache is limited to 16,384 items (128 KiB of integer extents), owns no
artwork, and is disabled by default. Clients must replace/invalidate their model
when changing height-affecting presentation outside a width change.

Views may additionally check `gtk4-enhanced-item-extents-groups-version == 1`
and attach a `GBytes` as `gtk4-enhanced-item-extents-groups-v1` with
`g_object_set_data_full()` and `g_bytes_unref` as its destroy notifier. The bytes
must have exactly one entry per model item. Zero means ungrouped; other byte
values declare items with equal minimum/natural heights at a given width.
GTK learns one representative measurement per group and uses it for unseen
members, while preserving each item's own measurement once available. This
avoids progressively growing a scroll range as tall headings are discovered.
Update the bytes before replacing the model contents. Model/width changes
clear learned group sizes; invalid lengths disable grouping. Group metadata
adds at most 16 KiB plus 2 KiB of integer estimates and retains no widgets or
artwork. This remains an experimental private protocol with stock behavior
when absent, rather than an installed API or an exported symbol.
The extent-enabled path uses the same per-item measurements for tile splits,
item allocation and pixel-to-item lookup. Applying uniform-height arithmetic
to a heterogeneous offscreen tile would select the wrong scroll anchor even
when the tile's total height was correct.

List/grid views advertise `gtk4-enhanced-replacement-anchor-version == 1`.
Setting `gtk4-enhanced-replacement-anchor-v1` to `GUINT_TO_POINTER(1)` keeps
a deleted tracked item's ordinal within the replacement range, clamped to
its new length. Upstream's default proportional fallback remains unchanged
without this opt-in. It can otherwise map row 1 of two provisional rows to
row 224 when 449 real items replace them. Surviving object identities still
use GTK's existing identity-preserving path. This policy neither changes the
scroll adjustment directly nor creates a deferred scroll/focus request.

Wayland Vulkan availability belongs to the toplevel context. Popup creation
must not replace that owner, and popup destruction must not clear a shared
frame clock's availability. Otherwise throughput falls back until a toplevel
swapchain resize republishes the capability. The ownership regression also
checks that an older context cannot clear its replacement's state.
When a popup requests native presentation feedback on a shared clock, that
feedback completes the frame. Throughput submission must not prematurely
complete the same record before the compositor reports it as presented.

In throughput mode, a successful Vulkan present is recorded during painting
and completed in `after-paint`. The generic draw-context wrapper must finish
marking the frame outstanding first; completing inside the Vulkan backend's
`end_frame` reopens a completed timing record and produces a warning per frame.
Results sharing a clock/frame are coalesced, without per-frame allocations or
invented presentation timestamps. A scheduler regression test exercises both
success and failure through the real outer-wrapper ordering.

`enhanced-frame-policy` is headless: arithmetic at every offered FPS, invalid
values, overflow, stall recovery, real main-loop clock lifecycle, idle/stop
cancellation and input flushing during a ceiling. These are scheduler tests,
not GUI tests or presentation benchmarks. Compare exported symbols and installed
public headers against the exact upstream tag; record tooling limitations.
For rollback, clear the object-data value or run the stock runtime. Consumer
builds and the isolated fork installation must remain separate from system GTK.
