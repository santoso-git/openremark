# Attribution

- `src/inserter.inc` derives from [Zen-Ink/xovi-content-inserter](https://github.com/Zen-Ink/xovi-content-inserter), commit `309519ddeaa1b279fd60fcfec31cf89e553df844`, copyright © 2026 NFJ / Nuanfengjia, GPL-3.0-only. Changes: removed constructor diagnostics, limited exported API, added cancellation/page identity guard before each ink run. Original upstream native insertion code remains in the source but is not exported by this plugin.
- The DocumentView QMD selector comes from [rmitchellscott/xovi-qmd-extensions](https://github.com/rmitchellscott/xovi-qmd-extensions), `3.27/penLayerMemory.qmd`, copyright Mitchell Scott, GPL-3.0-only. The inserted control is new.
- `fonts/Caveat.ttf` comes from [Google Fonts / Caveat](https://github.com/google/fonts/tree/main/ofl/caveat), under SIL Open Font License 1.1; see `fonts/OFL.txt`.
- XOVI generated glue is produced by [asivery/xovi](https://github.com/asivery/xovi) commit `2b99649f5e4fd6288be7792a8570bd16418adb70`.
- Runtime dependency [rm-shot](https://github.com/rmitchellscott/rm-shot), GPL-3.0, installed separately through Vellum.

New code in this project is GPL-3.0-only. The bundled LICENSE retains the upstream attribution and full license text.

- Button icon (`icons/ask.svg`): a generic asterisk drawn for the remarkable-claude fork, not a brand logo. The upstream OpenAI icon (Simple Icons, CC0-1.0) was removed.

- DejaVu Sans Mono: bundled for code rendering under the Bitstream Vera font license; see `fonts/DejaVu-LICENSE.txt`.
