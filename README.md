# GTK4-Enhanced

**GTK4, with more room to innovate.**

GTK4-Enhanced is an experimental, independently maintained fork of [GTK 4](https://www.gtk.org/) focused on smoother high-refresh rendering, native transparency and backdrop materials, and additional visual effects — **without breaking existing GTK4 applications**.

The first intended consumer is [SpotifyGTK](https://github.com/martiancomputer/SpotifyGTK), but enhancements are designed for GTK applications in general.

> **Project status: source baseline imported / experimental.** GTK **4.22.4** source, examples, demos, tests, and build support are present. No enhanced frame-clock or visual-effect implementation has been added yet. The capabilities below remain goals, not benchmarked features. There is no stable GTK4-Enhanced release.

The exact source commit is `7f99ab1a26408b6499a18f353f081e3c0598ea5c`.
See [UPSTREAM.md](UPSTREAM.md) for provenance, import boundaries, and licensing.
Upstream CI jobs and hosting templates were intentionally left out. CI and
consumer packaging come after a working local experiment.

## What we're building

| Area | Goals |
| --- | --- |
| **High-refresh rendering** | Correct frame scheduling and presentation pacing at **120 / 144 Hz**, then **240 / 360 Hz** where hardware and compositor support it. |
| **Transparency** | Reliable per-window and per-region alpha, translucent surfaces, and correct clipping and input behavior. |
| **Native materials** | Windows 11 **Mica / Acrylic** and compositor-supported blur-behind on Linux, with honest capability detection. |
| **Visual effects** | Optional in-window blur, tint, shadows, transitions, and shader-backed effects. |
| **Performance** | Lower frame-time jitter, efficient damage tracking, restrained CPU/GPU usage, and no needless redraws while idle. |

A configured frame interval is **not** proof of a presented frame rate. Effects must be verified on actual supported systems; unsupported effects must fall back cleanly.

## Compatibility first

GTK4-Enhanced aims to remain a **drop-in replacement for the corresponding upstream GTK4 release**:

- Preserve GTK/GDK/GSK public API, exported symbols, ABI, GObject contracts, library names, and `gtk4` pkg-config compatibility.
- Keep upstream appearance and behavior by default. New effects are **opt-in**, not globally forced.
- Favor internal fixes and capability detection over application-specific patches.
- Maintain usable fallbacks on Windows, Wayland, X11, and unsupported renderers or compositors.
- Do not make ordinary applications depend on fork-only APIs. If optional extensions are introduced, document their limits and stock-GTK fallbacks.

Compatibility is an **engineering requirement**, not yet a certification. Each release must be tested against its exact upstream base.

## Roadmap

| Milestone | Deliverable | Status |
| --- | --- | --- |
| **B0 — Foundation** | Import GTK 4.22.4, reproducible builds, ABI comparison and benchmarks | Source imported; Linux build smoke check passed; broader compatibility validation pending |
| **R1 — Frame timing** | Monitor-aware scheduling, Win32 investigation, proper fallbacks | Planned |
| **R2 — High refresh** | Hardware-validated 120 / 144 Hz rendering | Planned |
| **R3–R4 — Experimental** | 165 / 240 / 360 Hz investigation and performance tuning | Planned |
| **V1 — Transparency** | Alpha surfaces, clipping, and input correctness | Planned |
| **V2 — Materials** | Windows backdrops and negotiated compositor blur | Planned |
| **V3 — Effects** | Additional optional visual effects | Planned |
| **P1 — Integration** | Isolated packages, CI, and SpotifyGTK submodule integration | Deferred until the local experiment works |

The rendering and visual-effects tracks can progress **independently** once B0 is complete. Measured correctness and compatibility take priority over a target FPS or a visual demo.

## Build from source

GTK4-Enhanced uses GTK's existing **Meson + Ninja** build system. The source
tree includes the supporting resources and dependency recipes, not prebuilt
GTK or a bundled system SDK.

For the initial Linux experiment, provide a C toolchain, Python 3, Meson
1.5 or newer, Ninja, pkg-config, Gettext, and development packages for GLib
2.84+, Pango 1.56+, Cairo 1.18.2+, GdkPixbuf, Graphene, HarfBuzz, FriBidi,
Epoxy, Fontconfig, image codecs, and the selected windowing backends. Wayland
needs wayland-client 1.24+ and wayland-protocols 1.44+; the default Linux build
also enables X11. Vulkan needs its headers/loader and `glslc`. The default
media backend needs GStreamer 1.24+ development libraries.

GLib's development **tools** are required as well as its runtime and headers:
`glib-mkenums`, `glib-genmarshal`, and `gdbus-codegen`. On distributions that
split these out, install the development-tool package separately. A Git-source
build also needs `sassc` and its LibSass runtime to compile the GTK theme.
Do not work around a missing tool by deleting source, tests, or resources.

From the source root:

```sh
git clone https://github.com/martiancomputer/GTK4-Enhanced.git
cd GTK4-Enhanced
meson setup _build --prefix="$PWD/_stage/usr" --libdir=lib \
  --buildtype=debugoptimized --wrap-mode=nofallback \
  -Ddocumentation=false -Dman-pages=false -Dintrospection=disabled \
  -Dbuild-demos=true -Dbuild-examples=true \
  -Dbuild-tests=true -Dbuild-testsuite=true
meson compile -C _build
```

This prototype configuration disables generated API documentation and
GIR/typelib generation, not the C API or upstream tests. Before distributing a
drop-in package, restore the expected introspection artifacts and match the
consumer's backend/feature configuration. `--wrap-mode=nofallback` makes missing
host dependencies explicit instead of silently downloading an additional SDK.
The upstream recipes remain in `subprojects/` for deliberate fallback builds.

Local-only tooling may be kept under ignored `_bootstrap/`. If a checkout has
an existing `_bootstrap/native.ini`, that machine file selects its local tools;
it is not part of the portable source baseline or a replacement for documenting
build dependencies. Builds configured with it can still be resumed with
`meson compile -C _build`. Do not commit that cache, native file, build logs,
or generated binaries.

Everything builds locally; these commands do **not** replace system GTK.
For local-only staging, run `meson install -C _build` after a successful build;
the prefix above points inside the checkout. Never use `sudo` or install this
experimental fork into `/usr` or `/usr/local`.

The build retains upstream MSYS2/UCRT64 support, but the first experiment is
Linux-only. Windows builds and packaging are not validated by a Linux compile.
Consult the pinned [upstream contribution instructions](https://github.com/GNOME/gtk/blob/4.22.4/CONTRIBUTING.md)
for additional platform prerequisites.

### Source map and experiment programs

| Location | Purpose |
| --- | --- |
| `gdk/` | Frame clocks, input, surfaces, monitor information, and platform backends |
| `gsk/` | Vulkan/OpenGL/Cairo rendering, render nodes, GPU submission, and effects |
| `gtk/` | Widgets, layout, snapshots, themes, accessibility, and resources |
| `examples/` | Small GTK applications and progressive application examples |
| `demos/` | GTK demo, widget factory, node editor, and other demonstration tools |
| `tests/` | Interactive diagnostics, including `scrolling-performance`, `animated-resizing`, and `frame-stats.c` |
| `testsuite/` | Automated regression tests and their fixtures |
| `build-aux/`, `subprojects/`, `po/`, `docs/`, `tools/` | Build helpers, dependency recipes, translations, reference sources, and GTK tools |

Useful programs after building (start them manually when needed):

```sh
_build/demos/gtk-demo/gtk4-demo
_build/demos/widget-factory/gtk4-widget-factory
_build/demos/node-editor/gtk4-node-editor
_build/examples/window-default
_build/tests/scrolling-performance
```

Build-tree programs use the local library through their build RPATH. Inspect
the actual loaded library before comparing performance against stock GTK;
program names alone do not establish which runtime is in use. Do not run
automated UI interactions to validate the reported SpotifyGTK scrolling issue.

List available tests with `meson test -C _build --list`. Run relevant headless
logic tests first; display/rendering tests need a suitable isolated display and
explicit manual validation of the actual regression. A full upstream run,
when appropriate, is `meson test -C _build --print-errorlogs`. Do not interpret
a compile or a small test subset as ABI or high-refresh certification.

### Baseline verification

The initial Linux smoke check (2026-10-11) configured the pinned source with
Wayland, X11, Vulkan, examples, demos, and tests enabled. It successfully built
the shared GTK library, all 17 example applications, GTK Demo, Widget Factory,
Node Editor, and the `scrolling-performance`, `animated-resizing`, and
`testwidgettransforms` interactive diagnostics. These programs were compiled,
not launched or driven by automated UI input.

The following headless test command passed **5 tests / 73 subtests**, with no
failures:

```sh
env -u DISPLAY -u WAYLAND_DISPLAY meson test -C _build --no-rebuild \
  --print-errorlogs array rgba path-intersect curve-intersect half-float
```

`readelf` confirmed the upstream `libgtk-4.so.1` SONAME; the demo's build RPATH
and `ldd` resolution pointed to the local build-tree library. This checks
link resolution, not an unmodified consumer's live compatibility.

The build emitted warnings for newer host GLib/Pango enum values and demo
profile expressions. In a source import without Git metadata, the upstream
profile generator also prints a nonfatal Git lookup diagnostic. These were
not suppressed or patched out. The initial retained source was compared
byte-for-byte against the pinned commit. Before publication, inherited
developer-specific paths were sanitized in metadata, documentation examples,
a comment, and a test reference. The exact import and sanitization boundaries
are documented in [UPSTREAM.md](UPSTREAM.md).

The full upstream test suite, introspection artifacts, ABI comparison,
Windows build, consumer loading, and presentation/performance measurements
remain **unverified**. This smoke check does not complete the B0 exit gate.

### Using it with SpotifyGTK

SpotifyGTK will reference this repository as a **pinned Git submodule**. Its build process must explicitly link against the enhanced GTK build and bundle its runtime libraries where required (notably for Windows MSIX). A submodule alone does not make an application load the fork.

Until integration is validated, **standard GTK4 remains the supported fallback**.
This import does not change SpotifyGTK's submodule pointer, build, running
process, or system GTK. Keep independent stock and enhanced builds for rollback
and compare against the exact upstream base in [UPSTREAM.md](UPSTREAM.md).

## Contributing

Please read [AGENTS.md](AGENTS.md) before changing source or submitting a pull request. The same rules apply to human and automated contributors.

Keep changes focused. Include the upstream baseline, problem being solved, affected subsystems, ABI/API impact, tests, platform fallbacks, and **before/after measurements** for performance changes. New visual features need screenshots or recordings *and* unsupported-platform testing. Never report an untested refresh rate or native material as working.

Security and upstream correctness fixes take precedence over experimental features. Changes suitable for upstream GTK should be kept easy to isolate and propose upstream.

## Upstream and license

GTK4-Enhanced is **not an official GNOME or GTK release**.

- **Canonical upstream:** [GNOME/gtk on GitLab](https://gitlab.gnome.org/GNOME/gtk)
- **Upstream GitHub mirror:** [GNOME/gtk](https://github.com/GNOME/gtk)
- **Starting point:** GTK **4.22.4** (`4.22.4` tag)

GTK is licensed under **LGPL-2.1-or-later**, with component-specific notices.
Original attribution, license notices, and applicable source-distribution
obligations remain in effect. See [COPYING](COPYING), [AUTHORS](AUTHORS), and the
notices accompanying individual components.
