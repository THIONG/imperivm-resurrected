# Publishing a mod on GameBanana

Imperivm Resurrected mods are published at https://gamebanana.com/games/26069. The **Browse content** tab in the content manager reads that page: search, filter, view a mod's page and install with one click. To make your mod show up under the right type and filters, follow these steps.

## Before you upload

1. Set the category in your `mod.json` (one from the table below). If it's a map, a campaign, a translation or an audio pack, add its tags too:

   ```json
   {
     "id": "duelo-en-el-nilo",
     "name": "Duelo en el Nilo",
     "version": "1.0",
     "authors": ["Your name"],
     "category": "Maps",
     "tags": ["2 players"]
   }
   ```

2. Run it through the [validator](validating.md). It warns you if the category isn't one the game recognizes or uses an old name, if a map has no players tag, and if a campaign, translation or audio pack has no language tag.

3. Zip the mod folder, with its `mod.json` inside. Don't include executables or system scripts (`.exe .dll .asi .bat .cmd .com .scr .ps1 .vbs .js .msi .sys .cpl .lnk .reg .jar .hta .wsf .pif`): a mod containing any of them won't load.

## On GameBanana

1. Go to **Add → Mod** from the game's page (`https://gamebanana.com/mods/add?_idGameRow=26069`). Always **Mod**: anything uploaded as Sound, Tool or Script won't show up in the game.
2. **Category**: the same one as in your `mod.json`.
3. **Tags**: the same tags, written the same way.
4. **Files**: your `.zip`, and in the **Version** field the same version as in `mod.json`. Every new version is a new file: don't replace the old one, because anyone playing online with the old version still needs it.
5. **Requirements**: if your mod needs another one, link its page (`https://gamebanana.com/mods/...`). The game installs it along with yours.
6. At least one screenshot. The first one is the image shown in the game.
7. For the changelog, post an **Update** with the version as its title (for example `1.1`). The game shows it next to that version.

Only people who installed the mod from Browse content (that writes `sources.gamebanana` into their `mod.json`) get the new-version notice, or people with a mod that has the same `id` as the entry. If someone downloads the zip by hand and unzips it, they get no notice.

## Categories

One per mod: pick the one it does the most for. There are nine, in the three kinds the game tells apart. The kind isn't decided by the category but by the mod's files (see [What counts as gameplay?](README.md#what-counts-as-gameplay)); the category is how people find it in Browse content.

| Category | In the game | What goes here | Where its files go | Example |
|---|---|---|---|---|
| **Change gameplay** | | *Everyone online must have them* | | |
| **Gameplay & Balance** | Rules and balance | Rules, stats, costs, AI; also big mods that change the whole game | `data\`, patches | [patches-example](examples/patches-example), [scripts-example](examples/scripts-example), [custom-orders](examples/custom-orders) |
| **Units & Heroes** | Units & Heroes | New or changed units and heroes | `data\classes\`, `specials\` | [ability-example](examples/ability-example), [specialty-example](examples/specialty-example) |
| **Civilizations** | Civilizations | New or reworked civilizations | `civs\`, `data\` | [civilization-example](examples/civilization-example) |
| **Content to play** | | *Only whoever plays it needs it* | | |
| **Maps** | Maps | Maps for skirmish and multiplayer | `scenarios\MiMapa.bfhp` | [random-map-example](examples/random-map-example) |
| **Campaigns** | Battles and campaigns | Single-player battles with a story and objectives, on their own or chained | `adventures\` | |
| **Conquest** | Conquest | Conquest maps and games | `conquests\` | |
| **Cosmetic** | | *Each player has their own* | | |
| **Graphics** | Graphics and interface | Textures, unit looks, terrain, menus, icons, fonts | the game's package paths, `menuini\` | |
| **Audio & Voices** | Audio & Voices | Music, sounds and voices | `sounds\`, `currentlang\voices\` | |
| **Translations** | Texts and translations | The game or a mod in another language, changed texts | `loc\` | [french-language](examples/french-language), [custom-texts](examples/custom-texts) |

For the categories without an example (maps, campaigns, graphics, sound), just copy the file into its folder, with the same name if it replaces a game file.

**Old names.** GameBanana still has a **Scenarios** category: don't use it (the game shows it as Maps). A `mod.json` with `Scenarios`, `Interface` or `Total Conversions` still works as Maps, Graphics and Gameplay & Balance, but the validator asks for the new name.

## Tags

They're only needed in two cases: they are the players and language filters of Browse content.

| Category | Tag | Filter in the game |
|---|---|---|
| Maps | `4 players`, or a range like `2-4 players` (2 to 8) | Players |
| Campaigns, Audio & Voices, Translations | `Spanish`, `English`, `Italian`, `German`, `French`, `Polish` or `Multilingual` (you can add several) | Language |

Write them exactly as shown, in English, in the `tags` of your `mod.json` and in GameBanana's **Tags** field: the game displays them in each player's language. It also understands other common forms (`2v2`, `4 jugadores`, `Español`), but this way you're sure. A map without a players tag doesn't show up when filtering by players, and a campaign, translation or audio pack without a language doesn't show up when filtering by language.

The `Gameplay`, `Cosmetic` and `Online tested` tags of earlier versions are no longer needed: whether a mod changes gameplay, the game knows from its files.

## Mods made before Imperivm Resurrected

Older mods ship whole `.pak` packages, and most of what's inside is untouched game files. Convert them so they contain only what they change or add:

```
node tools/shrink-mod.js <old mod> <game folder> <new folder> --id mi-mod --name "Mi mod" --version 1.0 --category "Gameplay & Balance"
```

It pulls out of each package only the files that differ from the game's, skips `.bak` copies, and puts each language's text and sounds in `Local\<language>\` (or in `currentlang\` if they're the same in all). With `--paths` instead of the output folder, it only lists the folders inside the packages and writes nothing.

- The output folder must be empty.
- If the old mod already has a `mod.json`, the tool keeps it, except `sources` (where the original mod was downloaded from), so version notices don't point to someone else's page.

Test it in the game before uploading.
