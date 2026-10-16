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
- The first line is the signature: `//void, int player, str param`. `player` is the player (1 to 8). `param` is the limit value chosen in the lobby (see below).
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

## Online

A new mode changes gameplay: every player must have the same mod.
