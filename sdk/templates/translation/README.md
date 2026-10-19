# Translation template

The game, or another mod, in another language; or changed texts in a language the game already has.

## Steps

1. Copy this folder into `<game folder>\mods\` and rename it.
2. In `mod.json` change `id` (lowercase letters, numbers and hyphens: it identifies your mod), `name`, `authors` and `description`.
3. In `mod.json`, `language` declares a new language: `code` is its English name in lowercase (`french`), `name` is how it appears in the menu and `base` is the language used for anything you do not translate. Delete `language` if you only change texts of an existing language. In `tags`, put the language: `"Language: French"`.
4. Translate the texts in `loc\french.loc.xml` (rename it to your language's code). Anything you leave out shows in the `base` language.
5. Optional: files for that language only, in `local\<language>\`: `help.xml`, `tips.xml`, `credits.txt`, `splash.ini`, menu images and `voices\`. See [Languages and texts](../../languages.md).
6. Enable the mod in your profile, pick the language in Options → Language and restart.

## Folder layout

```
my-translation\
  mod.json
  README.md
  loc\
    french.loc.xml
  local\
    french\
      help.xml                 optional
      tips.xml                 optional
```

## Check and publish

```
node tools\validate-mod.js <your mod folder>
```

The validator reports problems with file and line without opening the game ([Validating a mod](../../validating.md)). Zip the mod folder with its `mod.json` inside and publish it as described in [Publishing on GameBanana](../../publishing.md).
