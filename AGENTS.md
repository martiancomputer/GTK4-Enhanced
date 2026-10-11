# AGENTS.md — GTK4-Enhanced

> Repository-wide instructions for coding agents and contributors. Read this file **before** editing source, build files, tests, or documentation. These are project contribution requirements, not a substitute for the user's explicit request or the licenses of upstream dependencies.

## 1. Project mandate

GTK4-Enhanced is a **compatibility-preserving, extensible fork of GTK 4**. Its first consumer is SpotifyGTK, but improvements must be generally applicable to GTK4 applications. The fork exists to add capabilities that upstream GTK lacks or does not expose portably, while retaining ordinary GTK behavior by default.

**Product areas:** (1) accurate, low-jitter, high-refresh rendering; (2) window transparency and native/compositor backdrops; (3) optional visual effects and animation controls; (4) platform integration and renderer improvements; and (5) measurable performance, power, and resource-efficiency work. These areas are independent feature tracks, not a commitment to implement everything immediately.

**Initial priority:** frame timing and 120/144 Hz validation. Separately, establish a sound capability/opt-in architecture so per-window alpha, background blur, Windows Mica/Acrylic, and future effects can be added without breaking existing applications. High-refresh targets of 240 and 360 Hz remain experimental until measured on capable hardware.

**Non-goals:** rewriting GTK in another language, replacing the widget system wholesale, implementing SpotifyGTK-specific features inside GTK, claiming to bypass compositor/OS limitations, indiscriminately enabling effects for existing apps, chasing an FPS counter at the expense of latency or power, and making unmeasured claims.

Upstream locations:
- Canonical: https://gitlab.gnome.org/GNOME/gtk
- Read-only GitHub mirror: https://github.com/GNOME/gtk
- Stable baseline for this fork: **GTK 4.24.1** (tag `4.24.1`), maintained against the `gtk-4-24` series unless the maintainer approves an upgrade.

Never silently switch the baseline to upstream `main` or combine code from a development series without reviewing API, dependency, and ABI changes. Record upstream commit/tag information in every rebase or version-upgrade PR.

## 2. Agent operating rules (mandatory)

1. **Inspect before editing.** Read affected code, relevant upstream history, Meson options, tests, and any existing compatibility notes. Reproduce the defect or collect baseline traces when possible.
2. **Write a short implementation plan** for any behavior, timing, ABI, packaging, or architecture change. State the hypothesis, touched components, verification method, and rollback path.
3. **Prefer the smallest correct patch.** Keep functionality in the responsible GTK/GDK/GSK subsystem. No application-specific hacks, mass refactors, or unrelated formatting changes.
4. **Do not invent evidence.** Never claim a display is running at 120/240/360 FPS based only on a configured interval or a tick callback count.
5. **Preserve a working fallback.** Unsupported, unknown, and headless environments must behave like stock GTK wherever practicable.
6. **Do not push, force-push, rewrite history, publish releases, change submodules, or install to system paths without explicit authorization.** Do not delete unrelated user work. Review `git status --short` before and after changes.
7. **Run the relevant tests.** Report exact commands, environment, failures, and untested scenarios. Never say tests passed if they were not run.
8. **Stop and request maintainer approval** for a proposed public ABI/API break, SONAME change, dependency floor increase, new external dependency, license change, renderer replacement, or a patch with substantial cross-platform behavior risk.

Before a change, identify which gate it targets: **correctness**, **compatibility**, **performance**, **visual effects**, **platform support**, or **packaging**. A patch should not silently weaken another gate. For a requested feature outside the present roadmap, make a short proposal under the extensibility policy below; do not reject it simply because it is not a frame-rate feature.

## 3. Compatibility and ABI contract (highest priority)

The default deliverable is a **drop-in compatible GTK4 build**, not a parallel toolkit API.

**MUST preserve without explicit maintainer approval:**
- Existing exported symbols, symbol versions where applicable, calling conventions, public function signatures, and exported type names.
- GObject type identities, properties, signal names and signatures, enum/flag numeric values, and public struct layouts.
- GTK/GDK/GSK public header contracts, GIR/typelib namespace identities, plugin/module expectations, and the existing `gtk4` pkg-config dependency interface.
- Upstream shared-library identity and ABI/SONAME/DLL naming for a compatible build. An enhanced binary must not claim compatibility unless verified.
- Existing event semantics, frame-clock phase ordering, input behavior, accessibility, localization, styling/CSS, clipboard, text rendering, DPI scaling, and rendering fallbacks.
- Source compatibility for applications using public GTK 4.24 APIs; builds should work with stock libadwaita and the expected GTK dependency stack.

**MUST NOT:**
- Rename `libgtk-4`/GTK public symbols, fork the namespace, change public headers for an internal optimization, or expose private data layouts as new public API without approval.
- Change version strings or advertised capabilities deceptively. Identify the enhanced build separately through package metadata/build provenance, not by falsely reporting a newer upstream GTK version.
- Depend on an app-defined callback, hard-code SpotifyGTK paths, or require consumers to patch their GTK calls.
- Treat a successful compile as proof of binary compatibility.

Prefer changes confined to private implementation details. If the feature needs application-facing controls, propose a **strictly additive, opt-in, namespaced and versioned** interface rather than modifying existing GTK contracts. A fork-only environment flag is acceptable for prototyping, but not a substitute for a reviewed per-window API when applications need to select effects independently.

**Important distinction:** adding exports may preserve compatibility for *existing* applications but creates a new dependency for applications that use them. Fork-only APIs must be documented as optional and capability-checked; an application using them is not automatically binary-compatible with stock GTK. Never claim otherwise. Consider a separately distributed optional helper/integration library if it avoids changing GTK's public ABI altogether.

**Compatibility validation:** compare exported symbols and relevant ABI descriptions with the exact upstream base, run upstream tests, and compile/run an unmodified GTK4 consumer (SpotifyGTK is a useful integration test). On Linux use `abidiff` or equivalent when available; on Windows compare DLL export tables and test runtime module loading. Record tooling limitations rather than skipping the compatibility conclusion.

## 4. Engineering principles for high-refresh rendering

1. **Measure the real pipeline:** input/event delivery -> frame-clock request -> layout -> snapshot -> renderer submission -> compositor/presentation. Locate the bottleneck before tuning.
2. **Use physical display information when trustworthy.** Monitor-aware refresh detection must handle unavailable/zero reports, multi-monitor moves, hotplug, DPI changes, remote sessions, and refresh-rate changes.
3. **Handle units precisely:** nanoseconds, microseconds, and millihertz must never be mixed. Use monotonic timestamps and overflow-safe arithmetic.
4. **Preserve frame-clock invariants:** monotonic timestamps, correct phase sequence, one coherent update per frame, resynchronization after stalls, and no busy loops.
5. **Respect presentation pacing.** A nominal 360 Hz timer does not guarantee 360 presented frames. Do not disable vsync or force tearing by default. Diagnose OS compositor, GPU driver, renderer, and monitor limitations separately.
6. **Remain idle when idle.** No permanent high-frequency timers, polling loops, unnecessary frame requests, or elevated system timer resolution without a documented need and lifetime.
7. **Keep CPU/GPU/power costs bounded.** Under load, skip/coalesce missed frames appropriately rather than accumulating an unbounded backlog.
8. **Support stock rendering paths:** Vulkan, OpenGL, and Cairo must remain functional; accelerated paths must have safe fallback behavior. Avoid regressions to X11, Wayland, macOS, or headless builds while improving Win32.
9. **Do not degrade user input to inflate FPS.** Preserve scrolling, keyboard repeat, pointer responsiveness, animation timing, and latency.
10. **Prefer correct upstreamable fixes** over hardcoded frame periods. A fixed 60 Hz constant is a candidate for investigation, not proof of the sole bottleneck.

Relevant source areas (verify for the pinned upstream revision before editing):
- `gdk/gdkframeclockidle.c`, `gdk/gdkframeclock.c`, `gdk/gdkframetimings.c`
- `gdk/win32/`, including monitor, surface, and Vulkan/GL context implementations
- `gsk/` for renderer/synchronization and frame submission
- `gtk/` only when a measurable widget/snapshot/layout issue requires it
- `tests/`, `testsuite/`, and `tests/frame-stats.c` for regression and performance tests

## 5. Transparency, materials, and extensible visual effects

This is a core product track, **not** an incidental GTK theme tweak. Implement it conservatively and provide clear platform capabilities rather than pretending every compositor can render the same effect.

**Terminology — agents MUST distinguish:**
- **Opacity/translucency:** alpha compositing of the application's own surface or widget contents. GTK4 already has RGBA surfaces and CSS opacity capabilities; audit existing support before introducing new APIs.
- **Background blur / blur-behind:** the desktop, windows, or compositor content *behind* an application is blurred. This requires platform/compositor support, and cannot generally be simulated by blurring the app's own widgets.
- **In-window backdrop filters:** blurring/tinting GTK-owned content behind another GTK widget. This is an application-rendering operation, not the same as blur-behind the application window.
- **Materials:** Windows Mica, Desktop Acrylic, tint/frosted glass, etc.; visually different implementations are not interchangeable. Do not label a plain transparent color as native Acrylic.

**Permitted feature candidates (only after written design and tests):**
1. Accurate per-window/per-region alpha compositing and translucent, rounded toplevel surfaces.
2. Blur-behind requests on compositors that implement suitable protocols; graceful opaque/translucent fallback elsewhere.
3. Windows 11 native system backdrops, including Mica, Mica Alt, and Desktop Acrylic where the OS supports them.
4. Configurable tint, blur region, rounded clipping, and effective material scope, while retaining correct hit testing and input regions.
5. Optional in-app blur, shaders, shadows, transitions, animation controls, and other effect proposals **only** when they are reusable GTK capabilities.
6. GPU-aware implementation, minimal overdraw, damage-based updates and reduced-motion/high-contrast behavior.

**Platform rules:**
- **Windows:** prefer stable, documented DWM APIs and system backdrop types; check the actual Windows build and capability. `DWMWA_SYSTEMBACKDROP_TYPE` is supported from Windows 11 build 22621, and the OS chooses the specific material. Do not rely by default on undocumented window composition APIs or assume the same effect exists on Windows 10. Test native CSD, maximized windows, resizing, DPI changes, and MSIX.
- **Wayland:** use advertised compositor protocols (for example `ext-background-effect-v1` where present) only after version/capability negotiation; test protocol removal/reconfiguration, surface lifecycle, blur regions, fractional scaling, and fallback. The protocol is still evolving; do not assume all Wayland desktops support it. Avoid making one compositor's private extension a global GTK dependency.
- **X11:** alpha compositing depends on a running compositor; blur-behind is window-manager-specific. Prefer feature detection and a safe fallback; document any X11-specific protocol use.
- **macOS and other backends:** preserve their existing behavior. New native materials need a separate, reviewed platform implementation; never insert Windows or Wayland assumptions in platform-independent code.

**Default behavior and user control:**
- All novel backdrops/materials must be **disabled by default** and should only activate after an app requests them explicitly. No implicit opt-in based on dark theme, renderer choice, or GPU type.
- Prefer capability queries that distinguish `unsupported`, `available`, `requested`, and `active`. A request that the compositor silently ignores must not be reported as an active effect.
- Specify precedence among OS accessibility settings (high contrast, reduced transparency, reduced motion), app choices, theme/CSS, and system compositor policy. Never defeat platform privacy/accessibility controls to force an effect.
- Provide legible contrast, keyboard/focus visibility, correct pointer hit-testing, unaffected text antialiasing, stable animation timing, and an **opaque fallback**. Do not blur sensitive content outside the compositor's authorized surface mechanisms.
- Do not force a global application opacity change when only one panel requests a translucent material. Avoid introducing excessive redraws or GPU readbacks for aesthetics.

**Architecture boundary:** backdrops belong primarily in GDK platform surface integrations and compositor negotiation; widget-local effects belong in GTK/GSK snapshot/render paths. Keep requests separate from how each backend implements them. Public API proposals need names, semantics, lifetime rules, default values, error/fallback behavior, versioning, and stock-GTK interoperability analysis. Additive interfaces still require explicit maintainer approval.

**Visual-effect PR acceptance:** include pixel/render comparisons on opaque and translucent backgrounds, dark/light themes, rounded edges, resize, window activation/deactivation, maximize/minimize, mixed DPI, screenshot/screencast behavior, accessibility modes, idle CPU/GPU cost, and an unsupported-environment test. Document when OS/compositor policy can suppress an effect. A pretty screenshot is not proof of compatibility.

**Future-feature procedure:** for every new capability not listed here, create a short design note with (a) user-visible problem, (b) what stock GTK already supports, (c) proposed smallest extension, (d) platform matrix/fallbacks, (e) API and ABI implications, (f) memory/latency/power budget, (g) tests and rollback, and (h) licensing/dependency impact. Put it on a named feature track; avoid speculative code. No feature is authorized solely because it appears in a brainstorming discussion.

## 6. Feature roadmap and sequencing

**Shared prerequisite: B0 — Baseline and compatibility harness.** Pin GTK 4.24.1; establish reproducible Windows/Linux builds, ABI checks, GTK demos, performance/visual diagnostics, isolation and rollback. No later milestone can ship without passing the baseline. Once B0 is established, individual feature tracks may proceed **independently**; do not block transparency work until 360 Hz is solved, or merge unrelated tracks into one PR.

| Track / milestone | Scope | Exit gate |
| --- | --- | --- |
| **R1 — Timing correctness** | Refresh detection, traceable frame clocks, fallback behavior, Win32 presentation-path analysis | Measured 60 Hz baseline remains correct; idle behavior and multi-monitor handling tested |
| **R2 — High refresh** | Minimal scheduler/presentation changes for 120/144 Hz | Reproducibly measured presentation on hardware that supports it; ABI tests pass |
| **R3 — Very high refresh** | 165/240 Hz, frame skips and pacing | Physical test where available; stable frame-time data, bounded CPU/GPU overhead |
| **R4 — Experimental 360 Hz** | 360 Hz presentation-path study and fixes, VRR/mixed refresh | Hardware-backed measurements or explicit unverified status; graceful lower-rate fallback |
| **V1 — Surface correctness** | Alpha compositing tests, per-window/per-region transparency, clipping | No opaque-default regressions; native input, sizing, accessibility correct |
| **V2 — Platform-native backdrops** | Windows 11 Mica/Acrylic, negotiated Wayland blur-behind, X11 fallback | Real supported-OS tests + unsupported fallback; no false capability reporting |
| **V3 — Optional visual effects** | In-window blur, tint, shadows, transitions, optional shader-based effects | APIs opt-in, measurable performance cost, reduced-motion/high-contrast behavior |
| **P1 — Packaging/integration** | CI, isolated runtime, DLL/SONAME verification, SpotifyGTK submodule consumer | Packaged app demonstrably loads enhanced GTK; stock-runtime rollback validated |

Security fixes, ABI regressions and platform correctness take precedence over new features. Each new feature track needs a written proposal and corresponding test coverage. Renderer replacements, public API additions, dependency floor changes or broad widget redesigns require explicit maintainer approval.

Do not enable 120/240/360 Hz or a named visual effect in a consuming app merely because its requested configuration value parses successfully. Mark it unsupported/experimental until actual presentation or visual implementation is verified on the target platform.

## 7. Measurements and acceptance criteria

Every frame-pacing performance PR should record:
- OS/build version, GTK upstream base and fork commit, CPU/GPU/driver, display connection, monitor model, refresh setting, scale, renderer, compositor, and relevant power mode.
- Test scene/workload, test duration, warm-up, exact commands, and whether the window was focused, animating, occluded, or idle.
- **Requested frames, produced/submitted frames, and presented frames when measurable** (do not conflate these numbers).
- Mean/median/p95/p99 frame interval, missed frames, long stalls, CPU/GPU utilization, idle activity, and memory use when relevant.
- A comparison against unmodified GTK on the **same machine under the same conditions**.

Correctness matters more than a favorable average. Report stutter, input latency, tearing, power consumption, and fallback behavior. If presentation timing cannot be observed directly, say so and qualify the result as scheduling/rendering data only.

At minimum test: 60 Hz baseline, one 120/144 Hz configuration, one unsupported/fallback case, rapid scroll/animation, idle, window resize, monitor move, and wake/resume. Run 240/360 Hz physical tests only when appropriate hardware is available; do not fabricate or emulate proof of real monitor presentation.

Never introduce test-only changes that secretly alter release behavior. Keep instrumentation compiled out or off by default unless its overhead is measured and accepted.

## 8. Build, packaging, and submodule requirements

GTK4-Enhanced must remain buildable through **upstream Meson/Ninja**; do not replace the build system. Discover supported Meson options for the pinned tag (`meson configure`/`meson.options`) instead of copying options from another release branch.

Representative build flow (adjust dependencies and platform options to match the host):

```sh
meson setup _build --prefix="$PWD/_stage/usr" --buildtype=debugoptimized
meson compile -C _build
meson test -C _build --print-errorlogs
```

Never install a development fork globally as part of normal verification. Use an isolated prefix and make consumer resolution explicit via `PKG_CONFIG_PATH` and platform library paths. Check the executable's actual loaded libraries; seeing the fork in a submodule directory is **not** proof that it was linked or loaded.

For SpotifyGTK integration:
- This repository is the GTK4 source. **SpotifyGTK owns the `dependencies/gtk4-enhanced` submodule pointer** and must update it separately; do not commit changes to the parent repository from inside the fork unless requested.
- Build GTK4-Enhanced first into an isolated staging prefix; configure the consumer against that prefix, then bundle exactly those runtime libraries and dependent modules.
- MSYS2/UCRT64 builds and MSIX packages must not accidentally substitute system GTK DLLs for the fork. Check DLL origins, export compatibility, and runtime dependency closure. Beware stale artifacts and `cp -n`/no-clobber copy behavior.
- Never overwrite globally installed GTK, GLib, GDK, GSK, or libadwaita libraries to make a test pass.
- Preserve a stock GTK runtime route as a rollback and compare it on the same app build.

## 9. Required tests and CI gates

For each behavior-changing PR:
1. Compile the affected platform configurations (debug + release where available).
2. Run relevant unit and integration tests from `testsuite/` and `tests/`.
3. Validate ABI/API compatibility against the base tag; document any exports or metadata differences.
4. Exercise at least one ordinary GTK demo/test program and one representative consumer without application patches when feasible.
5. Verify low-refresh behavior and idle CPU/power before promoting high-refresh performance changes.
6. Check affected rendering backends and platform fallbacks; if hardware is unavailable, explicitly mark that validation **not run**.
7. Run packaging tests for any library loading, dependency, Meson install, or Windows DLL change.
8. For material/transparency changes, run the visual-effect acceptance matrix in Section 5, with evidence of requested vs active behavior.

**Never disable tests, suppress diagnostics, or loosen failure conditions solely to obtain a green CI result.** Platform-specific skips need a stated technical reason and a tracking issue. New timing algorithms require focused automated tests for unit conversions, zero/invalid refresh, clock monotonicity, overrun recovery, and monitor transitions where testable.

## 10. Contributions and pull requests

Contributors should start from the correct `enhanced-4.24` development branch or the current documented integration branch. Use short, focused commits with descriptive messages; preserve attribution and include an upstream commit reference when backporting.

A PR description **MUST** contain:

```text
Summary:
Problem and reproduction:
Root-cause hypothesis:
Affected GTK/GDK/GSK paths:
Upstream baseline/commit:
Public ABI/API changed? (yes/no, with evidence):
Behavior changes / fallback:
Benchmarks or visual evidence: environment, procedure, stock vs enhanced:
Feature track / milestone and capability matrix:
Effects requested vs confirmed active (if applicable):
Accessibility (contrast, reduced motion/transparency) and fallback:
Tests run (exact commands + results):
Tests not run and why:
Packaging impact:
Rollback procedure:
```

- One independent concern per PR. Avoid mixing scheduler work with rendering refactors, cleanup, dependency upgrades, and documentation churn.
- Accompany bug fixes with regression tests where possible; accompany optimization with before/after data; accompany new effects with screenshots/video **and** platform/fallback tests.
- Follow upstream formatting and coding conventions. Preserve existing comments, copyright notices, and useful history.
- Avoid adding dependencies without proving necessity and receiving approval.
- Never commit credentials, private logs, generated build trees, package caches, SDK binaries, or machine-specific files.
- Security-sensitive reports should not be published with working exploit details before coordinated handling; follow applicable upstream/private disclosure channels.
- Maintain a small, clearly documented patch series suitable for periodic rebasing onto GTK stable maintenance releases.

## 11. License and upstream obligations

GTK is distributed under **LGPL-2.1-or-later** (subject to component-specific notices). Preserve its original license/copyright headers and upstream authorship. Distribute modifications and corresponding source in compliance with the applicable licenses, including when distributing DLLs inside an MSIX or another bundled application.

Do not relicense third-party code, remove notices, or copy code of uncertain provenance. Any added third-party material requires its license, provenance, and redistribution obligations to be documented. GTK4-Enhanced is an independent fork, **not an official GNOME/GTK release**.

## 12. Definition of done and final agent report

A change is done only when: the original problem is addressed; the patch is minimal and reviewed; appropriate tests run; ABI/API and fallback impact is understood; performance claims have reproducible evidence; and relevant documentation is updated.

At the end of a task, report in this order:
1. **Changed:** files and observable behavior.
2. **Verified:** exact test/build/benchmark results.
3. **Compatibility:** ABI/API, platforms, fallbacks, and package loading.
4. **Unverified:** hardware, presentation, OS/compositor effects, or scenarios not tested.
5. **Remaining risks:** known regressions, upstream drift, and recommended next steps.

**Default decision rule:** when visual features or performance conflict with compatibility, correctness, accessibility or safety, preserve the latter and propose an explicit experimental opt-in. Never silently change how stock GTK applications render to reach an FPS or aesthetics target.
