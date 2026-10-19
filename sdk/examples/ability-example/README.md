# Example: troop ability

A new ability, Second wind, for all melee soldiers (the classes that inherit from `Melee`): key `Y`, costs 5 stamina (of the 10 a soldier has) and heals 60 health.

| File | What it is |
|---|---|
| `data\commands\Respiro.xml` | The order: the button and its key. |
| `patches\data\classes\Melee.sc.xml` | Adds the `respiro` method to the `Melee` class with a patch, without replacing its file. |
| `data\mod\respiro.vs` | The script that runs it. |
| `loc\spanish.loc.xml` | The order's name and description, with `respiro` as context. |
| `art\respiro.png` | The button art (51 x 51). |
| `gameres\cmdbar\Actions\Respiro.bmp` | The button in game format, made from the PNG with `tools/convert-image.js`. |

To test it: copy this folder to `<game>\mods\`, enable it in the content manager and restart. Select a legionary or a warrior and press `Y` (or click its button). Each use writes `Respiro: a soldier was healed` to `Logs\imperivm\imperivm-mod.log`.

It changes gameplay: every player has the mod. Guide: [Troop abilities](../../abilities.md).
