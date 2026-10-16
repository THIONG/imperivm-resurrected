# Example: civilization

Dacia: a new civilization (index 10, letter `D`) cloned from Gaul with [`tools/clone-civilization.js`](../../tools/clone-civilization.js).

```
node extract-data.js "<game folder>" <output folder>
node clone-civilization.js <output folder>\DATA <mod folder> Dacia D Gaul 10
```

`extract-data.js` extracts `data.pak` to `<output folder>\DATA`, the folder the tool asks for.

| File | What it is |
|---|---|
| `civs\dacia.json` | The civilization's declaration. It has `"template": "Dacian"`, added by hand. |
| `data\classes\D*.sc.xml` | A copy of each Gaul class (`DSwordsman`, `DBarracks`, `DVillage`…) with a new id and `race="Dacia"`. Full copies, not children: the game doesn't inherit the entity (the model) or the sounds from the parent. |
| `data\commands\Dacia.xml` | The Gaul orders that name its classes (`trainGSwordsman Dacia`, `trainpeasantd`, `hireheroD`…), pointing to the new classes. |
| `patches\data\commands\*.xml` | Add the new classes to the shared orders (repair, barracks upgrades, smithy…). |
| `patches\randommapsettlements\maps\1\map.obj.xml` | The "Dacian Town" and "Dacian Village" templates (a widened city). Made by `create-template.js`; see [City templates](../../city-templates.md). |

Dacia shows up in the multiplayer lobby dropdowns. It changes gameplay: every player has the mod. In `Logs\imperivm\imperivm-mod.log` (lines tagged `civilizations`) you can see it get registered and, with `LogChannels=civilizations:debug` under `[Debug]` in `imperivm-mod.ini`, which player is playing it.

See [Civilizations](../../civilizations.md).
