# Civilization template

A new civilization that can be picked in the lobby, with its units, buildings and AI.

## Steps

1. Copy this folder into `<game folder>\mods\` and rename it.
2. In `mod.json` change `id` (lowercase letters, numbers and hyphens: it identifies your mod), `name`, `authors` and `description`.
3. Extract the game's data once: `node tools\extract-data.js "<game folder>" <output folder>`.
4. Clone a civilization from the game:

   ```
   node tools\clone-civilization.js <output folder>\DATA <your mod folder> MyCiv M Gaul 10
   ```

   It writes `civs\`, the classes and the commands. [Civilizations](../../civilizations.md) explains the options, the `index` (10 to 99, one per civilization) and how to change units afterwards.
5. Optional: its own city and village for random maps. `create-template.js` writes them in `patches\randommapsettlements\` ([City and village templates](../../city-templates.md)).
6. Delete the notes. Enable the mod in your profile and pick the civilization in the lobby.

## Folder layout

```
my-civilization\
  mod.json
  README.md
  civs\
    myciv.json
  data\
    classes\
      MyCiv....sc.xml          one per unit and building
    commands\
      MyCiv.xml
  patches\
    randommapsettlements\     optional: city and village templates
  loc\
    spanish.loc.xml
```

## Notes

- A civilization changes what happens in the game: everyone in the room needs the mod.
- The finished example is [civilization-example](../../examples/civilization-example).

## Check and publish

```
node tools\validate-mod.js <your mod folder>
```

The validator reports problems with file and line without opening the game ([Validating a mod](../../validating.md)). Zip the mod folder with its `mod.json` inside and publish it as described in [Publishing on GameBanana](../../publishing.md).
