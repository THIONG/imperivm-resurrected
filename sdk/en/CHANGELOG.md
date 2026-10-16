# Changes

The SDK version and which mods you can make with each one. The script API version (`Mod_ApiVersion()`) is separate: it's currently 6.

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
