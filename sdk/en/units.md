# New units

A new unit is three things: a class (the stats), a command to recruit it in a building, and its images and texts. It's all
mod files; no programming needed.

By default the unit reuses a game unit's model and animations. If you want your own art, see [Custom graphics](graphics.md):
convert your PNGs to `.rle` and generate the `.ent.xml`.

Ready-to-copy example: [`examples/unit-example`](examples/unit-example), the Roman Veteran: stronger than the hastatus, recruited
at the Republican Rome barracks. With custom sprites: `examples/unit-graphics-example` (see [Custom graphics](graphics.md)).

## Quick way

You need the game's `DATA` folder ([`extract-data.js`](tools/extract-data.js)). The example's Veteran comes from:

```
node tools/create-unit.js <DATA> <mod folder> RVeteran --base RHastatus --health 260 --damage 20 --armor 14 --building RBarracks --gold-cost 180 --time 9000 --key v --name Veterano --name-en Veteran
```

It generates, inside the mod folder:

- `data\classes\RVeteran.sc.xml`: a full copy of the base class, with your id and stats.
- `data\commands\RVeteran.xml`: the recruit command, copied from the base's.
- `loc\spanish.loc.xml` (and `english.loc.xml` with `--name-en`, `italian.loc.xml` with `--name-it`): name, plural and button text. If the file already exists, it adds the entries.
- `mod.json`, only if there wasn't one.

| Option | What it does |
|---|---|
| `--base` | The game unit to copy (required). Takes its id or its `altid` |
| `--health`, `--damage`, `--armor`, `--speed` | Stats. `--armor` sets `armor_slash` and `armor_pierce`. Without them, the base's |
| `--building` | Where it's recruited. Without it, wherever the base is recruited (if several, the one of its own civilization) |
| `--gold-cost`, `--food-cost`, `--time` | Recruit price and time (in milliseconds) |
| `--key` | The button's key. Without it, a letter from the name that's free in that building |
| `--name`, `--plural` | Spanish name. The plural is guessed; fix it with `--plural` if it's wrong |
| `--name-en`, `--plural-en`, `--name-it` | English and Italian name. `display_name` is the text key, so with `--name-en` the key is the English name |

If something's wrong (duplicate id, a base that isn't a unit, a building that doesn't exist) it says why and writes nothing. It doesn't overwrite existing files either.

Limits:

- It only copies units the game recruits with a normal command (`method="train"`). Units hired in groups (gladiators, Vikings…) are out.
- Portrait and button are the base's. For your own, see [Images](#3-images) and change the paths in the class and the command.
- The AI won't recruit the unit.
- Run the result through the [validator](validating.md) with `--data`. What it generates is identical to the example (except the images).

## What it does under the hood (doing it by hand)

If you'd rather write it yourself, or need something the tool doesn't cover, this is what it generates.

## 1. The class

`data\classes\RVeteran.sc.xml`. Start from a full copy of a similar game class, such as `RHastatus.sc.xml`. To see the
game files, extract `data.pak` with [`extract-data.js`](tools/extract-data.js): `node extract-data.js "<game folder>"
<output folder>` puts them in `<output folder>\DATA`. Change the `id` and the stats:

```xml
<class id="RVeteran" cpp_class="CVXUnit" parent="Melee" entity="Units/RHastatus/RHastatus.ent.xml">
	<properties race="RepublicanRome"/>
	<properties maxhealth="260"/>
	<properties damage="20" armor_slash="14" armor_pierce="14"/>
	<properties speed="60"/>
	<properties unit_specials="Deflection"/>
	<properties icon="gameres/icons/RVeteran.bmp"/>
	<properties display_name="Veteran" display_name_plural="Veterans"/>
	...
</class>
```

- Copy it whole; don't make it a child (`parent="RHastatus"`). The game doesn't inherit the entity (`entity`, the model) or the sounds from the parent, so
  a child without its own `entity` spawns without a model and vanishes.
- `entity` points to the model and animations. Here they're the hastatus's.
- `race` is the civilization: `RepublicanRome`, `Gaul`… or one from your mod ([Civilizations](civilizations.md)).
- The usual stats are `maxhealth`, `damage`, `armor_slash`, `armor_pierce`, `speed`, `sight` and `range`. What each one does, and
  which ones are useless, is in [Unit and building stats](stats.md). A unit copied from a normal one inherits `feeds=1` (it eats); siege
  machines and the game's sentries set it to 0.
- `unit_specials` are the [unit specials](specials.md), the game's and your own.
- Don't copy the original's `altid`. It's an alias scripts use to find a civilization's "soldier"; two classes with the
  same alias get mixed up.
- `edittree_pos` (for example `Units/Roman/Veteran`) is where it shows in the map editor and in the cheats menu: a class with an `entity`
  whose `edittree_pos` starts with `Units/` shows there, in its civilization's group. The tool copies it from the base and changes the
  last name.
- A new file in `data\classes\` loads on its own. No registration needed.

## 2. The recruit command

`data\commands\Veteran.xml`. It's a copy of the game's command (`trainRHastatus`, in `data\commands\RBARRACKS.XML`) with a different class and
cost. The game loads every `.xml` in `data\commands`.

```xml
<cmd name="trainRVeteran" priority="3"
	button="actions/train RVeteran.bmp" queueicon="gameres/icons/RVeteran.bmp"
	key="v" costgold="180" costfood="0" costpop="1" execdelay="9000"
	rollover="Equip Veteran" method="train" param="RVeteran" sclass="RVeteran" ...>
	<src obj="RBarracks"/>
</cmd>
```

- `<src obj="RBarracks"/>` is the building where the button appears. For another building, another class (`RBlacksmith`, `RArena`…).
- `param` and `sclass` are the class being recruited. `costgold`, `costpop` and `execdelay` (milliseconds) are price and time.
- `key` is the key. It shows up under Controls → Content, and is changed there ([Mod keys](keys.md)).
- A new building works the same way: a full copy of a building class, plus its commands with `<src obj="MiEdificio"/>`.

## 3. Images

The button is 153 x 51 and the portrait 65 x 76. Build them from a PNG with `tools/convert-image.js` ([Custom images](images.md)). Without
custom images, use the game unit's (`button="actions/train RHastatus.bmp"`, `icon="gameres/icons/RHastatus.bmp"`).

## 4. Texts

The class name and the button text are translated with a `loc\<language>.loc.xml` ([Languages and texts](languages.md)):

```xml
<translationtableentry text="Veteran@RVeteran class name" result="Veterano"/>
<translationtableentry text="Veterans@RVeteran class name plural" result="Veteranos"/>
<translationtableentry text="Equip Veteran@trainRVeteran" result="Equipar veterano"/>
```

## Multiplayer and limits

- Classes and commands are gameplay: all players need the same mod.
- The AI won't recruit your unit unless you ship your own AI scripts. See [Civilizations](civilizations.md#the-ai).
- A unit from a new civilization also needs its buildings and commands. The cloning tool in [Civilizations](civilizations.md)
  generates them.
