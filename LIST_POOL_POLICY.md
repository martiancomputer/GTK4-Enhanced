# Experimental retained-list pool (performance)

Large virtual lists can spend more CPU restyling retained offscreen widgets
than processing visible rows. This experiment permits a consumer to request
a smaller retained pool without changing upstream behavior by default.

On the GTK main context, a GtkListView advertises integer pointer value 1 in
GObject data named `gtk4-enhanced-list-pool-version`. A consumer may set
`gtk4-enhanced-list-pool-budget-v1` to an integer pointer value from 32 to 512,
then queue allocation. Absent or invalid requests retain the upstream pool.
The applied size is available as `gtk4-enhanced-list-pool-applied-v1`.

The requested budget is a floor: allocation retains at least three viewports
of the smallest positive measured row. Dense lists and fractional interface
scales therefore grow the pool to cover the actual viewport. Unknown geometry
uses the upstream default. Existing focus/selection trackers and their extra
rows remain intact. No timers, app callbacks or decoded-image storage are added.

This is an experimental private object-data protocol, not installed public API.
There are no exported-symbol, SONAME, GObject property or public-header changes.
Stock GTK ignores the request; consumers must capability-check. The arithmetic
test is headless. Real input, focus, resizing and platform performance need
consumer validation before promotion. Clear the request and queue allocation,
or use stock GTK, to roll back. Modified sources remain LGPL-2.1-or-later.
