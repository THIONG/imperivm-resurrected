# Contributing

This repository holds the mod (`src/`, `tests/`, `tools/`) and its SDK (`sdk/`). The mod is GPL-3.0-or-later with an additional permission for Steamworks. The SDK is MIT for the code and CC BY-SA 4.0 for the guides. A contribution is published under the license of the part it changes, and by sending it you state that you have the right to do so.

Issues and pull requests must be written in English.

## Reporting a bug

Open an issue and include:

- the mod version (bottom right of the main menu);
- what you did, what you expected and what happened;
- the report the mod creates: click the heart at the bottom left of the main menu, then **Create report**. It makes a zip in `Logs\imperivm\reports\` with the logs of the last sessions, the system and the active content, with Steam IDs, IP addresses and your Windows user name replaced. Nothing is sent by the mod, so read it and attach it to the issue. If you cannot start the game, attach the files of `Logs\imperivm\` instead (`imperivm-mod.crashed.log` is the last session that ended badly);
- for online problems, whether you were host or guest and the version of the other player;
- whether it also happens with the Original profile, if you were using content packs.

Do not attach files of the game.

## Proposing a change

For anything bigger than a small fix, open an issue first. It is cheaper to agree on the approach before the code is written than after.

## Building

Windows, Visual Studio Build Tools 2022 (C++, x86) and Node.js 24 or newer. Building the DLL also needs the Steamworks SDK, which is Valve's and is not in this repository.

```
set STEAMWORKS_SDK=C:\path\to\steamworks_sdk
build.bat
```

The result is `build\winmm.dll`. To try it, close the game and copy it to the game folder. `CMakeLists.txt` builds the same DLL (`IMPERIVM_BUILD_DLL`) and the tests (`IMPERIVM_BUILD_TESTS`).

## Checking a change

```
tools\run-tests.bat
```

This needs neither the game nor the Steamworks SDK, and the CI runs the same steps on every push and pull request:

1. `gen-tr.js` regenerates `src/i18n/tr_data.cpp` from `translations.json`.
2. `check-addresses.js`, `check-layering.js`, `check-conventions.js` and `check-includes.js` check the source.
3. `check-sdk.js` and `check-examples.js` check the SDK: its links, and that every example and template validates.
4. `syntax-check.js` compiles every file that does not need the Steamworks headers (all of them if `STEAMWORKS_SDK` is set).
5. The unit tests in `tests/` are built with CMake and run with ctest.

Passing them does not show that the game still works. If the change affects what the player sees or does, start the game and say in the pull request what you tried. Code in `src/net` can only be proved between two PCs on the same version; say whether you could test it.

## Code in `src/`

The checks enforce most of this.

Folders are ordered from the bottom up: `addr`, `core` and `i18n`, `hooks`, `mods`, `game`, `net`, `ui`, `app`. A file includes headers from its own folder and the ones before it, never from the ones after. When a lower folder needs something from a higher one, such as showing a notice, asking the interface or reading the network state, the higher folder registers a function table or a callback at start-up (see `core/uiport.h`, `game/netlink.h` and `mods/features.h`). A new file goes in the lowest folder that already holds everything it includes.

Addresses of `gbr.exe` are written only in `src/addr/*.h`, as `inline constexpr uintptr_t kName = 0x00xxxxxx;`, and used as `addr::kName`. Name each one after the game's own function when it is known.

Log with `LogI("channel", "text %s", value)`, `LogW`, `LogE` or `LogD`, never with `Log("Channel: text")`. The channel comes from the list in `src/core/logchannels.h`, and the message is English, without a trailing newline and never through `Tr()`. Use `LogE` when a feature failed and the player is affected, `LogW` for something unexpected that was handled, `LogI` for what happened in the session and `LogD` for detail. `tools/check-logs.js` checks all of this.

There are no comments in `src/`; the check fails on them. The reason for a decision goes in the pull request, and the code is explained by its names.

Formatting: UTF-8 without BOM, LF line ends, tabs, no trailing whitespace. Headers start with `#pragma once` and do not use `using namespace`. File names are lowercase letters, digits and underscores, and no two `.cpp` files share a name anywhere in `src/`, because the object files go to one folder. Project includes are written `"folder/file.h"` from `src/`. Functions and types are PascalCase, constants are `kName`, and file-private variables are `g_name` inside an anonymous namespace. Each file includes the standard headers it uses; `node tools/check-includes.js --fix` adds the missing ones.

A module too big for one file is split into `<module>.cpp`, `<module>_<topic>.cpp` and a private `<module>_internal.h` in the namespace `<module>_impl`, which only the files of that module may include. What those files share lives in topic structs declared there (`g_chat.scroll`), not in loose globals.

Text is English. What the player reads goes through `Tr("English text")`, where the English text is the key. Add it to `src/i18n/translations.json` with its Spanish (`es`) and Italian (`it`) versions (`"="` means the same as English, and format markers such as `%d` must match), then run `node tools/gen-tr.js`; the CI fails if `tr_data.cpp` is stale. Log messages are English and never go through `Tr`. Spanish names kept for compatibility (old ini keys, folders, commands) live only in `src/core/legacy.cpp`.

Do not mix moving code with changing what it does in the same pull request.

## Tests

A test is `tests/test_<name>.cpp` that includes `check.h` (`CHECK(cond)`, `CHECK_EQ(actual, expected)`, `return test::Summary("name")`), plus one line in `CMakeLists.txt`: `imperivm_test(test_<name> <the src files it links>)`.

Each test links only the files it needs, so the logic worth testing has to live in files that depend on nothing from the game (no `MinHook.h`, no `*_internal.h`). When a function mixes game state with logic, move the logic into a function of its inputs and leave the original to gather the state. A test that needs `Log` defines an empty one. Tests write only in their own temporary folder.

## Changing the SDK

The SDK is in English only and lives directly in `sdk/`: guides, `tools/`, `examples/` and `templates/`.

1. Change the guide, the tool or the example.
2. Run `node tools/check-sdk.js` (links between guides, examples with their README) and `node tools/check-examples.js` (every example and template passes the validator).
3. If a modder can now do something new, add a line to `sdk/CHANGELOG.md`. The SDK version is its first `## x.y`.

## Pull requests

- Branch from `main`, and keep each pull request to one thing.
- Run `tools\run-tests.bat` first. The CI has to pass.
- Write commit messages with a short subject in the imperative (`Fix the rejoin slot after a drop`) and, when it is not obvious, a body that says why.
- In the description say what changes and why, how you tried it, and what you did not try.

Releases are made by the maintainer. The update is signed with a key that never leaves their computer, so a pull request does not change the version number.
