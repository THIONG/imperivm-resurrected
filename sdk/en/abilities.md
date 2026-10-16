# Troop abilities

Any unit, regular or hero, can have new abilities. An ability is a command (the button) plus a class
method (the script that runs it), like the druids' and priests'. It's all mod files.

Ready-to-copy example: [`examples/ability-example`](examples/ability-example), "Respiro" (key `Y`) for melee soldiers.

## Quick way

`create-ability.js` writes all three pieces. You need the game's `DATA` folder ([Game data](README.md#game-data)):

```
node tools/create-ability.js <DATA> mods/mi-mod respiro --class Melee --key y --energy-cost 5 --name Respiro --name-en "Second wind"
```

| Option | What it does |
|---|---|
| `--class` | The class that gets the button. A base class works (`Melee`, `Military`, `Unit`) and its children inherit it, or a specific one (`RHastatus`, `Hero`). It must exist in the game |
| `--key` | A letter |
| `--energy-cost` | Energy from 1 to 9. If you set it, the script already charges it |
| `--name`, `--name-en` | The name in Spanish and English. Without `--name-en`, the button shows the id, or the name if it has no accents |
| `--description`, `--description-en` | The help text, same deal |
| `--button` | A game button (`Actions/Mass Healing.bmp`, the default) or your own ([Custom images](images.md)) |

It generates:

```
data\commands\Respiro.xml              the command
patches\data\classes\Melee.sc.xml      the method on the class (added to an existing patch if there is one)
data\mod\respiro.vs                    the script, with the signature and energy charge; you write the body
loc\spanish.loc.xml                    the Spanish texts (added to any that exist)
mod.json                               only if the folder has none
```

It warns you if the key is already used by another command of that class, a parent class or a child class, and lists which keys are free. It generates anyway. It doesn't overwrite files: if the id already exists in your mod or in the game, it stops without writing anything.

Then write the body of `data\mod\respiro.vs` and run the [validator](validating.md).

## What it does under the hood (doing it by hand)

### The three pieces

**1. The command.** A `<cmd>` in a new file in `data\commands` (the game loads every `.xml` in that folder):

```xml
<cmd name="respiro" priority="10" offset="1"
	button="Actions/Mass Healing.bmp" key="y"
	rollover="Second wind" description="..."
	groupverifier="" groupdispatch="" voidgroupdispatch="1"
	coststamina="5">
	<src obj="Melee"/>
</cmd>
```

- `name` is the command's name and also the method's `sig`.
- `button` is the button image: one from the game, or your own ([Custom images](images.md)).
- `<src obj="…"/>` are the classes that get the button. A base class works, and its children inherit it: `Melee` (melee), `Military`,
  `Unit` (all) or a specific one (`RHastatus`). Careful with `Military`: it includes heroes, and the `Y` key is already used by the hero's
  `skill_battle_cry` ability. `Melee` doesn't clash.
- `coststamina` is the energy it costs. A game soldier's `stamina` bar is 10, so the example's 5 spends half. The game
  doesn't charge it by itself: your script does. Use a value from 1 to 9. The tooltip shows the cost as energy if it's under 10 and as loyalty if it's 10 or more.
- `key` is the key. It shows up under Controls → Content, and is changed there ([Mod keys](keys.md)).

**2. The method.** In the class, with a [patch](patches.md) so you don't replace the whole file. It goes in `patches\data\classes\Melee.sc.xml`:

```xml
<patch>
  <add select="/class"><method sig="respiro" vs="data/mod/respiro.vs"/></add>
</patch>
```

Child classes inherit methods.

**3. The script.** `data\mod\respiro.vs`. An instant command receives `// void, Obj me`, the unit running it:

```
// void, Obj me

Unit this;
int cost;

this = me.AsUnit();
if (!.IsAlive)
	return;

cost = GetCmdStaminaCost("respiro");
if (.stamina < cost)
	return;
.SetStamina(.stamina - cost);
.Heal(60);
```

Targeted commands (a click on another unit) receive `// void, Obj This, Obj target`. See how the game's do it in
`data\subai\druid_*.vs` and `data\commands\DRUID.XML`. All the functions are listed in [funciones-del-juego.txt](game-functions.txt).

### Name and description

The command is translated using its name as context. In a `loc\<language>.loc.xml` ([Languages and texts](languages.md)):

```xml
<translationtableentry text="Second wind" context="respiro" result="Respiro"/>
<translationtableentry text="The soldier recovers 60 health (costs stamina)" context="respiro" result="El soldado recupera 60 de vida (gasta energía)"/>
```

The `text` is the command's `rollover` and `description`. [New units](units.md) shows the other form (`text="…@context"`); `languages.md` says
both work.

## Passive abilities

An ability without a button (healing on kill, extra damage, an aura…) doesn't need a command. Use an [event](scripts.md) (`OnObjectKill`, `OnObjectDie`) or
a `<behavior>` in the class.

## Multiplayer

Follow the rules in [Scripts](scripts.md#multiplayer): don't read anything local. All players need the same mod.

## What you can't do

- Create new native unit specials (`unit_specials`) or hero abilities (`HeroSkills`) with their own effect: the game provides their effect. You can use and combine the 25 that exist. The script commands above cover almost everything else, and custom unit specials
  go another way ([Unit specials](specials.md)).
- New visual effects and sounds: reuse the game's.
