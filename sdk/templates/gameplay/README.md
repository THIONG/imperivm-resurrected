# Gameplay template

Rules, stats, costs, AI, victory modes, starting bonuses and game options. Patches change part of a game file without replacing it, so two mods can touch the same file.

## Steps

1. Copy this folder into `<game folder>\mods\` and rename it.
2. In `mod.json` change `id` (lowercase letters, numbers and hyphens: it identifies your mod), `name`, `authors` and `description`.
3. Put what you need in the folders below and delete the rest. The `patches\` folder is not in the template (a note file there would be reported as a bad patch): create it with the path of the game file you patch, for example `patches\data\classes\RHastatus.sc.xml`.
4. Enable the mod in your profile and restart the game.
5. Read `Logs\imperivm\imperivm-mod.log`, in the game folder: it lists what your mod loaded and its errors.

## Folder layout

```
my-gameplay-mod\
  mod.json
  README.md
  patches\
    data\
      classes\                a patch has the path and name of the game file it changes
      commands\
  events\                      OnGameStart.vs ...
  data\
    GameScripts\               victory modes
    BonusScripts\              starting bonuses
    mod\                       your own scripts
  options\                     lobby switches
```

## What goes where

| Folder | Guide |
|---|---|
| `patches\` | [Patches](../../patches.md) |
| `events\`, `data\mod\` | [Scripts](../../scripts.md) |
| `data\GameScripts\` | [Victory modes](../../victory-modes.md) |
| `data\BonusScripts\` | [Starting bonuses](../../bonuses.md) |
| `options\` | [Game options](../../options.md) |

## Notes

- This is gameplay: every player in the room needs the same mod.
- The finished examples are in [examples](../../examples): `patches-example`, `scripts-example`, `victory-example`, `options-example`.

## Check and publish

```
node tools\validate-mod.js <your mod folder>
```

The validator reports problems with file and line without opening the game ([Validating a mod](../../validating.md)). Zip the mod folder with its `mod.json` inside and publish it as described in [Publishing on GameBanana](../../publishing.md).
