# New formations

Hero formations (Line, Front, Block, Cavalry) are data: a `formations.xml` with bonuses and placement, and two orders per formation in `data\commands\Hero.xml`. A mod adds its own with [patches](patches.md), with nothing to compile.

Example: [`examples/formation-example`](examples/formation-example) (the Wedge).

## Quick way

`create-formation.js` writes all three pieces. You need the game's `DATA` folder ([Game data](README.md#game-data)):

```
node tools/create-formation.js <DATA> mods/my-mod Wedge --damage 3 --armor 1 --like "Cavalry core" --key i
```

| Option | What it does |
|---|---|
| `<Name>` | The formation's `Name`: letters and numbers, no spaces. Must be new |
| `--damage`, `--armor`, `--pierce-defense`, `--slash-defense`, `--range`, `--level` | The bonuses (-1000 to 1000). With none, the formation only changes placement |
| `--like` | The game formation to copy the class list, size, spacing and button image from: `Front` (default), `Line`, `Block`, `Cavalry core`, `Horse Wings`, `Ranged Wings` |
| `--classes "Unit=wings,Horse=front"` | Your own placement, instead of `--like`'s. The slots are `front`, `wings` and `center` |
| `--width`, `--height`, `--front`, `--wing-x`, `--wing-y` | `Width`, `Height` and the three spacings (`Offset…`) |
| `--key` | The key that selects the formation. Without it, it looks for a free one |
| `--description`, `--button` | The `Description` (in English) and a different button (`States/…`) |

It generates:

```
patches\data\formations.xml                     the formation
patches\data\commands\Hero.xml                  its two buttons, form-<name>-menu and form-<name>
patches\data\subai\hero_formation_verify.vs     the script patch
mod.json                                        only if the folder doesn't have one
```

If the mod already has another formation, the new one is added to those same files without overwriting the old one. That also holds for the script patch: each formation adds its own `@replace`.

It checks that the key is free for the hero, which inherits the orders of `Unit` and `Military`. On a clash it warns and generates anyway. In the game only `i` is free, so a mod's second formation will have to share a key: pick a letter and make sure the clash doesn't bother you.

The buttons use the base formation's image. For your own, see [Custom images](images.md) and pass `--button`.

## What it does inside (doing it by hand)

### The three pieces

**1. The formation** (`patches\data\formations.xml`):

```xml
<patch>
  <add select="/Formations">
    <FormationClass Name="Wedge"
        Description="The warriors form a wedge with the cavalry at the point."
        Width="3" Height="1"
        BonusDamage="3" BonusArmor="1"
        OffsetFrontLineByY="40" OffsetWingsByX="30" OffsetWingsByY="0">
      <Class Name="Unit" Wings="1"/>
      <Class Name="Horse" FrontLine="1"/>
      <Class Name="BaseMage" CentralBlock="1"/>
      <Class Name="Peasant" CentralBlock="1"/>
    </FormationClass>
  </add>
</patch>
```

| Attribute | What it does |
|---|---|
| `Name` | The formation's name; orders refer to it |
| `Description` | Not shown on the button: the game shows fixed text ("Army bonus during stand ground") with the bonus icons |
| `Width`, `Height` | Grid size of the front-line group (`FrontLine`). In the Wedge, where `Unit` goes on the wings, `Width=3` only affects the horses |
| `BonusDamage`, `BonusArmor`, `BonusRange`, `BonusLevel` | Army bonuses, only while the troops stand still ("stand ground"). Clamped to -1000..1000 |
| `BonusDefencePierce`, `BonusDefenceSlash` | Defense against pierce and slash separately. `BonusArmor` sets both at once |
| `OffsetFrontLineByY`, `OffsetWingsByX`, `OffsetWingsByY` | How far the front line and the wings stand from the hero |
| `<Class Name="…" …/>` | Where units of that class go: `FrontLine="1"` in front, `Wings="1"` on the sides, `CentralBlock="1"` in the middle, around the hero |

`BonusAttack` appears in one of the game's formations but does nothing: don't count on it.

**2. The buttons** (`patches\data\commands\Hero.xml`). Two orders: `form-<name>-menu` (the button you see when that is the current formation) and `form-<name>` (the one that selects it). Both carry `param="<Name>"` and `menu="1"`. Copy `form-front-menu` and `form-front` and change `name`, `param`, `key` and `button` (the image; see [Images](images.md)).

The key must not clash with another hero order, and the hero also inherits the orders of `Unit`. In `data\commands` (extract it with `extract-data.js`, see [Patches](patches.md)) search for `key="x"` in `HERO.XML` and `UNIT.XML`. The example uses `i`, which is in neither; `q`, for instance, belongs to `reveal`.

**3. A patch for the script that lets the buttons through** (`patches\data\subai\hero_formation_verify.vs`). The game script only lets you pick a formation when one of its four menus is open, named in a list. Add yours:

```
@replace if (cmd_name != "form-front-menu")
if (cmd_name != "form-front-menu")
	if (cmd_name != "form-wedge-menu")
@end
```

If another mod patches the same line to add another formation, the patches stack: each `@replace` rewrites the game's line and leaves yours behind.

## Online

The bonuses change gameplay: every player must have the same mod.
