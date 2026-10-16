# Example: a new outpost

A new neutral outpost, the Archer fort, one of those scattered around the map that you can capture. It appears on random maps alongside the game's (as many as the Iberian training outpost), has 15000 health and a garrison of 16 Gaul archers, with a Spanish name and description.

It was made with one command. The `DATA` folder comes from `node extract-data.js "<game folder>" <output folder>`:

```
node tools/create-outpost.js <output folder>\DATA examples/outpost-example ArcherFort IOutpost "Archer fort"
```

| File | What it is |
|---|---|
| `data\classes\ArcherFort.sc.xml` | The outpost: a copy of the Iberian training outpost (`IOutpost`) with a new id, name, health and garrison (edited by hand afterwards). |
| `patches\data\randommap\placeobjects.xml` | Adds it to the random map's outpost list, for all three map sizes, with the base's quantities. |
| `loc\spanish.loc.xml` | The name and description in Spanish. |

To test it: copy this folder to `<game>\mods\`, enable it in the content manager, restart and play a random map. Outposts appear scattered around the map, whichever civilization you play. In the editor it's under *Structures / Outposts / Archer fort*.

It changes gameplay: every player has the mod. Guide: [Outposts and buildings](../../outposts.md).
