# Scenario template

A map for skirmish and multiplayer games, made in the game's map editor.

## Steps

1. Copy this folder into `<game folder>\mods\` and rename it.
2. In `mod.json` change `id` (lowercase letters, numbers and hyphens: it identifies your mod), `name`, `authors` and `description`.
3. In `tags` write how many players the map has: `"Players: 2"`, or a range such as `"Players: 2-4"`. Without it the map does not show when people filter by players.
4. Save your map in the editor as a skirmish map and put its `.bfhp` in `scenarios\`. Delete `PUT-YOUR-FILES-HERE.txt`.
5. Enable the mod in your profile (**Additional content** in the main menu), start a room and pick the map in the scenario list. The game draws the preview from the map itself: there is no image to provide.

## Folder layout

```
my-scenario\
  mod.json
  README.md
  scenarios\
    MyMap.bfhp
```

## Notes

- A map changes what happens in the game, so every player in the room needs the same one. With `"redistributable": true` the host can send it to players who do not have it.
- A map saved in the editor as an adventure or a conquest does not appear in the Scenarios list, even in this folder. The validator warns about it.
- If the map comes with its own music or texts, add `music\` or `loc\` as shown in the [full template](../full).

## Check and publish

```
node tools\validate-mod.js <your mod folder>
```

The validator reports problems with file and line without opening the game ([Validating a mod](../../validating.md)). Zip the mod folder with its `mod.json` inside and publish it as described in [Publishing on GameBanana](../../publishing.md).
