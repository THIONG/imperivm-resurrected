# New items

The items heroes carry (amulets, herbs, gems…) are data. The game ships 42 of them in `data\items.xml`, and a mod can add its own with a [patch](patches.md), with nothing to compile.

Ready-to-copy example: [`examples/item-example`](examples/item-example).

## Quick way

The example item, with every hero receiving it, comes from:

```
node tools/create-item.js <mod folder> "Example Amulet" --damage 10 --armor 5 --give-heroes
```

It generates, inside the mod folder:

- `patches\data\items.xml`: the item with all its attributes. If the patch already exists, it appends the item and leaves the existing ones alone.
- With `--give-heroes`: `patches\data\classes\Hero.sc.xml` (one more behavior for the `Hero` class) and `data\mod\dar_<id>.vs`, the script that gives the item to each hero that doesn't have it yet. If `Hero.sc.xml` already exists, it adds to it too.
- `mod.json`, only if there wasn't one.

| Option | What it does |
|---|---|
| `--health`, `--damage`, `--armor`, `--level`, `--experience` | What it adds to whoever carries it. `--armor` sets `armor_slash` and `armor_pierce` |
| `--icon` | A game icon, for example `"gameres/icons/items/Belt of might.bmp"`. Without it, the bear teeth icon |
| `--description` | The text in the item window. Without it, an English one like `Adds 10 to damage and 5 to armor` |
| `--give-heroes` | Every hero receives the item when it spawns |
| `--data <DATA>` | Checks that no game item already uses that id |

Running the command again with another id adds another item. If the id already exists in your patch or in the game, it says so and writes nothing.

It doesn't generate `use_script`, `equip_script` or `kill_script`, nor `usecount` or `cursed`: add them to the `<item>` by hand ([An item with its own effect](#an-item-with-its-own-effect)). What it generates matches the example.

## What it does inside (doing it by hand)

## An item

A patch of `data\items.xml`, at `patches\data\items.xml` in your mod:

```xml
<patch>
  <add select="/items">
    <item id="Example Amulet"
          image="gameres/icons/items/Bear Teeth.bmp"
          sound="data/sound entities/ItemUse.xml"
          level="0"
          name="Example Amulet"
          description="Adds 10 to damage and 5 to armor"
          important="no">
      <bonus health="0" damage="10" armor_slash="5" armor_pierce="5" level="0" experience="0"/>
    </item>
  </add>
</patch>
```

| Attribute | What it does |
|---|---|
| `id` | The name scripts use (`AddItem("…")`). Unique |
| `image` | The icon: one from the game, or your own inside `gameres/icons/items/` in your mod (a BMP) |
| `name`, `description` | What the item window shows |
| `level` | Minimum hero level to carry it (no game item uses anything but 0) |
| `important` | `yes` or `no`. Whether it's a special item (shown separately) |
| `usecount` | How many times it can be used: `1` for single-use items, more for charged ones (`1000` food in the wheat) |
| `cursed` | `1` if it can't be removed, `0` if it can. It's a number, not `yes`: the game reads it as a number and any text counts as 0, with no warning. |

`<bonus>` adds to whoever carries the item: `health`, `damage`, `armor_slash`, `armor_pierce`, `level` and `experience`. With `health_percent`, `damage_percent`, `armor_slash_percent` and `armor_pierce_percent` it adds a percentage instead of a flat number.

`items.xml` accepts a few more attributes (`customdata`, `location_script`, `object_script`, `attacheddie_script`), undocumented; see how the game's items use them.

## An item with its own effect

A script that runs when the hero uses the item:

| Attribute | When it runs | Signature |
|---|---|---|
| `use_script="file://data/mod/mi.vs"` | When the player uses the item | `// void, Obj owner` |
| `equip_script` | When it's equipped | See the game's |
| `kill_script` | When the carrier kills someone with it | See the game's |

Write the path with `file://`, like the game's items (`file://data/ItemScripts/….vs`).

The game's scripts are in `data\ItemScripts`; to see them, extract the data with [`extract-data.js`](tools/extract-data.js). The healing herbs one:

```
//void, Obj owner

CreateFeedback("Heal", owner.AsUnit());
owner.Heal(owner.maxhealth);
ItemUsed(1);
```

`ItemUsed(n)` spends `n` uses of the item. With `usecount="1"` it disappears when used.

## Getting it into a game

- Give it from a script: `Obj::AddItem("id")` returns `true` if it fits in the inventory. The example does it in a behavior on the `Hero` class (`patches\data\classes\Hero.sc.xml`), which runs for every hero that spawns. Check first that the hero doesn't have it (`FindItem("id").IsValid`): loading a saved game restarts the script.
- Drop it on a kill: an [`OnObjectKill`](scripts.md) event that calls `AddItem` on the killer, or `DropItem` on the ground.
- In map ruins and treasures: see `data\classes\BaseRuins.sc.xml`. It hands out items with `itemtype` and `levelperitem` lines. In the game data no other class uses this, and neither do the civilizations' shops.

Items change gameplay: in an online game every player must have the same mod.
