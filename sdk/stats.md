# Unit and building stats

Anything without a tag comes straight from the game data. *(inferred)* is a guess about how the game uses that value, and *(partial)* is the least reliable.

Stats are `<properties>` attributes in the class (`data\classes\<Class>.sc.xml`). To change them without copying the whole class, use a [patch](patches.md); to create a unit, see [New units](units.md).

```xml
<patch>
  <!-- Legionaries run faster: speed 60 -> 75 -->
  <set select="/class/properties[@speed]" speed="75"/>
</patch>
```

It goes in `patches\data\classes\RHastatus.sc.xml`. An attribute the class doesn't set is inherited from its parent.

## Which attribute does what

| I want to change… | Attribute | Notes |
|---|---|---|
| Movement speed | `speed` | The walk animation adapts by itself (inferred) |
| Attack rate | no attribute | Length of the attack animation, or `SetSpeedFactor` from a script |
| Archer range | `range` | |
| Minimum range (catapults) | `min_range` | |
| Real size (hitbox) | `radius` | Collision, paths and range (inferred) |
| Click area | `selection_radius` | Cosmetic (inferred) |
| Damage | `damage` | |
| Armor | `armor_slash` | `armor_pierce` does nothing (partial) |
| Health | `maxhealth` | |
| Sight | `sight` | |
| What gets attacked first | `target_factor` | |
| Projectiles | `projectile_class` | With limits, see *Range* |

### Speed (`speed`)

- Map units per second. In the game: infantry 60-80, archers 60-160, cavalry 130-190, heroes 120, rams 40, ships 240.
- The walk animation cycle lasts `step × 1000 / speed` milliseconds, with `step` taken from the `.ent.xml` (Hastatus: 42 × 1000 / 60 = 700 ms).
  Changing only `speed` doesn't desync the sprite: the animation speeds up or slows down with the movement. *(inferred)*
- `speed="0"` on a unit that walks freezes the game with a division by zero. So does a class that neither defines `speed` nor inherits it. *(inferred)*

### Attack rate

There's no "attack speed" attribute for units. *(inferred)*

- A blow lasts as long as the attack animation, the sum of the `<frame duration>` values in the `.ent.xml`, and the damage lands at the animation's `action_time`. Across the game's 118 animations, blows last 594 to 1584 ms, with a median of 990 ms (Hastatus: 994 ms, `action_time` 426).
- `attack_delay` is useless for units: only `Catapult` (500) and `FakeTower` (400) set it, and nothing else in the data does. *(inferred)*
- There are two ways to attack faster. One: shorten the attack animation's frames and rescale `action_time` by the same ratio.
  The other: `Unit::SetSpeedFactor(150)` from a script, which speeds up both walking and attacking by 50%. `SetSpeedFactor(0)` freezes the game. *(inferred)*
- To change the `.ent.xml` from a mod, an XML patch should work. A loose `.ent.xml` doesn't replace the game's ([Custom graphics](graphics.md)).
- Attack rate and blow timing are simulation: they must be the same for every player.

### Range (`range`, `min_range`)

- Game values: infantry 15-40, archers 300-500 (ships reach 700), sentries 600, cavalry 15-40, catapults 700-800, towers 600-800. `RCatapult` has `min_range=201`.
- It's measured edge to edge: the distance between centers minus the `radius` of both units. That's why `radius` counts too. *(inferred)*
- A melee unit becomes ranged with `range` plus a `projectile_class` (Arrow, Javelin, Axe, Slingstone) and an attack animation with `action_time`.
- With `range` below `min_range` it never attacks. With `range="0"` on a melee unit, it doesn't either. *(inferred)*
- A projectile takes as long as its animation, whatever the distance (arrow 495 ms, axe and javelin 561, sling stone 300). There's no projectile speed attribute. An attacker with a projectile does half damage to buildings. *(inferred)*
- Catapults: the arc comes from `shot_tan` and `shot_height`. Don't change them on one computer only.

### Hitbox and size

| Attribute | What it does | Game values |
|---|---|---|
| `radius` | Physical size: collision, paths, spacing and range (inferred). Simulation | Infantry 10, cavalry 20, heroes 20-30, rams 80, catapults 50 |
| `selection_radius` | Selection ring and click area. Cosmetic as long as the unit has a `formation_radius` (inferred) | |
| `formation_radius` | Spacing between slots in a formation. Simulation | Infantry 26, cavalry 35, heroes 45 |

Always set `radius` on a new building. The inherited one (`Building`: 600) is huge, and attackers "reach" the building from very far away. Real buildings use 160-250 for barracks, 47 for towers, 80-110 for walls and 333-345 for outposts.

### Damage and armor

Constants from `data\const.ini`, section `[GamePlay]`: `MinDamage=3`, `MinPercentOfAttackersDamage=20`, `DamageAmplify=200` and `LevelDifferenceDamageTable`. How they're used is *(inferred)*:

- A blow does `damage − armor_slash` of the defender, with a floor of 20% of the attacker's damage and never less than 3. Then the level difference between the two is applied: it runs from −90% to +90%, and if the defender has the higher level, it subtracts.
- `armor_pierce` isn't part of the calculation: you can write it and it does nothing *(partial: a script may use it)*. To give armor, use `armor_slash`.
- `damage_type` (`slash`, `pierce`, `siege`, `none`) doesn't change the damage: it decides what the unit can attack. `slash` and `pierce` attack units (and buildings if there's a projectile), `siege` attacks buildings too, `none` doesn't attack.
- Buildings have no armor: the damage goes in whole.
- `DamageAmplify` is the damage to a building's garrison. Tweak these constants with [INI patches](patches.md).

### Other attributes

| Attribute | What it does |
|---|---|
| `maxhealth` | Health. Infantry 160-3000, heroes 1000-1300, outposts 10000. `0` freezes a building or spawns the unit dead *(inferred)* |
| `maxstamina` | The energy that unit specials and [abilities](abilities.md) spend. Every soldier in the game has 10. With less than the cost, the special doesn't activate *(inferred)* |
| `sight` | Sight: reveals the fog (inferred). Infantry 500, sentries 600, civilians 150 |
| `target_factor` | What gets attacked first: lower means sooner. Default 100, heroes 200, civilians 1000. Negative = never picked on its own (wildlife uses -1) *(inferred)* |
| `unit_specials` | [Unit specials](specials.md), the game's and yours |
| `feeds`, `max_food` | `Unit` inherits `feeds=1` and `max_food=20`: every normal unit eats. At 0 food they lose health *(inferred; when exactly, unclear)*. `RamUnit`, `Sentry`, `GGhost` and the animals have `feeds=0`; set it on machines and magical creatures |
| `does_not_regenerate` | `1`: no food and no regeneration. Useful for summons and machines *(inferred)* |
| `stamina_dec_time` | On summoned units: milliseconds between each loss of 1 energy. They live ≈ `(maxstamina+1) × this value`; with `0` or less they die instantly *(inferred)* |
| `water_unit` | Ships and fish: water paths and targets |
| `desync_hash` | `1`: this object's movement goes into the sync check. Set `0` on new visual effects *(inferred)* |

## What does nothing

`armor_pierce` *(partial)*, `speed_modify` and `taunt` (as a property it's a sound name) are read and never used. *(inferred)* `attack_delay` is useless on units. `target_priority` only matters for towers and catapults; for units, use `target_factor`.

## Changing things mid-game (from a script)

| What | How |
|---|---|
| Health, max health, stamina, max stamina | `SetHealth`, `SetMaxHealth`, `SetStamina`, `SetMaxStamina`. The game recalculates the maximums when an item is equipped or a level is gained, and overwrites your value |
| Sight | `SetSight(n)`, from 100 to 2048 |
| Speed and attack rate | `Unit::SetSpeedFactor(n)`, as a percentage (100 = normal). `0` freezes |
| Level, experience, unit specials | `SetLevel`, `SetExperience`, `Mod_SetSpecial` |
| Damage, armor, range or radius of one specific unit | No function. Create another class with those values and convert the unit with `Unit::Mutate("OtraClase")`, which keeps health, experience and orders |

The functions are listed in [funciones-del-juego.txt](game-functions.txt). To change the damage of a single blow, use the `OnAttackResolved` event with `Mod_SetAttackDamage` ([Unit specials](specials.md)).

## What XML can't do

- The combat formula (flat armor, floors, level table). You can only adjust the `const.ini` constants.
- Projectile speed and flight range: the blow lands if the distance is below `range + 150`. *(inferred)*
- New code behaviors: the `cpp_class` values are only the ones the game ships with.
- Formation logic, beyond what [Formations](formations.md) allows.

## Multiplayer

Almost everything above is simulation. Every player must have the same mod, and the game checks this when entering a lobby; a difference between two players desyncs the game and it's aborted. Cosmetic: `selection_radius`, `healthbar_*`, `icon`, `display_name`, `help` and the sprites.

Sprites (`.rle`) aren't part of the check. The full list of what is checked is in the [README](README.md).

## What we don't know

- The exact conversion between map units and screen pixels.
- Whether `sight` also limits how far a unit looks for targets. It reveals the fog and scripts use it to search, but the exact calculation is unknown.
- Which `formation_priority` goes in front in a formation.
- What happens with an `action_time` longer than the animation. Avoid it.
