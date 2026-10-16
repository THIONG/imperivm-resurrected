# Custom city and village templates

A mod civilization starts the game with a city and a village that the random map picks from a list of game templates. By default it uses its base civilization's: a Dacia cloned from Gaul starts in a "Gaul Town". With your own templates you change the layout, for example a wider, almost square city with a different arrangement of buildings inside the walls.

`--straight` doesn't work well (see below).

## What you can and can't do

- You can move and repeat the game's pieces: walls (`?WallsN/S/E/W/NE/NW/SE/SW`), towers, gates and buildings.
- You can't create new wall pieces. The corners and gates are drawn for the game's octagonal enclosure, so the "square" shape is an octagon with longer long sides.
- No custom terrain: the ground comes from the biome (grass or sand). The game's city has a dirt road in the middle and yours doesn't.
- Templates only appear for players of that civilization. The map's independent villages and cities don't use them.

## How to do it

1. Get the game's templates. In `imperivm-mod.ini` (game folder), section `[Debug]`, set
   `Dump=randommapsettlements/maps/1/map.obj.xml`. Open the game, start a skirmish on a random map and quit. The file
   `mod-dump\randommapsettlements\maps\1\map.obj.xml` appears. Then remove that line.
2. Generate your civilization's templates:

   ```
   node tools\create-template.js "<game folder>" "<your mod folder>" Dacian --base Gaul --width 1
   ```

   - `Dacian` is the prefix. It creates "Dacian Town 1", "Dacian Town 2" and "Dacian Village 1..5".
   - `--base` is the civilization it copies from: Gaul, Republican Roman, Carthaginian, Iberian, Imperial Roman, Briton, Egyptian or German.
   - `--width N` (0 to 4) widens the cities by N spans of 220 units. With Gaul, `--width 1` gives 1760 x 1760 and only widens "Town 1": "Town 2"
     doesn't have the required shape, so the script warns you and leaves it as is.
   - `--no-villages` generates only the cities.
   - `--straight` (experimental, with `--side N`, 6 to 9 spans of 220 per side, 8 by default) makes a truly rectangular enclosure, without the
     cut corners, with the N, S and E gates in the middle of each side. The gate comes out crooked, because the game only has
     gates drawn for the cut corners. Don't use it except to experiment.

   It writes `patches\randommapsettlements\maps\1\map.obj.xml` in your mod.
3. Tell the civilization: in `civs\<id>.json` add `"template": "Dacian"` ([Civilizations](civilizations.md)).
4. Validate: `node tools\validate-mod.js <mod folder>`. It warns if the patch is missing, if there are no cities or villages with that prefix, and if
   any class in the templates doesn't exist.

If the civilization has no template of a given type (say, no village), it uses its base's for that type.

## Editing by hand

Each `<settlement name="Dacian Town 1">` in the XML is a list of pieces with their class and position (x, y). Rules worth following:

- Keep each side under about 1900 units. The satellite village is placed about 2000 from the city's center, and a bigger city would
  overlap it.
- Every class in the XML must exist, or the random map fails to generate and the game doesn't tell you. Use only classes from the game or your mod. The validator
  checks this.
- Towers go every 220 units, and each piece of wall at the midpoint between two towers. A gate takes up two slots.
- Leave the templates in the free area of the plane (y between 24192 and 27904) and at least 3000 units apart.
- Patches under `randommapsettlements/` count as gameplay: in an online lobby, everyone must have the same mod.
