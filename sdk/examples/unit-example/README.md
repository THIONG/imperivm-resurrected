# Example: new unit

The Roman Veteran: a new unit recruited at the Republican Rome barracks (key `V`). It's stronger than the Hastatus (260 health, attack 20, armor 14) and uses its model. It has its own portrait and button.

| File | What it is |
|---|---|
| `data\classes\RVeteran.sc.xml` | The class: a full copy of the Hastatus's with different stats. |
| `data\commands\Veteran.xml` | The order to recruit it at the barracks (180 gold). |
| `gameres\icons\RVeteran.bmp` | The portrait (65 x 76), made with `tools/convert-image.js` from `art\veteran.png`. |
| `gameres\cmdbar\Actions\train RVeteran.bmp` | The button (153 x 51), from `art\button.png`. |
| `loc\spanish.loc.xml` | The unit name and button text, in Spanish. |

To test it: copy this folder to `<game>\mods\`, enable it in the content manager and restart. Play Republican Rome and look for the new button (key `V`) at the barracks. It changes gameplay: every player has the mod.

For a unit with its own sprites, see [Custom graphics](../../graphics.md). Guide for this one: [New units](../../units.md).
