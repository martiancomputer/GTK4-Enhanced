# Upstream source baseline

GTK4-Enhanced is based on GTK **4.24.1** source, with the import selection and
privacy sanitization described below. This is not a GNOME release or a compatibility
certification of the enhanced fork.

- Canonical repository: https://gitlab.gnome.org/GNOME/gtk
- Import transport: https://github.com/GNOME/gtk (GNOME's read-only mirror)
- Tag: `4.24.1`
- Annotated tag object: `02b85fda7f2078b601774bef3e5c0daddca12b2a`
- Source commit: `2ea935e3ec680fb122f245b28e18c3495df8c344`
- Maintenance series: `gtk-4-24`

The tag was resolved with `git ls-remote` and checked against the fetched
commit. No cryptographic tag-signature verification is claimed.

## Imported

The source at that commit is retained, including GTK, GDK, GSK, all platform
backends, Meson files, build helpers, resources, translations, dependency wrap
recipes, tools, API documentation sources, demos, examples, interactive test
programs, and the upstream regression suite. `COPYING`, `AUTHORS`, and
component-specific notices remain intact. Keeping these supporting trees
avoids an artificially stripped GTK build and preserves upstream test inputs.

## Deliberately omitted or replaced

- Upstream top-level `README.md`: replaced by GTK4-Enhanced's own README.
- `.gitlab-ci.yml` and `.gitlab-ci/`: CI is deferred until local experiments
  work; no replacement jobs are added yet.
- `.gitlab/`: upstream hosting/issue templates are not needed for the local
  experiment.
- Upstream Git history and `.git/`: this is a pinned source import, not a clone
  of GNOME's repository in the working directory.

The fork's existing `AGENTS.md` is preserved. `.gitignore` is extended for local
builds, staging, tooling, and private artifacts; upstream wrap recipes must
remain eligible for version control.

The initial import was GTK 4.22.4 (commit
`7f99ab1a26408b6499a18f353f081e3c0598ea5c`). The maintainer explicitly approved
upgrading to 4.24.1 rather than downgrading the consumer's libadwaita. The update
retains the fork's README, instructions, omitted CI boundary, and privacy edits.
Dependency floors follow upstream 4.24.1, including GLib 2.89.3 and Pango 1.58.

Fork behavior changes are confined to the private GDK frame clock and its
headless tests. See [FRAME_POLICY.md](FRAME_POLICY.md). No installed public
header, public API, or SONAME changes are intended. Compare against the exact
source commit above, not the original 4.22.4 import.

## Privacy sanitization

Before publishing, developer-specific paths inherited from upstream were
removed from SVG export metadata, documentation media examples, a Win32 code
comment, an Italian translation editor header, and a historical node-parser
error reference. Relative filenames or neutral placeholders replace them.
Copyright, license, contributor, and translator attribution remains intact.

Affected files: `demos/gtk-demo/gtk-logo.svg`, the three reference-documentation
copies of `gtk-logo.svg`, `docs/reference/gtk/images/media-controls.ui`,
`docs/reference/gtk/images/video.ui`, `gdk/win32/gdkdrop-win32.c`, `po/it.po`,
and `testsuite/gsk/nodeparser/text-font-options-errors.ref.node`. These narrow
sanitization changes are additional documented differences from the base tag;
they do not implement enhanced rendering behavior.

## Reproduce or compare

Fetch the pinned tag into a separate temporary checkout, verify its commit
against the value above, and compare the corresponding source trees. Exclude
the deliberately omitted/replaced files and local generated directories.
Do not extract over an existing fork or overwrite its README, instructions,
or implementation patches. Never install the comparison build over system GTK.

Upstream GTK is LGPL-2.1-or-later, subject to component-specific notices.
Retain source attribution and meet corresponding-source obligations when
distributing modified libraries.
