# Custom unit specials

The game ships with 36 fixed unit specials (Parry, Penetration, Deflection…). They can't be extended, but a mod can declare
new ones, and you program their effect with scripts.

Ready-to-copy example: [`examples/specialty-example`](examples/specialty-example), "Critical hit": 25% of hastatus hits
deal triple damage.

## How it works

Each unit carries a 64-bit mask, one bit per special. Bits 0 to 35 are the game's 36. Bits 36 to 63 are yours and are saved with the game. A mod special takes one bit.

There are three steps.

**1. Declare it.** A `specials\<id>.json` file in your mod:

```json
{
  "id": "Critical",
  "bit": 36,
  "name": {"spanish": "Golpe crítico", "english": "Critical hit", "italian": "Colpo critico"},
  "description": {"spanish": "Uno de cada cuatro golpes hace el triple de daño.", "english": "One hit in four does triple damage.", "italian": "Un colpo su quattro fa il triplo del danno."}
}
```

| Field | What it is |
|---|---|
| `id` | The name: letters, numbers and `_`, starting with a letter, up to 24 characters. You write it in `unit_specials` and it's the script constant (`Critical`). It can't be the name of one of the game's 36 (`Triple strike`…). Also avoid ids that, lowercased and with `_` for spaces, match game constants (`triple_strike`, `keen_sight`, `life_steal`…): the validator doesn't check this and the constant would clash with the game's. |
| `bit` | A number from 36 to 63. Don't change it once published: saved games remember it. Two mods can't use the same one. |
| `name` | Its name in each language, or a single string. |
| `description` | It's read, but not shown anywhere yet. Describe it in the unit's help text. |
| `icon` | (Optional) It's read, but not used. |

**2. Assign it to classes.** With the usual `unit_specials` attribute. The game ignores names it doesn't know and the mod sets the bit. Better done with
a [patch](patches.md), so you don't replace the class file:

```xml
<patch>
  <set select="/class/properties[@unit_specials='Deflection']" unit_specials="Deflection, Critical"/>
</patch>
```

Inheritance works as in the game. If a class sets `unit_specials`, its list replaces the parent's; if it doesn't, it inherits the parent's whole list. When
adding one to a class that already has some, repeat the ones it had (`Deflection, Critical`). In the game names are case-sensitive; your mod's aren't.

**3. Program the effect.** The game only stores the bit. You write what it does in a [script](scripts.md):

| Function | What it does |
|---|---|
| `Mod_HasSpecial(Unit unidad, int bit)` : `bool` | Does the unit have this special? Works for any bit from 0 to 63, including the game's. Takes a `Unit`; with an `Obj`, use `objeto.AsUnit()` (if it isn't a unit, returns `false`). |
| `Mod_SetSpecial(Unit unidad, int bit, bool puesta)` | Sets or clears the special on that unit, for example on level-up. It's saved with the game. |

The bit is the constant named after the `id`: `Mod_HasSpecial(attacker.AsUnit(), Critical)`.

The natural place for the effect is the `OnAttackResolved` event, which can change each hit's damage (`events\OnAttackResolved.vs`). Any
other script works too: a `<behavior>` that checks units periodically, `OnObjectKill`, `OnObjectDie`…

```
// void, Obj attacker, Obj victim, int damage
if (!attacker.IsValid) return;
if (!Mod_HasSpecial(attacker.AsUnit(), Critical)) return;
if (rand(100) >= 25) return;
Mod_SetAttackDamage(damage * 3);
```

## Multiplayer

`specials\`, `events\` and `civs\` count as gameplay: all players need the same mod.

## Errors

They show up in `Logs\imperivm\imperivm-mod.log` (lines tagged `specials`) and on the mod's card in the manager: an invalid `id`, a missing or
duplicate `bit`, a name that belongs to a game special.

Warning: if you add an unknown `[Section]` to `UNIT_SPECIALS.INI` (with a patch, to give a special an icon and text),
the game may crash. Don't. Mod specials don't use that file.

## What's missing

- The unit info bar at the top, where the game shows up to three of a unit's specials with their icon, doesn't show mod specials.
- The recruit button's stats text should list them below the game's, without the "Specials" title:
  a unit with only mod specials would show them with no title.
- Custom icons: not shown.
- You can't add to the table of the game's 36 or create new native hero abilities. Active ones are made
  with [commands and scripts](abilities.md).
