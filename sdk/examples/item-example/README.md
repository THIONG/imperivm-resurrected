# Example: new item

Adds an item, the Example Amulet (+10 damage and +5 armor), and has every hero receive it when it appears on the map.

- `patches\data\items.xml`: the item, added to the game's list. Each `<item>` has an `id` (the name scripts use), an icon (`image`), a text (`description`) and its bonuses (`<bonus>`: health, damage, armors, level, experience). An item with its own effect also has `use_script="file://data/mod/mi.vs"`; the game's are in `data\ItemScripts` (extract them with `extract-data.js`, see [Patches](../../patches.md)).
- `patches\data\classes\Hero.sc.xml`: one more behavior for the `Hero` class.
- `data\mod\give_amulet.vs`: what that behavior does, `Obj::AddItem("Example Amulet")`.

If the game can't find the icon or the script fails, `Logs\vxlog.log` says so.

To test it: copy this folder to `<game>\mods\`, enable it in the content manager, restart and start a skirmish with any hero. `Logs\imperivm\imperivm-mod.log` should show «A hero received the Example Amulet», and the hero will have the item in their inventory.

Other ways to give an item: `Obj::AddItem` from any script (for example an `OnObjectKill` that drops it on a kill), or put it in the map's ruins and treasures (see `data\classes\BaseRuins.sc.xml`). Guide: [New items](../../items.md).
