# Example: custom game options

Adds two options to the lobby's RULES: a toggle, **Fast heroes** (+50%), and a list, **Hero speed** (Normal, Fast +25%, Very fast +50%).

- `options\heroes-veloces.json` and `options\velocidad-heroes.json`: the options, with a per-language name and help text and their default value. The `id` is what scripts use.
- `data\mod\fast_heroes.vs` and `patches\data\classes\Hero.sc.xml`: a behavior on the `Hero` class that reads `Mod_GetOption("heroes-veloces")` and `Mod_GetOptionValue("velocidad-heroes")` and speeds the hero up.

They appear in the lobby for all players, but only the host can change them. They travel with the other rules and are saved with the game. Across all active mods there are 8 bits for options: a toggle uses 1 and a list uses 1 to 4. Outside the lobby (adventures, campaigns) they're always 0.

To test it, copy this folder to `<game>\mods\`, enable it in the content manager, restart, create a skirmish and check "Fast heroes" under RULES. With `Mod_LogInt` the script leaves one line per hero in `Logs\imperivm\imperivm-mod.log`. Guide: [Game options](../../options.md).
