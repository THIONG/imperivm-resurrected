# Adventure template

A single-player adventure or campaign: great battles, challenges, missions with a story and objectives.

## Steps

1. Copy this folder into `<game folder>\mods\` and rename it.
2. In `mod.json` change `id` (lowercase letters, numbers and hyphens: it identifies your mod), `name`, `authors` and `description`.
3. In `tags` write the language of the texts of your adventure (`"Language: English"`, `Spanish`, `Italian`, `German`, `French`, `Polish` or `Multilingual`).
4. Save your adventure in the editor and put the `.bfhp` in `adventures\` together with its image: a `.bmp` with exactly the same name.
5. Cutscenes are optional. Put each `.avi` in `movies\` and play it from the map's script with `PlayMovie(Translate("movies\\MyVideo.avi"))`. That is the form the game's own adventures use.
6. Delete the notes and the folders you do not use. Enable the mod in your profile and find the adventure in the game's menu.

## Folder layout

```
my-adventure\
  mod.json
  README.md
  adventures\
    MyAdventure.bfhp
    MyAdventure.bmp
  movies\
    MyVideo.avi
```

The game's own campaigns follow this layout: for example `Adventures\GreatBattles\1_Great_Battles_Zama.BFHP` and `.bmp`, and the matching `Movies\1_Intro_Great_B_Zama.avi` and `1_Extro_Great_B_Zama.avi` (intro and outro of each mission).

## Notes

- Videos are looked up in the active mods first, in load order, so a mod can also replace one of the game's.

## Check and publish

```
node tools\validate-mod.js <your mod folder>
```

The validator reports problems with file and line without opening the game ([Validating a mod](../../validating.md)). Zip the mod folder with its `mod.json` inside and publish it as described in [Publishing on GameBanana](../../publishing.md).
