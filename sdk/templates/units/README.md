# Units template

A new unit or hero: its class, the command that recruits it, its images and its texts.

## Steps

1. Copy this folder into `<game folder>\mods\` and rename it.
2. In `mod.json` change `id` (lowercase letters, numbers and hyphens: it identifies your mod), `name`, `authors` and `description`.
3. Create the unit from an existing one. From the folder of the SDK:

   ```
   node tools\create-unit.js <DATA folder> <your mod folder> MyUnit --base RHastatus --health 260 --damage 20 --armor 14 --building RBarracks --gold-cost 180 --time 9000 --key v --name "My unit"
   ```

   It writes the class, the command and the texts in the folders below. [New units](../../units.md) explains every option and where the DATA folder comes from.
4. Make the portrait and the button from PNG files with `tools\convert-image.js` ([Custom images](../../images.md)).
5. Delete the notes. Enable the mod in your profile and recruit the unit.

## Folder layout

```
my-unit\
  mod.json
  README.md
  data\
    classes\
      MyUnit.sc.xml
    commands\
      MyUnit.xml
  gameres\
    icons\
      MyUnit.bmp
    cmdbar\Actions\
      train MyUnit.bmp
  loc\
    spanish.loc.xml
```

## Going further

- Its own sprites instead of another unit's: [Custom graphics](../../graphics.md) and the [graphics template](../graphics) (`units\MyUnit\`).
- A new ability, special or item for it: [abilities](../../abilities.md), [specials](../../specials.md), [items](../../items.md).
- The attributes that do something and the ones that crash the game: [Unit stats](../../stats.md).

## Check and publish

```
node tools\validate-mod.js <your mod folder>
```

The validator reports problems with file and line without opening the game ([Validating a mod](../../validating.md)). Zip the mod folder with its `mod.json` inside and publish it as described in [Publishing on GameBanana](../../publishing.md).
