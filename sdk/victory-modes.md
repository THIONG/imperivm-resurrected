# Custom victory modes

The game's victory conditions (Elimination, Score limit…) are `.vs` scripts. A mod can ship its own, and they show up by themselves in the lobby's "Mode" dropdown.

Example: [`examples/victory-example`](examples/victory-example) (the "Dominio" mode: whoever controls N settlements wins).

## The script

It goes in `data\GameScripts\` in your mod:

```
data\GameScripts\6 Dominio.vs
```

- **The file name is the mode's id**, number included. The lobby lists modes by full file name. Two mods clash only if they use exactly the same name (`6 Dominio.vs` in both). The game's own modes are 1 to 5.
- The lobby shows it without the leading number, and runs it through translation: if the text "Dominio" has an entry in your `loc`, it shows up translated ([Languages](languages.md)).
- The signature is `//void, int player, str param`. The game always loads the script with it, so the first line is optional, but it is best to write it. `player` is the player (1 to 8). `param` is the limit value chosen in the lobby (see below).
- The game launches 8 copies of the script, one per player, each in its own thread, even with fewer players. Write it from a single player's point of view, looking at the others. It has to cope with empty slots: the example treats them as eliminated because they have no settlements and no troops.

## Functions you'll use

| Function | What it does |
|---|---|
| `EndGame(player, false)` / `EndGame(player, true)` | The player wins / loses. Follow it with `return` |
| `SetPlayerStatus(player, linea, texto, important)` | Status text at the top: line 0 is the mode name, line 1 the notice. There's also `SetPlayerStatus(player, linea, bool)`; the game's scripts use both |
| `ClassPlayerObjs("BaseTownhall", player)` | A player's objects of a class; `.count` counts them, `.IsEmpty()` tells you if there are none |
| `MilUnits(player)` | How many troops they have |
| `DiplAreAllied(a, b)` | Whether two players are allied |
| `Str2Int(param)` | Converts `param` to a number |
| `EnvReadInt` / `EnvWriteInt` | Variables shared between scripts. With a player (`EnvWriteInt(player, "clave", n)`) or without (`EnvWriteInt("modo", 1)`) |
| `GetConst("…")` | `CONST.INI` constants |
| `Translate("…")` / `Translatef("…", a, b)` | Localized text |

To see how the game does it, extract its scripts with `extract-data.js` (`node tools\extract-data.js "<game folder>" <output folder>`) and read `GAMESCRIPTS\1 ELIMINATION.VS` and `2 SCORE LIMIT.VS`. You can also use the dump from [Patches](patches.md), with the full path: `Dump=data/GameScripts/1 ELIMINATION.VS`.

## The "Limit" dropdown

The game fills it from `CONST.INI` entries, section `[GamePlay]`. The key is the file name with `_` instead of spaces, plus a number from 0. The value has two parts separated by a comma: the text you see and what the script receives as `param`.

The example does it with an INI patch in `patches\data\const.ini`:

```ini
[GamePlay]
6_Dominio0 = 2 asentamientos, 2
6_Dominio1 = 3 asentamientos, 3
6_Dominio2 = 4 asentamientos, 4
6_Dominio3 = 5 asentamientos, 5
```

With no entries, the dropdown comes up empty and there's no limit to pick. What `param` receives then hasn't been seen in game: the example falls back to 3 if `Str2Int(param)` gives less than 2.

## Fixed settings and civilizations of a mode

A mode can fix room settings and the civilizations that can be chosen with it. It goes in `modes\<name>.json`:

```json
{
  "mode": "2 LIGA",
  "gold": 5000,
  "population": 100,
  "noFog": false,
  "explored": false,
  "bonus": 1,
  "civilizations": ["RomeImperial", "Gaul", "Iberia", "CarthageNumidian"]
}
```

| Field | What it is |
|---|---|
| `mode` | The name of the mode's script in `data\GameScripts`, without `.vs` (`2 LIGA` for `2 LIGA.vs`), up to 31 characters. If you leave it out, the name of the json file. It also works for the game's modes. |
| `gold` | (Optional) The starting gold: one of the values of the dropdown (`5000`, `10000`…). |
| `population` | (Optional) The population: `50`, `100` or `150` (`LowPop`, `NormalPop` and `HighPop` of `CONST.INI`). |
| `noFog` | (Optional) `true` removes the fog, `false` keeps it. |
| `explored` | (Optional) `true` starts with the map explored. |
| `bonus` | (Optional) The bonus of every player, from 0 (none) to 3, the same number as the room's bonus button. Disable bonuses can no longer be ticked. |
| `civilizations` | (Optional) The civilizations that can be played in that mode: the game's (`Gaul`, `RepublicanRome`…) or the `id` of one of the mod's. A game base allows all its variants; a variant, only that one. |

While that mode is chosen, the room doesn't let anyone change what it fixes: the host sees those rows but can't change them. A player with a civilization the mode doesn't allow moves to the first one of the list, and the random civilization is drawn only among the allowed ones. The host applies it when the game is created, so everyone plays with the same. Saved games and scenarios with fixed civilizations don't change.

## Online

A new mode changes gameplay: every player must have the same mod.
