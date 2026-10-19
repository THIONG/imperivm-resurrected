# Conquest template

A conquest: a map of territories (the game's own is the Mediterranean) with its own folder of images, listed in the game's menu as a conquest.

## Steps

1. Copy this folder into `<game folder>\mods\` and rename it.
2. In `mod.json` change `id` (lowercase letters, numbers and hyphens: it identifies your mod), `name`, `authors` and `description`.
3. Make your conquest in the editor and put its `.bfhp` in `conquests\`.
4. The conquest map is a folder in `conquestmaps\`. Its name must be **exactly** the one stored inside your conquest's `.bfhp` (the game's own conquest stores `ConquestMaps/1 - GBR Europe`). Rename `MY-CONQUEST-MAP` to it and change `data=` in `territories.xml` to the same name.
5. Put the images in that folder (list below) and edit `territories.xml`.
6. Cutscenes are optional, in `movies\`, played from the map's script with `PlayMovie(Translate("movies\\MyVideo.avi"))`.
7. Delete the notes and the folders you do not use. Enable the mod in your profile and look for the conquest in the game's menu.

## Folder layout

```
my-conquest\
  mod.json
  README.md
  conquests\
    MyConquest.bfhp
  conquestmaps\
    MY-CONQUEST-MAP\
      territories.xml
      territories.bmp
      global.bmp
      ShieldArrow.bmp
      ShieldBritain.bmp
      ShieldCarthage.bmp
      ShieldEgypt.bmp
      ShieldGaul.bmp
      ShieldGermany.bmp
      ShieldItaly.bmp
      ShieldSpain.bmp
  movies\
    MyVideo.avi
```

## The files of the conquest map

These are the files of the game's own conquest (`ConquestMaps\1 - GBR europe\`), with the sizes it uses:

| File | What it is |
|---|---|
| `territories.xml` | The territories. Each one has an `id`, an `index`, a `state` and its `neighbours`. The ids of the game's conquest are `Spain`, `Britain`, `Gaul`, `Italy`, `Carthage`, `Egypt` and `Germany`. The file in this template has three of them as an example; if in doubt about `index` and `state`, copy the game's file. |
| `territories.bmp` | 1024x768, 8-bit (256 colors). |
| `global.bmp` | 1024x768, 16-bit: the picture of the map. |
| `Shield<Name>.bmp` | 74x79, 16-bit: one shield per territory (`ShieldSpain.bmp` for `Spain`), plus `ShieldArrow.bmp`. |

The game's own files are in `<game folder>\ConquestMaps\1 - GBR europe`: open them in an image editor to see what each one holds.

## Notes

- The conquest's `.bfhp` and everything in `conquestmaps\` count as game content: every player in the room needs the same mod.
- Videos are looked up in the active mods first, in load order.

## Check and publish

```
node tools\validate-mod.js <your mod folder>
```

The validator reports problems with file and line without opening the game ([Validating a mod](../../validating.md)). Zip the mod folder with its `mod.json` inside and publish it as described in [Publishing on GameBanana](../../publishing.md).
