# Example: victory mode

Adds a new mode, **Dominio**: whoever controls a set number of settlements wins. That number comes from the lobby's "Limit" dropdown (2, 3, 4 or 5); if the script doesn't get a valid value, it uses 3. Elimination still applies: whoever is left with no town halls and no troops loses.

- `data\GameScripts\6 Dominio.vs`: the script. The game launches one copy per player (`//void, int player, str param`). The file name, number included, is the mode's id: don't reuse another mod's or one of the game's (1 to 5).
- `patches\data\const.ini`: fills this mode's "Limit" dropdown. Each line, like `6_Dominio0 = 2 asentamientos, 2`, holds the key (the file name with `_` instead of spaces, plus a number from 0), the text you see and, after the comma, the value the script receives as `param`.
- The script ends a player's game with `EndGame(player, false)` (wins) or `EndGame(player, true)` (loses).

The script's texts are in English and the example ships no translations, so in the Spanish game they show in English. Translate them with a `loc`: see [Languages](../../languages.md).

To test it, copy this folder to `<game>\mods\`, enable it in the content manager, restart and check the "Mode" dropdown in a skirmish. Guide: [Victory modes](../../victory-modes.md).
