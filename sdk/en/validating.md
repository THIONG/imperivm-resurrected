# Validating a mod

The validator checks your mod without opening the game and reports problems with file and line. You need [Node.js](https://nodejs.org).

```
node tools/validate-mod.js <mod folder>
node tools/validate-mod.js <mod folder> --data <DATA folder>
```

`--data` is the `DATA` folder from the extracted `data.pak` (`tools/extract-data.js` extracts it, see the [README](README.md#game-data)). With it, the validator also checks that the files your mod patches exist and that the scripts your classes point to exist. It only looks at paths starting with `data/`: if you patch `units/…`, `buildings/…` or `currentlang/…`, it doesn't check them. Without `--data`, everything else is still checked.

With `--game <game folder>` (read-only) it checks the names in `Units.pak`, `Buildings.pak`, `MapObjects.pak` and `Visuals.pak` to see whether the images and entities your mod takes from the game exist, and warns if your `.rle` has the same name as one of the game's:

```
node tools/validate-mod.js <mod folder> --data <DATA> --game "C:\Program Files (x86)\Steam\steamapps\common\Imperivm - Great Battles of Rome HD"
```

It exits with code 1 if there are errors, 0 if there are only warnings or nothing, and 2 if you call it wrong. You can use that to automate it.

An error is something the game would reject or that will fail. A warning is a suspicion: something the game ignores or that you probably didn't mean.

## What it checks

- **`mod.json`**: broken JSON, a category or tags that aren't GameBanana's, wrongly typed texts (errors). Missing `id`, `name`, `version` or `category` (warnings).
- **Files**: executable and system script extensions (error: the mod wouldn't load) and paths with `..`. An odd but harmless extension (`.psd`, `.gif`, `.zip`…) only gives a warning: the mod loads, the file just takes up space.
- **`civs\*.json`**: an invalid `id`, `index` outside 10–99 or duplicated, a `base` that doesn't exist, a `letter` longer than one letter, an unknown `biome`, a misspelled `template` or one without its file. With `"ai": true` and no `alias` table it warns, and it checks that the `alias` points to classes and commands the mod has.
- **`specials\*.json`**: an invalid `id` or one that's a game unit special, a `bit` outside 36–63 or duplicated.
- **`options\*.json`**: the bit allocation (8 fit across all mods).
- **XML patches**: malformed XML (the patch wouldn't apply at all), an operation without `select`, a misspelled selector, tags that aren't operations. A selector that doesn't start with `/` or `//` is an error even though the DLL would accept it.
- **Script patches (`.vs`)**: a directive that doesn't exist, `@before`, `@after` or `@replace` without text, a directive with no code under it, and `@top` with code that isn't a variable declaration. It doesn't check the patch's code, the `Mod_*` functions or the signature.
- **INI patches**: lines it can't understand, and a new section in `UNIT_SPECIALS.INI` (the game may crash). It doesn't warn about trailing comments on a patch line, which the engine doesn't support (see [Patches](patches.md)).
- **Classes (`data\classes`)**: malformed XML, an unknown `race`, misspelled or unknown `unit_specials` (the game is case-sensitive: `Parry`, not `parry`), and a class without `entity` (warning: the game doesn't inherit it from the parent and the unit disappears).
- **Encoding**: a Latin-1 class XML with accented characters. The game doesn't load the class and doesn't warn; save it as UTF-8.
- **Commands (`data\commands`)**: malformed XML and duplicate commands in a file.
- **Entities (`.ent.xml`)**: error if the image doesn't exist (the game hangs when loading the class; checked against the mod folder if it ships its `.rle` files, and always with `--game`), duplicate `idx` or one outside 1–32, layers and animations that point to something that isn't there, engine limits (32 images, 32 layers, 8 states, animations 1–28, 64 frames), and `columns`, `rows` or a `<frame>` count that don't add up. An animation that reads more rows than its image has is only a warning.
- **Graphics (`.rle`, `.pass`)**: a truncated or corrupt `.rle`, a renamed `.rle.mmp` (no pixels, doesn't load from a mod), an empty palette, a malformed or empty `.pass`, a `pass_file` that doesn't exist, and an `.rle` with the same name as one of the game's (the game's wins; with `--game`).
- **`placeobjects.xml`** (random maps): a named class that doesn't exist (with `--data`). Map generation hangs without a message.
- **Texts (`loc`)**: malformed XML, entries without `text` or without `result`.
- **Scripts (`.vs`)**: missing signature on the first line, an unknown event or one with the wrong signature, `Sleep` in an event that can't wait, `Mod_*` functions that don't exist, `Mod_HasSpecial` with an `Obj` instead of a `Unit`.

## Example output

```
WARNING  specials/c.json  no "name": the id will be shown
ERROR  specials/d.json  bit 40 is already used by specials/c.json
WARNING  data/classes/G.sc.xml:2  unknown unit special "Zzz": the game ignores it

5 files checked: 1 errors, 5 warnings.
```

## What it doesn't do

It doesn't apply patches or run scripts: you only see that in the game (use `Dump=` from [Patches](patches.md) to see the result, and `Logs\imperivm\imperivm-mod.log` for runtime errors). It doesn't judge image quality either.

It's meant for new mods made with this SDK. An old mod that works in the game, like the Community Mod, gives errors (scripts without a signature, class scripts that don't exist, animation rows) without that meaning it won't load.
