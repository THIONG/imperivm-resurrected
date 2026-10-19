# Custom game options

A mod can add **toggles** (on / off) and **value lists** (a dropdown) to the RULES list in the lobby, next to "Fog of war" or "Lock diplomacy". The host picks them, everyone sees them, and your scripts read the choice with `Mod_GetOption("id")` (toggle) or `Mod_GetOptionValue("id")` (list). Both are script API 6.

Example: [`examples/options-example`](examples/options-example) (the "Héroes veloces" toggle and the "Velocidad de héroes" list).

## Quick way

`create-option.js` writes the JSON and, if you want, a script skeleton. It doesn't need the `DATA` folder.

```
node tools/create-option.js mods/mi-mod heroes-veloces --name "Héroes veloces" --name-en "Fast heroes" --description "Todos los héroes se mueven un 50 % más rápido."
node tools/create-option.js mods/mi-mod velocidad-heroes --name "Velocidad de héroes" --values "Normal,Rápida,Muy rápida" --default Normal --script
```

The first makes a toggle; with `--values` (2 to 16, comma-separated) it makes a list.

| Option | What it does |
|---|---|
| `--name` | Required. The name shown in the lobby |
| `--name-en`, `--name-it` | The name in English and Italian |
| `--description`, `--description-en`, `--description-it` | The lobby help text |
| `--values`, `--values-en`, `--values-it` | The values of a list, in the same order and the same number. Each value's id comes from its text |
| `--default` | Toggle: `true` or `false`. List: the value's number (from 0), its id or its text. Without it: `false`, or the first value |
| `--script` | Also creates `data\mod\<id>.vs`, a behavior with the `if (Mod_GetOption("<id>"))` (or `Mod_GetOptionValue`) ready to fill in. Hook it to a class with a patch, as the example does |

It generates `options\<id>.json` and, with `--script`, `data\mod\<id>.vs`. It tells you how many of the 8 bits it uses and how many the mod already has. It never overwrites anything: if the mod already has that id, it stops without writing.

## Doing it by hand

1. An `options\<id>.json` file in your mod.

   A toggle:

   ```json
   {
     "id": "heroes-veloces",
     "name": { "spanish": "Héroes veloces", "english": "Fast heroes" },
     "description": { "spanish": "Todos los héroes se mueven un 50 % más rápido.", "english": "All heroes move 50% faster." },
     "default": false
   }
   ```

   A list (2 to 16 values):

   ```json
   {
     "id": "velocidad-heroes",
     "name": { "spanish": "Velocidad de héroes", "english": "Hero speed" },
     "values": [
       { "id": "normal", "name": { "spanish": "Normal", "english": "Normal" } },
       { "id": "rapida", "name": { "spanish": "Rápida (+25 %)", "english": "Fast (+25%)" } },
       "Muy rápida"
     ],
     "default": "normal"
   }
   ```

   - `id`: letters, numbers, `_` and `-`, up to 32. Scripts use it; it's case-insensitive. If missing, the file name is used.
   - `name` and `description`: one text, or one per language (the game's language, falling back to English).
   - `values`: each one is a text or an object with `id` and `name`. The first is 0.
   - `default`: for a toggle, `true` or `false`; for a list, the value's number (from 0) or its `id`. An `id` that doesn't exist silently falls back to 0; `validate-mod.js` catches it.

2. A script that reads it (a class behavior, an event or a victory script):

   ```
   if (Mod_GetOption("heroes-veloces")) {
       This.AsUnit().SetSpeedFactor(150);
   }
   if (Mod_GetOptionValue("velocidad-heroes") == 1) {
       This.AsUnit().SetSpeedFactor(125);
   }
   ```

   `Mod_GetOption` returns `true` if the value isn't 0 (lists too). `Mod_GetOptionValue` returns the number of the chosen value.

## Limits

- **There are 8 bits for all the options of all active mods.** A toggle uses 1; a list uses 1 for 2 values, 2 for 3 or 4, 3 for 5 to 8, and 4 for 9 to 16. They're taken in alphabetical order of `id`; whatever doesn't fit is ignored and the log says so. If two options share an `id`, the game uses the first one it finds when it assembles the mods.
- **They only work with a lobby.** Options are fixed when a game starts from the lobby (or when you rejoin an online one). Adventures, campaigns and tutorials have no lobby: `Mod_GetOption` always returns 0 and a list is 0, even with `"default": true`.
- The lobby presets (Classic, Competitive) don't touch mod options.

## Fixed rules of a mod

Some engine rules are not room options: the mod fixes them for all its games in `rules\game.json`:

```json
{
  "armyStaysWithHero": true,
  "sentriesUnselectable": true,
  "protectedClasses": ["Peasant", "PeasantAmbient", "Trader", "Tradecar_Donkey", "Camel", "CartTrader"]
}
```

| Field | What it does |
|---|---|
| `armyStaysWithHero` | With `true`, the units of an army don't leave their hero when one is inside the walls and the other outside. The path is worked out again all the same. |
| `sentriesUnselectable` | With `true`, wall sentries can't be selected (clicking one selects the gate) and don't join AI armies. |
| `protectedClasses` | Classes that take no damage and can't be attacked, with all the ones that inherit from them. They are class names (`Peasant`), not unit names. |

They change the game, so in multiplayer everyone must run the mod. If several active mods bring rules, they add up.

## Online

This is gameplay: every player must have the same mod. The option travels with the other lobby rules and is saved with the game.
