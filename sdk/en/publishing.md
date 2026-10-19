# Publishing a mod on GameBanana

Imperivm Resurrected mods are published at https://gamebanana.com/games/26069. The **Browse content** tab in the content manager reads that page: search, filter, view a mod's page and install with one click. To make your mod show up under the right type and filters, follow these steps.

## Before you upload

1. Set the category in your `mod.json` (one from the table below) and, if needed, its [tags](#tags): more categories if your mod does several things, the players if it's a scenario and the language if it's an adventure, a translation or an audio pack:

   ```json
   {
     "id": "duelo-en-el-nilo",
     "name": "Duelo en el Nilo",
     "version": "1.0",
     "authors": ["Your name"],
     "category": "Scenarios",
     "tags": ["Players: 2"]
   }
   ```

2. Run it through the [validator](validating.md). It warns you if the category isn't one the game recognizes or uses an old name, if a tag is badly written, if a scenario has no players tag, and if an adventure, translation or audio pack has no language tag.

3. Zip the mod folder, with its `mod.json` inside. Don't include executables or system scripts (`.exe .dll .asi .bat .cmd .com .scr .ps1 .vbs .js .msi .sys .cpl .lnk .reg .jar .hta .wsf .pif`): a mod containing any of them won't load.

## On GameBanana

1. Go to **Add → Mod** from the game's page (`https://gamebanana.com/mods/add?_idGameRow=26069`). Audio is the exception: it goes in **Add → Sound** (`https://gamebanana.com/sounds/add?_idGameRow=26069`). Anything uploaded as Tool or Script won't show up in the game.
2. **Category**: the same one as in your `mod.json`.
3. **Tags**: the same tags. Each one has a title and a value: in `Type: Civilizations` the title is `Type` and the value `Civilizations`. You don't have to create them first: they are written in each mod.
4. **Files**: your `.zip`, and in the **Version** field the same version as in `mod.json`. Every new version is a new file: don't replace the old one, because anyone playing online with the old version still needs it.
5. **Requirements**: if your mod needs another one, link its page (`https://gamebanana.com/mods/...`, or `https://gamebanana.com/sounds/...` for audio). The game installs it along with yours.
6. At least one screenshot. The first one is the image shown in the game.
7. For the changelog, post an **Update** with the version as its title (for example `1.1`). The game shows it next to that version.

Only people who installed the mod from Browse content (that writes `sources.gamebanana` into their `mod.json`, or `sources.gamebananaSound` if it comes from Sounds) get the new-version notice, or people with a mod that has the same `id` as the entry. If someone downloads the zip by hand and unzips it, they get no notice.

## Categories

**In short:** on GameBanana you pick **one category**, the one that best describes the mod overall. Everything else it brings goes in **tags** `Type`, `Players` and `Language`: add all the ones that apply, and only those.

GameBanana allows only one per mod: pick the one it does the most for. If your mod does more, for example a large one that brings civilizations and maps, add the others with `Type` tags (see [Tags](#tags)) and it will show up in all of them. There are nine, in the three kinds the game tells apart. The kind isn't decided by the category but by the mod's files (see [What counts as gameplay?](README.md#what-counts-as-gameplay)); the category is how people find it in Browse content.

| Category | In the game | What goes here | Where its files go | Example |
|---|---|---|---|---|
| **Change gameplay** | | *Everyone online must have them* | | |
| **Gameplay & Balance** | Gameplay & Balance | Rules, stats, costs, AI; also big mods that change the whole game | `data\`, patches | [patches-example](examples/patches-example), [scripts-example](examples/scripts-example), [custom-orders](examples/custom-orders) |
| **Units & Heroes** | Units & Heroes | New or changed units and heroes | `data\classes\`, `specials\` | [ability-example](examples/ability-example), [specialty-example](examples/specialty-example) |
| **Civilizations** | Civilizations | New or reworked civilizations | `civs\`, `data\` | [civilization-example](examples/civilization-example) |
| **Content to play** | | *Only whoever plays it needs it* | | |
| **Scenarios** | Scenarios | Maps for skirmish and multiplayer | `scenarios\MiMapa.bfhp` | [random-map-example](examples/random-map-example) |
| **Adventures** | Adventures | Great battles, challenges and single-player campaigns, with a story and objectives | `adventures\` | |
| **Conquests** | Conquests | Conquest maps and games | `conquests\` | |
| **Cosmetic** | | *Each player has their own* | | |
| **Graphics** | Graphics | Textures, unit looks, terrain, menus, icons, fonts | the game's package paths, `menuini\` | |
| **Audio & Voices** | Audio & Voices | Music, sounds and voices. Published in GameBanana's **Sounds** section, not in Mods | `sounds\`, `currentlang\voices\` | |
| **Translations** | Translations | The game or a mod in another language, changed texts | `loc\` | [french-language](examples/french-language), [custom-texts](examples/custom-texts) |

For the categories without an example (adventures, conquests, graphics, sound), just copy the file into its folder, with the same name if it replaces a game file.

**Loose maps.** If the `.zip` only holds the `.bfhp`, with no folders, the game places it by itself when installing, by the kind the map itself states (what its author chose in the editor): an adventure goes to `adventures\` (with its `.bmp` of the same name), a conquest to `conquests\` and a skirmish map to `scenarios\`, or to `adventures\` if the category is Adventures (single-player missions made as a normal map). Even so, it is better to upload it already inside its folder; the [validator](validating.md) warns when a map is in the wrong one, for example an adventure in `scenarios\`, which the Scenarios list doesn't show.

**Scenario preview.** When a scenario is chosen in the room, the game draws a preview from the map's own terrain, with its players and their civilizations, and where it comes from: you don't need to upload any image. The colors come from the game's minimap tiles, so a new terrain looks right if it brings its tile in `minimapzoom8	errain`.

**Old names.** Scenarios, Adventures and Conquests are named after the game's folders (`scenarios\`, `adventures\`, `conquests\`). They used to be **Maps**, **Campaigns** and **Conquest**: a `mod.json` with those names, or with `Interface` or `Total Conversions`, still works, but the validator asks for the new name.

## Tags

Each tag has a title and a value. The game uses three:

| Tag | What for | Example |
|---|---|---|
| `Type: <category>` | More categories, besides the main one. One tag per category | `Type: Civilizations`, `Type: Scenarios` |
| `Players: <players>` | Players of a scenario, from 2 to 8. You can add several | `Players: 4`, `Players: 2-4` |
| `Language: <language>` | Language of an adventure, a translation or an audio pack: `Spanish`, `English`, `Italian`, `German`, `French`, `Polish` or `Multilingual`. You can add several | `Language: English` |

Write them exactly as shown, in English, in the `tags` of your `mod.json` (for example `"tags": ["Type: Scenarios", "Players: 2-4"]`) and in GameBanana's **Tags** field: the game displays them in each player's language. Use only these three: the game uses no other, and the validator warns you if there is any other.

A scenario without `Players` doesn't show up when filtering by players, and an adventure, translation or audio pack without `Language` doesn't show up when filtering by language. The same goes for the categories in `Type`: a mod with `Type: Scenarios` needs `Players` to show up when filtering by players.

The forms without a title from earlier versions (`4 players`, `Spanish`) still work, but the validator asks for the new one. The `Gameplay`, `Cosmetic` and `Online tested` tags are no longer needed: whether a mod changes gameplay, the game knows from its files.

## Mods made before Imperivm Resurrected

Older mods were installed by copying them over the game: they ship whole `.pak` packages that replace the game's ones, and most of what's inside is untouched game files. Convert them so they contain only what they change or add:

```
node tools/shrink-mod.js <old mod> <game folder> <new folder> --id mi-mod --name "Mi mod" --version 1.0 --category "Gameplay & Balance"
```

The tool works out what the game sees with the mod copied over it and what it sees without it, and keeps only the difference:

- From the packages, the files that differ from the game's, keeping their order (in a folder the package with the greater name wins, like `zzMod.pak` over `Data.pak`).
- The texts and sounds of each language in `Local\<language>\` (or in `currentlang\` if they are the same in all), and the rest of a language package in `Local\<language>.pak`.
- What the old mod removed from the game by replacing its packages, in `hidden.txt`, so that the game doesn't see it either.
- `data\const.ini` as a patch and without the `[Resolutions]` section: the player picks the resolution in the options.
- If the mod ships its own `rle.mmp`, only what it changes in or adds to the game's, in `rle.mmp.tail`; the game's is left as it is.
- It leaves out `.bak` copies, `Config.ini` and `Settings.ini` (install settings) and executables, with which the mod wouldn't load.

Options:

- `--exclude <patterns>` leaves more files of the old mod out, like install guides (paths with `*` and `?`, separated by commas).
- `--hide <patterns>` hides loose game files that the old mod's readme said to delete (for example `currentlang/*,units/*`).
- `--paths`, instead of the output folder, only shows which folders are inside the packages, without writing anything.

The output folder has to be empty. If the old mod already has a `mod.json`, the tool keeps it, except `sources` (where the original mod is downloaded from), so that version notices don't point to someone else's page. At the end it says what it left out and what it hides.

Test it in the game before uploading it.
