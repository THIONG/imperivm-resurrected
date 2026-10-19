# Example: patches

Changes the Roman barracks and one constant without replacing any game file. Guide: [Patches](../../patches.md).

What it does:

- The Hastatus goes from 100 to 60 gold, trains in 3 seconds instead of 6 and no longer uses population.
- The barracks gets a new order, `trainRArcherExample`, that trains archers (key `z`).
- `DamageAmplify` goes from 200 to 250 in `CONST.INI`.

It changes gameplay: in multiplayer everyone must have the mod.

To test it, copy this folder to `<game>\mods\`, enable it in the content manager and restart. Check `Logs\imperivm\imperivm-mod.log`: the lines tagged `patch` say what was applied and what failed.
