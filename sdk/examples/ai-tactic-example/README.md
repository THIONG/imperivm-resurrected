# Example: custom AI tactic

A mod civilization with `"ai": true` plays like its base by default: the game's AI scripts see the base civilization, so Dacia uses the same as Gaul. This example makes Dacia's AI pick a different tactic.

`GetTacticScript.vs` decides at startup which tactic each AI uses, based on its civilization. For Gaul it returns `TS_GaulTactic`, which doesn't compile, so the Gaul AI follows no tactic. The game ships another one, `TS_GaulTactic2` (a build order with hero, barracks, archers and druids), that no AI uses. Dacia uses it here.

| File | What it is |
|---|---|
| `patches\data\ai\GetTacticScript.vs` | A [script patch](../../patches.md): adds a chunk to the game's `GetTacticScript.vs`, right before its first line of code (variables are declared before it). If the real civilization of the central building is Dacia (`Mod_TrueRace`), it returns `TS_GaulTactic2`. The rest of the script is untouched. |

`Mod_TrueRace(object)` returns an object's real civilization. `object.race` won't work here: with `"ai": true` the AI sees the base (Gaul).

It needs the "Example: civilization" mod (Dacia). To test it: copy this folder to `<game>\mods\`, enable it together with `civilization-example` in the content manager, restart and play against a Dacia AI (or a random AI: Dacia is in the draw). The AI should build a barracks, hire a hero, train archers and druids, and raise population, in that order. `Logs\imperivm\imperivm-mod.log` shows `script: GetTacticScript: la IA de Dacia usa
TS_GaulTactic2`.

Because it's a patch and not a copy of the file, two mods can give tactics to their civilizations at the same time without overwriting each other. It changes gameplay: every player has the mod. See [Civilizations](../../civilizations.md#the-ai) and [Patches](../../patches.md).
