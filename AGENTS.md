# AGENTS.md

Rules for agents and contributors working on this repo. [CONTRIBUTING.md](CONTRIBUTING.md) has the full detail of the code rules, the tests and the SDK.

## Commits

Conventional Commits, in English, imperative mood, lowercase after the type, no trailing period.

```text
<type>(<scope>): <short summary>
```

- Types: `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `build`, `ci`, `chore`.
- Scopes (optional): the `src/` folder (`core`, `i18n`, `hooks`, `mods`, `game`, `net`, `ui`, `app`), `sdk`, `tools`, `ci`, `release`.
- Keep the summary under about 72 characters. Add a body only when the why isn't obvious.
- One logical change per commit. Generated files (`src/i18n/tr_data.cpp`) go in the same commit as their source.
- Breaking changes: `!` after the type or a `BREAKING CHANGE:` footer.

```text
fix(net): keep the rejoin slot after a drop
feat(sdk): add the .bfhp reader
ci: build winmm.dll on pull requests
```

## Pull requests

- Big features and fixes go through a pull request, from a branch off `main`. One thing per PR.
- Small changes (typos, docs, one-line fixes) can go straight to `main` as a commit.
- Issues and pull requests are written in English.
- The PR title follows Conventional Commits too (`fix(net): keep the rejoin slot after a drop`), since a squash merge of several commits uses it as the commit subject.
- Never mix moving code with changing what it does.
- In the description: what changes and why, how it was tried, and what was not tried.

## Issues

Not required. Bugs and requests are tracked on Discord, so a change doesn't need an issue first. Bug reports on GitHub use the `bug_report` template.

## Checks

Run before every commit:

```sh
tools\run-tests.bat
```

On Windows it runs everything the CI runs. Without Visual Studio, the Node checks still work: `gen-tr.js`, `check-addresses.js`, `check-layering.js`, `check-conventions.js`, `check-includes.js`, `check-logs.js`, `check-sdk.js` and `check-examples.js` in `tools/`. The syntax check and the unit tests need MSVC.

Passing them doesn't prove the game works. If the change affects what the player sees or does, say in the PR what was tried in-game. `src/net` can only be proved between two PCs on the same version.

## Code in `src/`

- No comments. The check fails on them; the reason goes in the PR or the commit body.
- Folders are ordered `addr`, `core`/`i18n`, `hooks`, `mods`, `game`, `net`, `ui`, `app`. A file only includes its own folder and the ones before it. Go up with a registered callback or function table.
- Addresses of `gbr.exe` only in `src/addr/*.h` as `kName`.
- Logs with `LogI/W/E/D("channel", ...)`, in English, never through `Tr()`.
- Player-facing text goes through `Tr("English text")` and into `src/i18n/translations.json` with `es` and `it`. Then run `node tools/gen-tr.js`.
- Tabs, LF, UTF-8 without BOM, `#pragma once` in headers.

## Generated and protected files

- `src/i18n/tr_data.cpp` is generated from `translations.json`. Don't edit it by hand.
- The SDK is English only and lives in `sdk/`. If a modder can do something new, add a line to `sdk/CHANGELOG.md`.
- Don't add files from the game, and don't attach them to issues or PRs.
- The Steamworks SDK and the signing keys (`*.pem`, `keys/`, `modio_key.h`) never go in the repo.

## Releases

Made by the maintainer only. Don't change the version number or touch `release-notes/` in a PR.
