# Custom graphics for units and buildings

Start from the [graphics template](templates/graphics), an empty mod with the folders this guide uses.

The SDK doesn't draw anything: use Photoshop, Paint.NET, Aseprite, Blender… The game only reads its `.rle` format, which no program exports, so the SDK ships a converter.

A mod can ship its sprites as loose files, using the same paths the game does. A new unit or building changes gameplay: every player in the game must have the mod.

Example: [`examples/unit-graphics-example`](examples/unit-graphics-example) (a cube soldier, 8 directions, player color). It's drawn by `tools\draw-soldier.js`, the only thing you'd replace with your own drawings.

Run every command in this guide from the repo root. The tools are Node.js and need nothing else.

## What you can do

- A new unit with its own sprites: yes, in a new `units\MyUnit\` folder.
- Change the sprite of an existing unit: not with a loose file, because the game prefers its own. Create a new unit.
- A new building with its own sprites: yes, with more work (big sheets, construction stages, `.pass` footprint and points). See Buildings.
- Icons and interface buttons: already covered in [Custom images](images.md).
- New sounds: `.wav` and `.ogg`.

## The tools

- `tools\convert-sprite.js`: your PNGs to `.rle`. It crops, reduces to 256 colors, marks the player color and makes the shadow.
- `tools\create-entity.js`: writes the `.ent.xml` (images, layers, states, animations) of a unit or building, and a building's `.pass` footprint.
- `tools\read-rle.js`: the reverse, `.rle` to PNG. Use it to look at a game sprite and to start from it.
- `tools\validate-mod.js`: warns about missing images, repeated `idx` values, and rows and columns that don't add up ([Validating](validating.md)).

## A unit, step by step

### 1. Draw and save the PNGs

One PNG per animation, direction and frame, all the same size (the canvas, 150 x 150 for a unit) with a transparent background:

```
Idle_d0_f00.png  Idle_d0_f01.png  ...  Idle_d7_f07.png      <Name>_d<direction>_f<frame>.png, counting from 0
Walk_d0_f00.png  ...
Fight_..., Death_..., Transition_..., Taunt_...
```

- **Animations.** Required: `Idle` (standing), `Walk`, `Fight` (attack) and `Death`. Optional: `Transition` (from idle to combat) and `Taunt`. If they're missing, `Idle` is used instead.
- **Directions.** 8 for soldiers, 12 for heroes and animals, 4 for ships. Every animation of a unit uses the same number.
- **Direction order.** In the game's sheets, column 0 faces the camera (down on screen) and the rest go clockwise: south, west, north, east, with the diagonals in between. With a sprite of your own, look at a game sheet with `read-rle.js` before drawing all eight.
- **Canvas and feet.** Same canvas in every frame, with the feet at the same point. In the game's units it's pixel (75, 100) of 150 x 150. Change it with `--canvas` and `--feet`.
- **Size.** The engine doesn't impose one: each frame is cropped to what you drew. For reference, a game soldier is about 40 x 52 pixels and a hero about 40 x 88 (approximate).
- **Frames.** Up to 62 per animation. Hastatus: walk 15, attack 14, die 20, idle 8.
- **Transparency.** All or nothing: alpha below 128 is transparent. The game has no semi-transparency.
- **Colors.** Up to 256 per set; with player color, 192 (the other 64 are the tint). If you use more, the converter reduces them and warns you.
- **Player color.** Paint in magenta whatever should change with the team. The converter treats magentas (similar red and blue, almost no green) as the tint. `(255, 0, 255)` is almost pure player color; `(128, 0, 128)`, `(64, 0, 64)`… are its shades (7 usable levels). Avoid purples and pinks elsewhere on the figure: they can end up as tint by accident. Don't smooth the edges: a blended semi-transparent edge comes out pink, not tint.
- **Shadow.** Optional: one PNG per frame with the same name plus `_sombra` (`Walk_d3_f05_sombra.png`), opaque black or dark gray on transparent. If you skip it, the converter draws an ellipse under the feet. The game darkens the ground under the shadow by half.

You can also give one assembled sheet per animation: `Walk.png` with directions as columns and frames as rows, each cell the size of the canvas (`--canvas WidthxHeight`). The shadow goes in `Walk_sombra.png` with the same layout.

### Starting from a game sprite

The game's sprites are in the `.rle.mmp` files of the `.pak` files, and their pixels in `rle.mmp`, in the game folder. Extract them first with `extract-data.js`:

```
node tools\extract-data.js "<game folder>" <output folder> Units.pak HASTATUS
```

And convert them to PNG with `--convention`, which writes exactly what `convert-sprite.js` expects (one PNG per direction and frame, full canvas, the player tint in magenta and the shadow in `_sombra`):

```
node tools\read-rle.js <output folder>\UNITS\RHASTATUS\WALK.RLE.MMP --rle-mmp "<game folder>\rle.mmp" --convention --output <carpeta de PNG>
node tools\convert-sprite.js <carpeta de PNG> --player --output <mod>\units\MyUnit
```

Without `--convention`, `read-rle.js` writes a sheet with cells the size of each crop, and `convert-sprite.js` won't accept it. `--canvas WidthxHeight` changes the canvas (150 x 150 by default). The game's shadow is a separate file (`WALK_SHADOW.RLE.MMP`): if you don't extract it, the converter draws the ellipse. These are your game files, for studying or starting from: don't redistribute them.

### 2. Convert to `.rle`

```
node tools\convert-sprite.js <folder with the PNGs> --output <mod>\units\MyUnit --player
```

- `--player`: use player color (magenta becomes the team tint). Without it, the palette is a free 256 colors with no player color; it's the simplest and the one wolves and deer use.
- `--shadow auto|files|none`: `auto` by default (your `_shadow` files if present, otherwise an ellipse).
- `--palette-per-animation`: one palette per `.rle`. By default there's one for the whole unit, with the quality loss of reducing to 192 or 256 colors if you use more. The game uses one per animation.
- `--canvas WidthxHeight` (the PNGs' size by default), `--feet X,Y` (only for the automatic shadow), `--alpha-threshold N`.

It writes `Idle.rle`, `Idle_shadow.rle`, `Walk.rle`…

### 3. Create the entity (`.ent.xml`)

```
node tools\create-entity.js unit <mod>\units\MyUnit --name MyUnit
```

It reads the `.rle` files in the folder and writes `MyUnit.ent.xml`: 2 layers (unit `z=1000`, shadow `z=800`), 2 states (idle and combat) and the engine's 7 animations (walk 1, attack 5, die 9, `Idle` 13, `ToIdle` 18, `ToAttack` 19, `Taunt` 20) with the Hastatus timings (walk 55 ms per frame, attack 71, die 80 with a slow last part, idle 150…). That's 6 animation sheets for 7 engine animations, because the `Transition` sheet serves `ToAttack` and, reversed, `ToIdle`.

Engine limits: see *Format reference*. To change the pace, use `--config animations.json`:

```json
{ "Walk": { "duration": 55, "step": 42 }, "Fight": { "duration": 71, "hit": 426 } }
```

`duration` is milliseconds per frame. `step` and `hit` (`action_time`, the milliseconds until the blow lands) are part of the simulation and affect gameplay.

### 4. The class and the rest

The class (`data\classes\MyUnit.sc.xml`) is a full copy of a game class with `entity="Units/MyUnit/MyUnit.ent.xml"`. The recruit order, the button and the texts are covered in [New units](units.md). Don't make the class a child of another: the game doesn't inherit `entity` from the parent and the unit disappears. The folder name must be new: if you reuse a game unit's name, the game uses its `.rle` and yours is ignored.

### 5. Validate and test

```
node tools\validate-mod.js <mod> --data <DATA> --game "<game folder>"
```

A missing image crashes the game when it loads the class (a repeated `idx` or a corrupt `.rle` does the same), and the validator catches it beforehand. Then copy the mod to `<game>\mods\`, enable it and restart. If the game closes, `Logs\imperivm\imperivm-mod.log` points to the image that failed.

## Buildings

Same shape as a unit, with these differences:

- **One direction** (`_d0`) and several rows, which are the construction stages (the game's use 4). One sheet per building layer, `layer1`, `layer2`… (up to 5 in the game), plus `shadow`. PNGs: `layer1_d0_f00.png` (layer 1, stage 0), `layer1_d0_f01.png`… and `shadow_d0_f00.png`… An animation named `shadow` is taken directly as a shadow mask (any opaque pixel counts).
- **Large canvases**, about 700 x 700 in the game. You choose the point where the building is pinned (`--feet`); on the Roman barracks it's (305, 330).

```
node tools\convert-sprite.js <PNG> --output <mod>\buildings\MyBuilding --player --shadow none
node tools\create-entity.js building <mod>\buildings\MyBuilding --name MyBuilding --feet 305,330 --footprint 8x8 --points-from <another .ent.xml>
```

- **The footprint (`.pass`)** is the 1-bit grid that says where units can't walk. Each cell is 16 map units (a bit under half a soldier's width). `--footprint WidthxHeight` makes a rectangle centered on the anchor point. `--footprint-png máscara.png` uses a drawing (1 pixel = 1 cell, north up; opaque and dark = blocked, transparent or white = free). Keep it to about 120 x 120 cells at most. A game house is about 15 cells per side and the Roman barracks about 29.
- **The points (`<point>`)** are doors, sentry spots, smoke… The converter can't invent them. With `--points-from <.ent.xml of a similar game building>` it copies that building's points; their coordinates are relative to the anchor point, so adjust them if yours has a different shape.
- The building's class (`CVXBuilding`, `parent="Building"`, orders, costs…) is made as in [New units](units.md): a full copy of a game one.
- Not done: winter versions (`entity_winter`) and building animations (smoke, fire). You can add them by hand.
- The final message of `convert-sprite.js` suggests `create-entity.js unit` even when you convert a building: use `edificio`.

## Format reference

If you want to read or write the formats yourself, the code is in `tools\lib\rle.js` (`.rle`) and `tools\lib\pass.js` (`.pass`). The tools write 256-color `.rle` files (with or without player color) and shadow; there's no true color.

`.ent.xml` limits:

- `<image idx file rows columns drawmode>`: 32 images; `idx` from 1 to 32, no repeats; `file` without extension or with `.rle`.
- `<layer idx name image z …>`: 32. Unit `z=1000`, shadow `z=800`.
- `<state idx …>`: 8.
- `<anim idx name frames duration action_time step>`: `idx` from 1 to 28; `frames` = rows + 2 (two sentinel frames with duration 0); up to 64 frames.
- `variations` of `<entity>`: equal to the columns of all the images.

## Sounds

The class names its sounds with `<sounds attack="…" die="…"/>`. Each name is a file `data\sound entities\<Name>.xml` that lists the `.wav` or `.ogg` files; you can put your own in the mod. If you don't want new sounds, copy those of a similar unit (`attack="UnitSwordFight" die="UnitDeath"`).
