# Custom random map types

A mod can add a random map type (like Coastal, Islands or Mountain passes) to the lobby's "Map type" dropdown. The type is a drawn shape: land, sea, mountains and the spots where each player starts. The game warps it, rotates it and fills it with terrain, trees and objects using its usual rules.

Example: [`examples/random-map-example`](examples/random-map-example) (a ring of land with a sea in the middle and mountains in between). The repo only ships the drawing: the files the game reads are generated with the command in step 2.

## How to do it

1. **Draw the shape** in a square PNG, with any program you like and four colors:

   | Color | Meaning |
   |---|---|
   | black `(0, 0, 0)` | sea |
   | white `(255, 255, 255)` | land |
   | magenta `(255, 0, 255)` | mountains: thin lines, about 3 pixels (optional) |
   | red `(255, 0, 0)` | points where the game may place start sites and neutral cities (optional) |

   A color that isn't exact snaps to the closest one. Transparent counts as sea. If the PNG isn't square, it's stretched to 512 x 512 without warning. The game then stretches it to the map size, so use thick strokes: fine details get lost.

   The game's own drawings have hundreds of loose red dots across the land, and the game picks among them. Normally, don't paint any and let the tool spread them. Paint red only if you want the sites in particular areas.

2. **Turn it into a map type**, from the repo root:

   ```
   node tools\create-random-map.js <mod folder> Anillo dibujo.png --players 8
   ```

   For the example:

   ```
   node tools\create-random-map.js examples\random-map-example Anillo examples\random-map-example\drawings\ring.png --players 8
   ```

   The tool writes `data\randommap\Anillo.rm.xml` and `randommap\Anillo\010.bmp … 120.bmp` into your mod. The name (`Anillo`) uses only unaccented letters, numbers and spaces, starts with a letter and is 2 to 31 characters long. Don't use the name of a game type (`Coastal`…): the tool doesn't warn you and you'd overwrite the game's.

   | Option | What it does |
   |---|---|
   | `--players N` | Maximum players on a large map (2 to 8). On a medium one, up to 6 |
   | `--small N` | Maximum on a small map (0 = doesn't appear on small; default 2) |
   | `--distortion X` | How much the shape is warped when generated, 0 to 0.5 (0.1 barely, 0.25 a lot; default 0.2) |
   | `--no-rotate` | Don't rotate or mirror it at random |

   You can pass several PNGs (`dibujo1.png dibujo2.png …`) and the game picks one at random each game. The tool fills up to 12 by repeating yours. If 12 isn't a multiple of the PNG count, the split is uneven (with 5, three PNGs come up 3 times and two come up 2). With just one, that shape always comes up.

3. Enable the mod and restart. The lobby offers "Random: Anillo".

## Things to keep in mind

- It changes gameplay: every player must have the same mod.
- "Find game" uses a fixed list of five game types: a new type only shows up in games you create yourself.
- You can't create new biomes: the game has four fixed zones. You can change what each one holds with [patches](patches.md) on `data\randommap\terrainmix.xml`, `placetrees.xml` and `placeobjects.xml`.
- Test with the player count you plan to allow.
