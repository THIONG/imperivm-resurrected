# Starting bonuses

A starting bonus is a script the game runs for one player when the match begins. The lobby lists them in the "Bonus" column of each player, and a mod can add its own.

## The script

It goes in `data\BonusScripts\` in your mod, named `NNN NAME.vs`:

```
data\BonusScripts\004 Tribute.vs
```

- The number sorts the list and is the value the lobby sends to the other players. Only numbers from `000` to `099` appear in the lobby; `100` and above are scripts that other scripts start (the game's conquests use them) and are never offered.
- `000 None.vs` must exist and stay empty: it is the "no bonus" choice. The game brings `000` to `003`; a new bonus takes the next free number.
- The name in the list is the file name without the number, in capitals only at the start of each word (`004 BOAR TEETH.vs` is "Boar Teeth").
- The signature is fixed: `//void, Settlement set, int playerid`. The script runs once per player that chose it, with their main settlement.

Example, 3000 gold at the start:

```
//void, Settlement set, int playerid
//@name 3000 gold
//@name.es 3000 de oro
//@description 3000 gold at the start.
//@description.es 3000 de oro al empezar.

Sleep(100);
set.SetGold(set.gold + 3000);
set.GoldConverted(3000);
```

## Name and description in the lobby

The lines that start with `//@` at the top of the script give the text of the dropdown. They must come before the first line of code.

| Line | Use |
|---|---|
| `//@name Text` | The name shown in the dropdown and in the player's row (short: the column is narrow). |
| `//@description Text` | What the bonus does. It is shown beside the dropdown when the pointer is over that bonus. |
| `//@name.es`, `//@description.it`… | The same for one language (`en`, `es`, `it`). If the player's language has no line, the line without a language is used, then the English one. |

Without these lines the name is the file's and there is no description. The three bonuses of the game have their own names and descriptions while their script is the game's own; a mod that changes the effect of one of them gets the file name, so it should add these lines. If two bonuses end up with the same name, the lobby adds the file number to tell them apart.

## Online

All players need the same files: the content of the room is compared before the match, and the bonus value is a position in the list.

Needs Imperivm Resurrected 1.0 or later.
