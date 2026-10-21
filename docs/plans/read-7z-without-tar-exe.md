# Plan: read .7z files without tar.exe

Status: implemented on branch `read-7z-without-tar-exe`, stacked on `read-rar-without-tar-exe` (pull request #20, still open). Rebase onto `main` once #20 is merged. See "Result" at the end.

## Problem

The content manager chooses the reader by the first bytes of the file (`Extract()` in `src/mods/downloads.cpp`): `.zip` goes to miniz, `.rar` to `core/rarfile` (libarchive), and everything else to the `tar.exe` of the system.

Per the maintainer, GameBanana accepts `zip`, `rar`, `7z`, `bnp` and `json` (not checked by me against GameBanana). So a mod author can publish a `.7z`, and today that depends on `tar.exe`:

- **Wine and Proton** have no `tar.exe`: the install fails with "this system can only install .zip and .rar files".
- **Windows 10** ships `bsdtar 3.5.2 - libarchive 3.5.2 zlib/1.2.5.f-ipp` (seen in a user report, build 19045.6466). That version string lists no liblzma. A 7z written with LZMA or LZMA2, which is what 7-Zip writes by default, needs it. With a libarchive 3.5.2 that I built without liblzma, `tar -t` fails with `LZMA codec is unsupported`. **Not checked with Microsoft's own binary**, so the failure on real Windows 10 is a strong suspicion, not a result.

## What is not known yet

1. What a `.bnp` and a `.json` upload are for this game, and whether the manager ever receives them. A grep of `src/` finds no handling of `.bnp`. Because the reader is chosen by content, a `.bnp` that is not a zip or a rar would end up in the `tar.exe` path and fail.
2. Which compression methods the real 7z files of the community use (LZMA, LZMA2, BCJ filters, PPMd, BZip2, Deflate, encryption). Sample the files in `community-mods` and on GameBanana before choosing what to support.
3. The license text of the liblzma sources of xz (latest release seen: 5.8.4). Read its `COPYING` before vendoring and add the real text to `THIRD-PARTY-NOTICES.md`.

Do 1 and 2 first. They decide the size of the work.

## Proposal

Add the 7z reader that libarchive already has (`archive_read_support_format_7zip.c`, 5074 lines) to `core/rarfile`, and vendor only the part of liblzma that decodes LZMA and LZMA2.

- The 7z reader calls the liblzma API (`lzma_*`), so the LZMA SDK cannot replace it without rewriting the reader.
- The reader also has code for BZip2 and Deflate behind `HAVE_BZLIB_H` and `HAVE_ZLIB_H`. Done in the end: both are cheap (miniz and bzip2).
- Rename `core/rarfile` to something that is not RAR-specific only if the 7z code goes in the same module; otherwise add `core/sevenzfile` next to it with the same shape (`Is…File`, `…Extract`, same limits, progress, path check and messages).
- Reuse `ZipEntryPath`, `ZipLimits`, `ZipProgress` and `winpath.h`, as `rarfile` does.
- Keep refusing links, special files and password protected archives.

## Steps

1. Answer unknowns 1 and 2. Write the result at the end of this file.
2. Branch from `main`. Vendor the liblzma decoder files (and a hand-written `config.h` for them) in `third_party/xz/`, unmodified, with their license in `THIRD-PARTY-NOTICES.md`.
3. Add `archive_read_support_format_7zip.c` to `third_party/libarchive/` and the macros it needs to `third_party/libarchive/config.h`.
4. Add the reader, call it from `Extract()`, add the messages to `translations.json` (es, it) and run `node tools/gen-tr.js`.
5. Tests in `tests/test_7z.cpp`, built like `test_rar`:
   - real 7z files made by 7-Zip (p7zip from nix is enough) with LZMA, LZMA2, BCJ and a solid archive, plus libarchive's own `test_read_format_7zip_*.7z.uu` files;
   - generated files with `../evil.txt`, `C:/`, `con.txt`, `:stream` names, the limits, cancel, truncated and damaged files, and paths over MAX_PATH, as in `test_rar`;
   - break the path check on purpose and check that the tests fail.
6. Measure the DLL size against `main` and against PR #20. The decoder is small, but write the number down.
7. Compare the output with `bsdtar` 3.8.10 (`diff -r`) on a large real 7z, and time it against `tar.exe`.
8. CI (MSVC) green. Install a `.7z` mod from GameBanana in the game under Proton, and ask the Windows 10 user from the report to try it with their `tar.exe` 3.5.2.

## Out of scope

- `.tar`, `.tar.gz`, `.tgz` and `.gz` (the manual file picker lists them). They still use `tar.exe`. gzip needs zlib or miniz's inflate; decide separately once it is known whether anyone uses them.
- Multi-volume archives and self-extracting `.exe`.
- Removing `tar.exe` entirely. It stays as the fallback for whatever the new readers do not recognize.

## Decisions for the maintainer

- Is it acceptable to refuse 7z files that use Zstd (they go to `tar.exe`)?
- Should one module read both RAR and 7z, or two modules with the same interface?
- Should `.bnp` and `.json` be handled by this work or by their own?

## Result

### Unknowns (step 1)

1. **`.bnp` and `.json`**: not answered. No file of those types in `community-mods`, and none among the GameBanana uploads (see 2). They still go to `tar.exe`, as before. Needs the maintainer.
2. **Methods in the wild**: on 2026-10-21 the GameBanana API (`Mod/Index` and `Sound/Index` for game 26069, then `ProfilePage` of each) returned 15 items and 15 files: 14 `.zip`, 1 `.rar`, **no `.7z`**. `community-mods` has 59 zip, 56 rar and no 7z mod (its four `.7Z` files are the game's own `paks`, LZMA2, one solid block). So no 7z mod exists today; what the reader supports comes from what 7-Zip writes by default (LZMA2), plus LZMA, PPMd, Delta and the BCJ filters, which cost nothing once liblzma and the PPMd code of RAR are in. Zstd is not read (only the 7-Zip-zstd fork writes it).
3. **License of liblzma**: 0BSD (`SPDX-License-Identifier: 0BSD` in all 55 vendored files); text in `THIRD-PARTY-NOTICES.md`.

### Decisions taken (maintainer can revert)

- One module for RAR and 7z: `core/rarfile` became `core/archivefile` (`IsArchiveFile`, `ArchiveExtract`). The reader is libarchive's, so a second module would have copied the same 170 lines. Move and behavior change should go in separate commits: the `git mv` plus the identifier rename first, the rest after.
- Deflate and BZip2 are built in (second commit): Deflate with miniz through a hand-written `zlib.h`, BZip2 with bzip2 1.0.8 in `third_party/bzip2/`. BCJ2 works with LZMA, LZMA2, Copy, Deflate and BZip2. Zstd is not built in (libzstd is large and only 7-Zip-zstd writes it). When the own reader answers "unsupported format or damaged file" (`ArchiveNotUnderstood`), `Extract()` tries `tar.exe` if it exists; any other answer (password, links, path outside, limits, cancel) is final. Without `tar.exe` the player sees the own reader's message. `tar.exe` stays as the last resort.
- `.bnp` and `.json` are odd formats (maintainer): not handled here; they go to `tar.exe` as before.

### What was done

- `third_party/bzip2/` (7 files, bzip2 1.0.8) and `third_party/libarchive/zlib.h` (miniz); with `HAVE_ZLIB_H` libarchive also takes `crc32` from miniz, and RAR reads the same.
- `third_party/xz/`: 20 decoder `.c` files and 35 headers of xz 5.8.4, unmodified, plus a hand-written `config.h`. `third_party/libarchive/`: `archive_read_support_format_7zip.c` added, `config.h` gets `HAVE_LZMA_H`.
- `CMakeLists.txt` and `build.bat`: both libraries have a `config.h`, so each is compiled seeing only its own folder.
- `tests/test_7z.cpp` (134 checks) and `tests/data/7z/` (11 files, 700 KB): 7-Zip 26.02 files with LZMA solid, LZMA2 non solid, BCJ, Delta, PPMd, BZip2, Deflate, password (data and headers), a symlink and a 20 MB file (BZip2 and Deflate extract; only the libarchive Zstd file is refused); five files from libarchive's test suite (BCJ+LZMA, ARM64+LZMA2, BCJ2+BZip2, Zstd, empty archive); generated files with `../evil.txt`, `C:/`, `/abs`, `con.txt`, `:stream`, MAX_PATH. With the path check removed on purpose 7 checks fail.

### Measurements

- `winmm.dll` (clang-cl x86, `tools/build-dll.sh`): `main` 5,317,120 bytes, PR #20 5,489,664, LZMA only 5,553,152, with Deflate and BZip2 5,579,776 (+90,112 over #20, +1.6%).
- Community Mod v12 repacked as 7z with 7-Zip (1018 MB; LZMA2 level 5, LZMA2 level 9 solid with a 64 MB dictionary, BZip2 and Deflate): `diff -r` against the RAR extraction shows no differences. Under Wine 11, mingw -O1: 11.4 s, against 10.8 s for `bsdtar` 3.8.10 on the same file.

### Not done

- MSVC build (`build.bat`, CI): not run here; only clang-cl and MinGW. The CI will say.
- Installing a `.7z` from GameBanana in the game under Proton (there is none; it needs a test upload) and the Windows 10 user with `tar.exe` 3.5.2.
- Self-extracting `.exe`, multi-volume, `.tar`/`.gz`: out of scope as planned.
