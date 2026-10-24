# Qt GIF reader for Orbit

The Dolphin Windows Qt subset disables GIF decoding. Orbit builds the official
Qt 6.8.3 GIF image-format plugin against the same bundled Qt version, so QMovie
can display animated game backgrounds. No Qt DLL is replaced.

`main.cpp`, `qgifhandler.cpp`, `qgifhandler_p.h` and `gif.json` are unmodified
files from The Qt Company Ltd. and the Qt contributors:

https://github.com/qt/qtbase/tree/v6.8.3/src/plugins/imageformats/gif

Their original copyright and license choices remain in the source headers.
This distribution uses the GPL-3.0-only option; its text is in
`LICENSES/GPL-3.0-only.txt`. The plugin and its complete source are distributed
with Orbit. The CMake integration was written by Codex (OpenAI).
The build copies these notices to `Binaries/QtGif-Licenses`, independently of
Dolphin's `Licenses` directory, which is recreated after each executable link.

SHA-256 of the upstream files:

| File | SHA-256 |
| --- | --- |
| qgifhandler.cpp | 00080d5f7e561c03787311ac7892cadd1576b6fa63c78b1896907ed75726d5ad |
| qgifhandler_p.h | 0044619e68942391ff5e28a2bf78943a85ccd4957899c48525ed13109593d4df |
| main.cpp | 6ae3da17c8ab9f2af04251a279431cd18634908c485afb00ec5ed559547bd26f |
| gif.json | 79e37742235acbc27daed937cef58a84d3c2669d3dfec5388523582922e48131 |
