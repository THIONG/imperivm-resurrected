# Credits

## Contributors

- **iiTzSenn** — https://github.com/iiTzSenn
- **Villoh** — https://github.com/Villoh

## Built on

- **Syncrash** — AlvaroPeyleth (Discord: xtalvarotx), MIT license. https://github.com/AlvaroPeyleth/Imperivm-Syncrash
  The crash protection in `src/hooks/tweaks.cpp` (`InstallCrashFix`) comes from their research: the three sites of the script
  object lists (`0055f500`, `0057177c`, `005717f6`) and the piece that checks `IsKindOf("CVXMapObj")`
  before the virtual call. Here it is applied in memory at startup instead of modifying `gbr.exe`.
  Their analysis of the voices (paths missing from the language packs, including English) was used to check and
  complete the voices fix in `src/mods/vfs.cpp`: it matches 100 % in Spanish and Italian, and English was added.
- **ImperivmVoicesPatch** — Upercat. https://github.com/Upercat/ImperivmVoicesPatch (reference on the voices).
