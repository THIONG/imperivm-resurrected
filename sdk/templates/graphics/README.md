# Graphics template

Textures, unit looks, buildings, icons and menu images. Each player can use their own: graphics never have to match between players.

## Steps

1. Copy this folder into `<game folder>\mods\` and rename it.
2. In `mod.json` change `id` (lowercase letters, numbers and hyphens: it identifies your mod), `name`, `authors` and `description`.
3. Put your files in the folder that matches what you change, with the same path and name as the game's file when you replace one. Delete the other folders.
4. Enable the mod in your profile and restart the game.

## Folder layout

```
my-graphics\
  mod.json
  README.md
  units\
    MyUnit\                   sprites (.rle) and MyUnit.ent.xml
  buildings\
    MyBuilding\
  gameres\
    icons\                    unit portraits (.bmp)
    cmdbar\Actions\          command buttons (.bmp)
```

## Where to start

- Sprites for a unit or a building from PNG files: [Custom graphics](../../graphics.md) (`convert-sprite.js`, `create-entity.js`).
- Buttons and icons from a PNG: [Custom images](../../images.md) (`convert-image.js`).
- A new unit that uses them: [New units](../../units.md) and the [units template](../units).

## Notes

- `.rle` files and images do not count as gameplay. The `.xml` and `.pass` files in `units\`, `buildings\` and `mapobjects\` do: if you change them, everyone in the room needs the mod.
- An `.rle` with the same name as one of the game's is ignored; the validator warns with `--game`.

## Check and publish

```
node tools\validate-mod.js <your mod folder>
```

The validator reports problems with file and line without opening the game ([Validating a mod](../../validating.md)). Zip the mod folder with its `mod.json` inside and publish it as described in [Publishing on GameBanana](../../publishing.md).
