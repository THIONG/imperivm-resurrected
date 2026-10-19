# Changes

The SDK version and which mods you can make with each one. The script API version (`Mod_ApiVersion()`) is separate: it's currently 7.

## 1.6

Needs Imperivm Resurrected 0.32.24 or later.

- A variant that replaces its base (`"replacesBase": true`) can change with `letter` and `template` the letter and the town and
  village templates of that base for the whole game, also those of the neutral settlements of the random map. This is how a whole
  civilization of the game is replaced ([Civilizations](civilizations.md#variants)). The validator also looks for the templates in
  the mod's `packs\randommapsettlements.bfhp`.
- `modes\<name>.json` fixes the room settings and the civilizations of a victory mode ([Victory modes](victory-modes.md)).
- `rules\game.json` fixes engine rules for all the games of the mod: the army stays with its hero at the walls, sentries can't be
  selected and some classes can't be attacked ([Options](options.md)).
- A bonus script numbered `004` to `099` in `data\BonusScripts` appears in the lobby, with the name and description from its `//@` lines ([Starting bonuses](bonuses.md)).
- The validator knows `Mod_PlayerCiv` and `Mod_CanExecCmd`.

## 1.5

Needs Imperivm Resurrected 0.32.20 or later.

- `shrink-mod.js` also converts mods whose `rle.mmp` changes the game's graphics, not only those that append graphics at the
  end. The game's `rle.mmp` stays whole: what the mod changes or adds goes after it, in `rle.mmp.tail`, and its images are
  adjusted to find it in its new place. The game's images that the mod does not replace still look as in the game.
- `shrink-mod.js` keeps closer to what the game showed with the old mod: a loose file in `Currentlang\` wins over the one in
  the language package, and a loose file identical to the one in the game's packages is kept if some language showed another one.
- The validator no longer gives an error for two commands with the same name for different buildings, because the game binds each
  one to the buildings of its `<src>`; it only warns when both are for the same building. It no longer asks for the signature on
  the first line of victory modes (`data\GameScripts`) or item scripts either: the game passes it.

## 1.4

Needs Imperivm Resurrected 0.32.20 or later.

- A mod can show up in several categories of Browse content: the main one is its GameBanana category and the others are added
  with `Type` tags, for example `Type: Civilizations`.
- Tags have a title, as on GameBanana: `Players: 2-4` and `Language: Spanish`. The forms without a title still work and the
  validator asks for the new one.
- In the game, the categories have the same names as on GameBanana: **Gameplay & Balance** (was Rules and balance),
  **Graphics** (was Graphics and interface) and **Translations** (was Texts and translations).

## 1.3

Needs Imperivm Resurrected 0.32.13 or later.

- The map categories are named after the game's folders: **Scenarios**, **Adventures** and **Conquests**, instead of Maps,
  Campaigns and Conquest. The previous names still work and the validator asks for the new one.
- A loose `.bfhp` at the root of the `.zip` is placed, when installing, in `scenarios\`, `adventures\` or `conquests\` by the kind the map
  itself states (`game_type`); the category only decides whether a normal map is an Adventures mission. The validator warns about maps in a
  folder that isn't theirs.
- Audio is published in GameBanana's **Sounds** section, as GameBanana asks, and Browse content shows it under Audio & Voices. What you
  install from there keeps its number in `sources.gamebananaSound`.

## 1.2

Needs Imperivm Resurrected 0.32.10 or later.

- `Mod_CanExecCmd(object, command)` (API 7): the same check a command's button makes (the object has it, it isn't disabled and its
  verifier accepts it). The cheats menu lists the troops of the loaded classes, mods' included (`edittree_pos` under `Units/`), and
  researches what the buttons allow, in the order of their requirements and without picking two options of one choice.
- Civilizations: `"variant": true` declares a variant of a game civilization, with the same classes and another name in the lobby;
  with `"replacesBase": true` it takes its base's place in the lobby and in the random draw.
  `GetPlayerRace` returns the base and `Mod_PlayerCiv(player)` (API 7) the chosen one ([Civilizations](civilizations.md#variants)). The
  interface of a mod civilization (bars, corners, quick icons, end screen) is its base's, not Gaul's. The script constant keeps the
  capital letters of the `id` (`RomeImperial`).
- A `.bfhp` in `packs\` shipped by a mod (for example `RandomMapSettlements.bfhp`, the random map starting towns) is really used: the
  game opens it for writing and used to keep its own.
- The loose files in `Local\<language>\` (texts, help, tips) are always used: they used to be lost if the game read them before the mods
  were mounted.
- Old mods: `shrink-mod.js` keeps the order of their packages, puts in `hidden.txt` what they removed from the game, writes `rle.mmp.tail` if they extend `rle.mmp` and `Local\<language>.pak` for language files outside `currentlang\`, and turns `data\const.ini` into a patch without `[Resolutions]`. New options `--exclude` and `--hide`. Cutscenes are also looked up in the packs.
- Nine GameBanana categories in three kinds: Gameplay & Balance, Units & Heroes, Civilizations · Maps, Campaigns, Conquest · Graphics, Audio & Voices, Translations. `Scenarios`, `Interface` and `Total Conversions` still work as Maps, Graphics and Gameplay & Balance, but the validator asks for the new name.
- Tags are only used for the players filter (`4 players`, `2-4 players`, on maps) and the language filter (`Spanish`…, on campaigns, translations and audio). `Gameplay`, `Cosmetic` and `Online tested` are no longer needed. The validator warns about a map without players and about a campaign, translation or audio pack without a language, and an unknown tag is now a warning, not an error.
- The manager no longer has a Publish button: you make the zip yourself (see [Publishing](publishing.md)).
- The game works out the order of the mods ([Reference](reference.md#1-mod-structure)): it's no longer set by hand in the profile, and the multiplayer check doesn't depend on the order. The `imperivm-mod.ini` key is `[Mods] Profile`.

## 1.1

Needs Imperivm Resurrected 0.31.58 or later.

- The field names of `mod.json`, `civs\*.json`, `specials\*.json` and `options\*.json` are now in English (`name`, `authors`, `description`, `tags`, `category`, `index`, `biome`, `template`, `letter`, `values`, `default`…). The Spanish names still work.
- The directives of script patches (`.vs`) are now `@top`, `@bottom`, `@before`, `@after`, `@replace` and `@end`. The old ones still work.
- The keys of `imperivm-mod.ini` are now in English (`[Debug]`, `Dump`, `LogFiles`, `[Mods] Override`…). The game renames the old ones at startup.
- The log moved to `Logs\imperivm\imperivm-mod.log` (the game deletes the loose files of `Logs\` at startup). Its lines carry a level (`I`, `W`, `E`) and a tag (`patch`, `language`, `civilizations`, `specials`, `script`, `event`) instead of a text prefix such as `Patch:`, and the last five sessions are kept. `LogLevel` and `LogChannels` under `[Debug]` in `imperivm-mod.ini` ask for more detail. The heart in the main menu creates a report with the logs to share.

## 1.0

Needs Imperivm Resurrected 0.29.5 or later.

With files alone and nothing to compile, a mod can:

- Change any XML, INI or script file of the game with patches, and merge the changes of several mods.
- Add units with their stats, and custom graphics converted from PNG.
- Add troop and hero abilities, and unit specials.
- Add civilizations, with their starting city and village, AI and heroes.
- Add neutral outposts.
- Add items.
- Add victory modes, with their limits.
- Add game options to the lobby: switches and value lists.
- Add random map types.
- Add hero formations.
- Add languages and texts, custom commands and buttons.
- React to what happens in the game with script events and `Mod_*` functions.

Tools: validate a mod, extract the game data, create a unit, an item, an ability, a formation or a game option, clone a civilization, create an outpost, city templates, map types, convert images and sprites, create the entity of a unit or building, adapt an old mod.
