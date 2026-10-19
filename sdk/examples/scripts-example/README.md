# Example: scripts

Shows the events and behaviors from the [Scripts](../../scripts.md) guide. Careful: it upsets the balance. Every player's Hastatus does double damage and heals 25 when it kills a soldier. Don't enable it in a serious game.

What's in it:

- `data\mod\hello.vs`: a behavior on the `RHastatus` class. It writes lines to the log with `Mod_Log`, `Mod_LogInt`, `Mod_IsActive` and `Mod_ApiVersion`.
- `patches\data\classes\Military.sc.xml` and `data\mod\soldier.vs`: a behavior on the soldiers' base class, which all of them inherit. It records the first ten soldiers to enter the map. This is how you react to an object appearing.
- `events\OnGameStart.vs`: one line when the game starts.
- `events\OnObjectDie.vs`: one line each time a soldier dies.
- `events\OnObjectKill.vs`: one line per kill, and Hastatus heal 25 when they kill a soldier.
- `events\OnAttackResolved.vs`: doubles Hastatus damage with `Mod_SetAttackDamage`. It records the first three blows.
- `events\OnPlayerDefeated.vs`: one line when a player is defeated.
- `events\OnSettlementCaptured.vs`: one line when a settlement changes owner, and it counts captures with a mod variable.

To test it, copy this folder to `<game>\mods\`, enable it in the content manager and restart. Start a game, train a Hastatus with Republican Rome, fight with it and check `Logs\imperivm\imperivm-mod.log`: the lines tagged `script` come from this mod.
