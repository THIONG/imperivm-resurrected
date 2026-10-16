# Civilizations

The game ships with 8 civilizations, numbered 0 to 7: Gaul, Republican Rome, Carthage, Iberia, Imperial Rome, Britain, Egypt and Germany.

Imperivm Resurrected lets you declare new civilizations, numbered from 10 up. Each one inherits everything your mod doesn't define from a game civilization (its
*base*). The tools need Node.js.

Ready-to-copy example: [`examples/civilization-example`](examples/civilization-example).

## Declaring a civilization

A `civs\<id>.json` file in your mod:

```json
{
  "id": "dacia",
  "index": 10,
  "base": "Gaul",
  "name": {"spanish": "Dacia", "english": "Dacia", "italian": "Dacia"},
  "letter": "D"
}
```

| Field | What it is |
|---|---|
| `id` | The name: letters, numbers and `_`, starting with a letter, up to 24 characters. You use it in `race="..."` and in scripts. |
| `index` | A number from 10 to 99. Don't change it once published: saved games remember it. Two mods can't use the same one. |
| `base` | The game civilization it inherits from: `Gaul`, `RepublicanRome`, `Carthage`, `Iberia`, `ImperialRome`, `Britain`, `Egypt` or `Germany`. Defaults to `Gaul`. |
| `name` | Its name in each language, or a single string. |
| `letter` | The letter that prefixes its classes and commands (`DSwordsman`, `trainDSwordsman`). If you leave it out, it uses the base's. Set your own only if you have classes and commands with that letter: the game builds names from it, such as the peasant training one. |
| `template` | (Optional) Prefix of your city and village templates, so the random map doesn't use the base's. See [Templates](city-templates.md). |
| `internalName` | (Optional) The name the game uses to find the civilization's interface files; defaults to the base's. |
| `folder` | (Optional) The name shown in selection texts. |
| `random` | (Optional, defaults to `true`) Whether it's included in the random-civilization draw, also on random maps. The AI only draws it if it also has `"ai": true`. Set `false` to exclude it. |
| `biome` | (Optional) A game civilization: the terrain, trees and objects the random map places around its start are that civilization's. Without it, the base's. See *The biome*. |
| `ai` | (Optional) With `true`, the AI plays with your classes and commands. Otherwise it plays as the base civilization. The cloning tool sets it to `true`. See *The AI*. |
| `ownAi` | (Optional, with `ai`) For mods that ship all their own AI scripts and want to see the civilization as itself (`Dacia`, not its base). |
| `alias` | (Optional, written by the cloning tool) `classes` and `commands`: name in the base to name in your civilization (`"GArcher": "DArcher"`, `"trainGSwordsman": "trainGSwordsman Dacia"`). The AI uses them to translate. |

With just `id`, `index` and `base` you get a copy of the base under another name. From there, change whatever you like.

## Using it in your classes and scripts

In each unit's or building's `.sc.xml`:

```xml
<properties race="Dacia"/>
```

The game only knows the names of its 8 civilizations. The mod recognizes your civilizations' `id` (case-insensitive) and sets it on the
class. If you set `internalName` by hand, that works too. The default one (the base's) doesn't count, so `race="Gaul"` is still
Gaul even if you have a civilization cloned from it.

A class with `parent=` inherits stats and behaviors, but not the entity (`entity`, the model) or the sounds: set them again or
the unit spawns without a model and vanishes. That's why the cloning tool copies whole classes.

In scripts, each declared civilization is a constant named after its `id` with a capital first letter:

```
if (GetPlayerRace(player) == Dacia)
	...
```

The game's AI scripts compare against `Gaul`, `RepublicanRome`… and don't know yours. With `"ai": true` they see the base. To find out which
civilization an object really plays as, use `Mod_TrueRace(objeto)`.

## In the lobby and on the map

Declared civilizations appear in the multiplayer lobby's dropdowns, after the game's. On a random map, yours uses its biome's terrain and starts with a city and a village built from its classes
(`DTownhall`, `DBarracks`…). If you don't give it templates, the building layout is the base's.

If something doesn't show up, check `Logs\imperivm\imperivm-mod.log`: the lines tagged `civilizations` say what was registered and what failed (an invalid `id`,
a missing or duplicate `index`, a `base` that doesn't exist). Errors also appear on the mod's card in the manager.

## Cloning a civilization

Writing a civilization's dozens of classes and commands by hand is no fun. `tools/clone-civilization.js` clones one from the game.
It needs the game's `DATA` folder, which you get with [`extract-data.js`](tools/extract-data.js):

```
node extract-data.js "<game folder>" <output folder>
node clone-civilization.js <output folder>\DATA <mod folder> Dacia D Gaul 10
```

It generates `civs\dacia.json`, a copy of each of the base's classes (`DSwordsman` is a copy of `GSwordsman`, with `race="Dacia"`), the commands that
name base classes (`trainGSwordsman Dacia`, `trainpeasantd`, `hireheroD`…) and patches that add the new classes to the
shared commands. It also clones the heroes `GHero1`…`GHero7`, which hang from `GaulHero` and have no `race`: without heroes that have a model, hiring one
crashes the game. From there you edit whatever you want. The sample result is in
[`examples/civilization-example`](examples/civilization-example).

The tool doesn't write the `template` field. If you want your own templates, add it yourself ([Templates](city-templates.md)).

The copies don't carry the originals' `altid`. The `altid` is a unique alias (`Barrack`, `Village`) that scripts and the AI use to look up a
civilization's "barracks". If two classes share one, one civilization's AI stops seeing its buildings. If you write classes by hand, don't copy it.

## The AI

By default, the AI that picks a mod civilization plays as its base. With `"ai": true` it plays with your classes and commands. The game's AI scripts
decide based on the civilization of the central building and build names from it, so the game makes them see the base and
translates what they ask for:

- The commands and research they ask for by name (`trainGSwordsman`, `Gaul Axeman`, `hireheroG`…) are mapped to your civilization's, according to
  `alias.ordenes`, only on objects of an AI of your civilization.
- Your classes count as the base's: `DArcher` is a `GArcher` for counts, barracks, temples and counter-units.

The cloning tool fills in both tables. If you write your classes by hand, fill them in yourself. Nothing changes for the human player.

AI limits:

- Your AI behaves like the base's, unless you give it another tactic.
- A unit that isn't in the base's six slots won't be recruited.
- New research (not cloned from one of the base's) is unknown to it.
- The costs it reads before paying are the base's, even if the command charges your values.

**A different tactic for your civilization.** The game picks each AI's tactic at start, in `data\ai\GetTacticScript.vs`. A
[script patch](patches.md) can add a piece to it for your civilization. Since the AI sees the base, ask for the real civilization with
`Mod_TrueRace(set.GetCentralBuilding)`. The example [`examples/ai-tactic-example`](examples/ai-tactic-example) makes Dacia use a game tactic
that no AI uses. Patches from several mods add up, so two civilizations can pick their tactic at the same time.

With `"ownAi": true` the mod has to ship all the AI scripts, because the game's see civilization 10 and their tables don't know it.
Normally you leave `"ai": true` and change only the tactic.

## The biome

On a random map, what surrounds each player's start (grass, sand, trees, decorations) is decided by their civilization. Yours uses its
`base`'s, or whatever you set in `biome`:

```json
{ "id": "dacia", "index": 10, "base": "Gaul", "biome": "Britain" }
```

The game only has four terrain-and-tree biomes, so civilizations share them in pairs:

| Terrain and trees | Used by |
|---|---|
| 1st | Gaul and Imperial Rome |
| 2nd | Republican Rome and Britain |
| 3rd | Carthage and Egypt |
| 4th | Iberia and Germany |

Map objects (ruins, palm trees, rocks) do tell all eight apart; Carthage and Egypt act as a "desert" versus "temperate" switch.
The starting city is still the base's unless you use `template`. A fully custom biome, with new terrain, isn't possible.

## What you can't do

- Games against the AI outside the multiplayer lobby (scenarios and campaigns) with mod civilizations.
- Map editor: your civilization's classes show up mixed into its base's folders.
- New AI scripts without shipping all of them (see *The AI*).
