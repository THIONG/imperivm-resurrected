<p align="center">
  <img src="assets/banner.png" alt="Imperivm Resurrected" width="640">
</p>

<p align="center"><b>English</b> · <a href="README.es.md">Español</a> · <a href="README.it.md">Italiano</a></p>

# Imperivm Resurrected

Unofficial mod for **Imperivm: Great Battles of Rome HD** (Steam). It gives the game what it needed to stay alive:
online play with no port forwarding, one-click mods, rebindable controls and many fixes.

> **Status:** version 1.0. Tested in two-player online games; not yet tested with more players. If something fails, see [If something fails](#if-something-fails).

## What it brings

- **Online through Steam**, with no port forwarding or VPN: rooms, invitations to friends, chat and rejoining if someone drops.
- **One-click mods** from inside the game, with profiles, versions and dependencies. Mods are published on
  [GameBanana](https://gamebanana.com/games/26069).
- **Rebindable controls**: any key of the game and of the commands, with presets.
- **Modern display**: Direct3D 11, current resolutions, window or full screen without flicker, and the Steam overlay.
- **Modern sound**, autosave, a performance panel and fixes for crashes of the original game.
- **A problem report** built into the game, with private data hidden.
- **Updates itself**: the game tells you when there is a new version.

## Install

1. Download the zip from the [latest release](https://github.com/THIONG/imperivm-resurrected/releases/latest).
2. Extract it into the game folder, where `gbr.exe` is
   (in Steam: right-click the game → Manage → Browse local files).
3. Start the game from Steam.

To uninstall, delete `winmm.dll`, `steam_api.dll` and the `imperivm-resurrected` folder from the game folder. The mod does not change any game file.

You need your own copy of the game from Steam. This mod includes no game file.

## If something fails

In the game's main menu, click the heart at the bottom left and then **Create report**. It makes a zip with the logs and
your system, with your Windows user name, Steam IDs and IP addresses replaced. The mod sends nothing: you attach it yourself.

Then [open an issue](https://github.com/THIONG/imperivm-resurrected/issues/new/choose) with the zip, or ask on
[Discord](https://discord.gg/6kCatX3Zbc). The logs are in `Logs\imperivm\`, inside the game folder.

## Making mods

The **SDK** is in [`sdk/`](sdk): guides, tools and examples to make content without compiling anything, in English
([start here](sdk/README.md)). Each release attaches it as a zip.
Finished mods are published on [GameBanana](https://gamebanana.com/games/26069).

## Development

The mod's code is in `src/` (C++ for Windows, 32-bit), its tests in `tests/` and the checks in `tools/`.
How to build, what `tools\run-tests.bat` checks and how to send a change: [CONTRIBUTING.md](CONTRIBUTING.md).
Building the DLL needs Valve's Steamworks SDK, which is not in this repository.

Every release is checked by `.github/workflows/release.yml` (tag, hashes, signature and contents of the zip). You can run the
same check on a downloaded release with `node tools/verify-release.js <folder>`.

## Code signing policy

Free code signing provided by [SignPath.io](https://about.signpath.io), certificate by [SignPath Foundation](https://signpath.org).

- Committers and reviewers: [THIONG](https://github.com/THIONG), [iiTzSenn](https://github.com/iiTzSenn), [Villoh](https://github.com/Villoh).
- Approvers: [THIONG](https://github.com/THIONG), [iiTzSenn](https://github.com/iiTzSenn), [Villoh](https://github.com/Villoh).
- Privacy policy: [imperivmresurrected.com/privacy.html](https://www.imperivmresurrected.com/privacy.html)

## License

- **Code** (`src/`, `tests/`, `tools/`): GPL-3.0-or-later with an additional permission for Steamworks. See [LICENSE](LICENSE)
  and [ADDITIONAL-PERMISSION.md](ADDITIONAL-PERMISSION.md). Third-party software: [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).
  Credits: [CREDITS.md](CREDITS.md).
- **SDK** (`sdk/`): MIT for the code and CC BY-SA 4.0 for the guides. See [sdk/LICENSE.md](sdk/LICENSE.md).
- **Images** (`assets/`): all rights reserved. See [assets/LICENSE.md](assets/LICENSE.md).

---

Imperivm Resurrected is a fan project. It is not affiliated with or endorsed by FX Interactive or Haemimont Games.
Imperivm and Great Battles of Rome are trademarks of their respective owners.
