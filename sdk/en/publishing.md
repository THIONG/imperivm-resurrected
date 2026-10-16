# Publishing a mod on GameBanana

Imperivm Resurrected mods are published at https://gamebanana.com/games/26069. The **Browse content** tab in the content manager reads that page: search, filter, view a mod's page and install with one click. To make your mod show up under the right type and filters, follow these steps.

## Before you upload

1. Set the category and tags in your `mod.json` (use the ones from the tables below):

   ```json
   {
     "id": "duelo-en-el-nilo",
     "name": "Duelo en el Nilo",
     "version": "1.0",
     "authors": ["Tu nombre"],
     "category": "Maps",
     "tags": ["Cosmetic", "2 players", "Spanish"]
   }
   ```

2. Run it through the [validator](validating.md). It tells you if the category or a tag isn't one the game recognizes.

3. Zip the mod folder, with its `mod.json` inside. The manager's **Publish** button builds `mods\_publish\<id>-<version>.zip` and opens the upload page. It zips everything in the mod folder (including an `art\` folder with source PNGs, if you left one), and refuses if there are forbidden extensions or a broken `mod.json`. The forbidden ones are executables and system scripts (`.exe .dll .asi .bat .cmd .com .scr .ps1 .vbs .js .msi .sys .cpl .lnk .reg .jar .hta .wsf .pif`): a mod containing any of them won't load and can't be published.

## On GameBanana

1. Go to **Add → Mod** from the game's page. Always **Mod**: anything uploaded as Sound, Tool or Script won't show up in the game. The manager's Publish button opens `https://gamebanana.com/mods/add?_idGameRow=26069`.
2. **Category**: the same one as in your `mod.json`.
3. **Tags**: the same tags.
4. **Files**: your `.zip`, and in the **Version** field the same version as in `mod.json`. Every new version is a new file: don't replace the old one, because anyone playing online with the old version still needs it.
5. **Requirements**: if your mod needs another one, link its page (`https://gamebanana.com/mods/...`). The game installs it along with yours.
6. At least one screenshot. The first one is the image shown in the game.
7. For the changelog, post an **Update** with the version as its title (for example `1.1`). The game shows it next to that version.

Only people who installed the mod from Browse content (that writes `fuentes.gamebanana` into their `mod.json`) get the new-version notice, or people with a mod that has the same `id` as the entry. If someone downloads the zip by hand and unzips it, they get no notice.

## Categories

One per mod: pick the one it does the most for.

| Category | What goes here | Where its files go | Example |
|---|---|---|---|
| **Maps** | Maps for skirmish and multiplayer | `scenarios\MiMapa.bfhp` | |
| **Scenarios** | Single-player scenarios with a story and objectives | `scenarios\` (with their scripts) | |
| **Campaigns** | Several chained scenarios | `adventures\` | |
| **Conquest** | Conquest maps and games | `conquests\` | |
| **Gameplay & Balance** | Rules, stats, costs, AI | `data\`, patches | [patches-example](examples/patches-example), [scripts-example](examples/scripts-example), [custom-orders](examples/custom-orders) |
| **Units & Heroes** | New or changed units and heroes | `data\classes\`, `specials\` | [ability-example](examples/ability-example), [specialty-example](examples/specialty-example) |
| **Civilizations** | New or reworked civilizations | `civs\`, `data\` | [civilization-example](examples/civilization-example) |
| **Graphics** | Textures, unit looks, terrain | the game's package paths | |
| **Audio & Voices** | Music, sounds and voices | `sounds\`, `currentlang\voices\` | |
| **Interface** | Menus, icons, fonts, interface text | `menuini\`, `loc\` | [custom-texts](examples/custom-texts) |
| **Translations** | The game or a mod in another language | `loc\` | [french-language](examples/french-language) |
| **Total Conversions** | Big mods that change the whole game | everything | |

For content categories without an example (maps, scenarios, graphics, sound), just copy the file into its folder, with the same name if it replaces a game file.

## Tags

They're optional, but they drive the game's filters: without them, your mod won't show up when filtering.

| Filter in the game | Tag |
|---|---|
| Changes gameplay | `Gameplay` |
| Cosmetic only | `Cosmetic` |
| Players (maps) | `2 players` … `8 players` |
| Language | `Spanish`, `English`, `Italian`, `German`, `French`, `Polish`, `Multilingual` |
| Verified | `Online tested` (only if you've played it online with someone else) |

Use `Gameplay` or `Cosmetic`, one of the two. A mod that changes the game must be installed by everyone online; a cosmetic one doesn't (see [What counts as gameplay?](README.md#what-counts-as-gameplay)).

Write them exactly as shown, in English: the game displays them in each player's language. The filter matches substrings, and categories count as tags too. A "Gameplay & Balance" mod shows up when filtering by "Changes gameplay", even if it carries `Cosmetic`.

## Mods made before Imperivm Resurrected

Older mods ship whole `.pak` packages, and most of what's inside is untouched game files. Convert them so they contain only what they change or add:

```
node tools/shrink-mod.js <old mod> <game folder> <new folder> --id mi-mod --name "Mi mod" --version 1.0 --category "Gameplay & Balance" --tags Gameplay
```

It pulls out of each package only the files that differ from the game's, skips `.bak` copies, and puts each language's text and sounds in `Local\<language>\` (or in `currentlang\` if they're the same in all). With `--paths` instead of the output folder, it only lists the folders inside the packages and writes nothing.

- The output folder must be empty.
- If the old mod already has a `mod.json`, the tool keeps it, except `sources` (where the original mod was downloaded from), so version notices don't point to someone else's page.

Test it in the game before uploading.
