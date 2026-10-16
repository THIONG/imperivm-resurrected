# Outposts and defensive buildings

"Outpost" covers two different things:

- Neutral map outposts: the posts scattered across the map that you capture ("Trade outpost", "Recruitment outpost"…). A mod can
  add new types with a tool, and they appear on their own on random maps.
- Cities' defensive buildings: towers, walls and gates.

## A new neutral outpost

`tools/create-outpost.js` copies a game outpost under a new id and name and adds it to the random maps. It needs the `DATA` folder, which you
get with [`extract-data.js`](tools/extract-data.js) (`node extract-data.js "<game folder>" <output folder>`):

```
node create-outpost.js <output folder>\DATA <mod folder> ArcherFort IOutpost "Archer fort"
```

It writes the class (`data\classes\ArcherFort.sc.xml`), the `data\randommap\placeobjects.xml` patch that adds it to all three map sizes (with the
base's quantities) and the name in Spanish. With `GOutpost`, `TOutpost` or `ROutpost` as the base it also writes `patches\data\commands\*.xml`, so
the new outpost gets the post's commands (sell food, interest).

The possible bases:

| Base | Type |
|---|---|
| `GOutpost`, `TOutpost` | Trade outpost |
| `ROutpost`, `EOutpost` | Gold outpost (interest) |
| `IOutpost`, `BOutpost` | Training outpost |
| `COutpost` | Recruitment outpost |

Then edit the class however you like: `maxhealth`, the garrison (`defender_cls_1`, `defenders_max_1`, `start_level_1`…), its behavior
(`<behavior script=…>`), what it costs to capture. Example: [`examples/outpost-example`](examples/outpost-example).

To change how many appear, edit `min` and `max` in the patch's lines (how many the generator places per map). `mindist` is the minimum distance
between outposts, in pixels. The other numbers are the game's.

The model (`entity`) and icon are the base's. For a model with custom sprites see [Custom graphics](graphics.md): it has a section on
buildings.

## Towers, walls, gates and posts

They're building classes like any other. What's worth knowing before adding your own:

- A settlement (city, village, post) is a group of buildings with an owner, population, gold and food. Its type comes from the class of the central
  building: `BaseTownhall` is a city, `BaseVillage` a village, `Outpost` a post.
- The base game doesn't let you build buildings. There's no build command (only ships and siege machines). Towers, walls, gates and posts
  come from the map: the city template, the random map's posts, scenarios and the editor.
- City templates name specific classes (`GTownhall`, `GTower01`, `GWallsN`, `GGateNW`…). A new civilization uses its own if you clone them
  with the tool in [Civilizations](civilizations.md).

| Base class | What it is |
|---|---|
| `Tower` (`CVXBuilding`) | Tower. Its commands (`Tower Attack`, repair) hang from `Tower`, and a child inherits them |
| `Wall` (`CVXBuilding`) | Wall. `num_sentry_slots` and `sentry_class_name` set how many sentries patrol it and of what class |
| `Gate` (`CVXGate`) | Gate: open, close, permanent, repair |
| `Outpost` (`CVXTownHall`) | Base of the forward posts. The specific ones (`GOutpost`, `GGuardPost`…) use `CVXOutpost` and hang from `Outpost`: they're independent until captured and each does something through a script (sell food, interest, experience…) |
| `GGuardPost` | Guard post, Gaul only: 12 sentries by script. It isn't in `placeobjects.xml`, and `create-outpost.js` warns about it |

For a new class: a `.sc.xml` in `data\classes\` with `parent=` one of the above, a `cpp_class` that already exists (`CVXBuilding`, `CVXGate`,
`CVXOutpost`, `CVXTownHall`) and its `entity`. Copy the `entity` from the base's class, because the game doesn't inherit it from the parent. Custom
behavior goes in `<behavior>` and `<method sig="…" vs="…">`, and commands in `<cmd>`, the same as in [Abilities](abilities.md). A building type
with a new `cpp_class` isn't possible.

## Where to place it

| Place | How |
|---|---|
| Random map | Add it to `placeobjects.xml`, which is what `create-outpost.js` does |
| A civilization's starting city or village | Cloned classes show up in the starting city on their own. To change the layout, see [Templates](city-templates.md) |
| The map editor | With `edittree_pos` in the class. The editor has fixed folders per civilization: a new civilization's show up mixed in with its base's |
| A hand-made map | Place it with the editor and the map names the class (it needs the mod) |

## Multiplayer and limits

It changes gameplay: all players need the same mod.

- The editor has no custom folders or wall sets.
