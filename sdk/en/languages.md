# Languages and texts

A mod can change or add game texts without replacing the whole text file, and can add a new language (French, German, Polish…) that shows up in **Options → Language**. It can also ship files used by one language only: help, tips, menu images, voices.

Examples to copy: [`examples/custom-texts`](examples/custom-texts) and [`examples/french-language`](examples/french-language).

## How the game language works

The language is chosen in Options → Language and applies after a restart. It's stored in `Settings.ini`, `[Language] Default=`.

The game's texts are in `currentlang/game.loc.xml`, inside `local\<language>.pak`. Each text is a line like this:

```xml
<translationtableentry text="Single player@/Menu/mainmenu.ini:SinglePlayer:Text" justtext="Single player" result="Estrategia"/>
```

`text` is the key: the original English text, sometimes followed by `@` and a context (where it's used). `result` is what you see in that language. If a text has no translation, the game shows the key, which is the English. That's why English hardly needs a file.

Imperivm Resurrected's own interface (content manager, lobby, Controls, saved games, performance panel) comes in Spanish, English and Italian. With the game in Italian it shows in Italian; with any other language, in English. Your mod's texts (name, description, commands) are translated with the `loc\` files below.

Cutscenes and music are the same in every language. What changes is texts, unit voices, help, tips and some menu images.

To see the text file, add this to `imperivm-mod.ini` and open the game:

```ini
[Debug]
Dump=currentlang/game.loc.xml
```

It's copied to `mod-dump\currentlang\game.loc.xml` with your mods' texts already merged in. To see the original, use the Original profile.

## Change or add texts

Put the texts in `loc\<language>.loc.xml` inside your mod. If there are many, split them across several `*.loc.xml` files inside `loc\<language>\`:

```
mods\mi-mod\
  mod.json
  loc\spanish.loc.xml
  loc\italian.loc.xml
  loc\english\unidades.loc.xml
```

`<language>` is the language's English name in lowercase, as in `Settings.ini`: `spanish`, `english`, `italian`, `french`, `german`…

```xml
<translationtable>
  <!-- Changes the text everywhere it appears -->
  <translationtableentry text="Single player" result="Un jugador"/>

  <!-- One place only: the full key, copied from the game's game.loc.xml -->
  <translationtableentry text="Options@/Menu/endgamemenu.ini:Options:Text" result="Ajustes"/>

  <!-- The same, written with context -->
  <translationtableentry text="Options" context="/Menu/endgamemenu.ini:Options:Text" result="Ajustes"/>

  <!-- New text, e.g. the name of a unit in your mod (display_name in its .sc.xml) -->
  <translationtableentry text="Gladiator" result="Gladiador"/>
</translationtable>
```

- Without a context (`text="Single player"`), the change applies everywhere the game uses that text. With a context (`text="…@…"` or the `context` attribute), only in that place.
- Line break: `\n` inside the text, as in the game's files.
- In a text, write `&` as `&amp;`, `<` as `&lt;` and double quotes as `&quot;` (or use single quotes around the value).
- Save as UTF-8. If it's ANSI, the mod converts it, but UTF-8 is the safe choice.
- A malformed line is skipped and logged in `Logs\imperivm\imperivm-mod.log` with its line number. The rest of the file loads.
- `<game>\mod\loc\<language>.loc.xml` works for testing without creating a mod, and beats all mods.
- If two mods change the same text, the one first in the [load order](reference.md#1-mod-structure) should win, as with files.
- Texts are read at startup: restart after changing them.

The names of your mod's units, buildings and commands are written in English in their files (`display_name`, `rollover`…). That's the key. To translate them, add the translation to `loc\spanish.loc.xml`, `loc\italian.loc.xml`, etc.

## Add a new language

1. Declare the language in `mod.json`:

   ```json
   "language": {"code": "french", "name": "Français", "base": "spanish"}
   ```

   - `code`: the language's English name in lowercase. It's what gets stored in `Settings.ini`.
   - `name`: what shows in the selector.
   - `base` (optional, `english` by default): the language to take whatever your mod doesn't provide from. Read the warning below before leaving it as English.

2. Translate the texts in `loc\french.loc.xml` (or in several files inside `loc\french\`). Anything you don't translate shows in English.

3. Optional: language files in `Local\french\`, with the paths they have inside `currentlang\`. The ones the game asks for are:

   ```
   Local\french\help.xml              help (F1)
   Local\french\tips.xml              loading screen tips
   Local\french\credits.txt           credits
   Local\french\splash.ini            startup screens
   Local\french\menu_16_9.bmp         menu background (also menu_4_3.bmp, menubackground.bmp, logo.bmp)
   Local\french\voices\<unit>\1.wav unit voices (1.wav up to 5.wav depending on the unit)
   ```

   You can also pack it all into `Local\french.pak`.

4. Enable the mod in your profile, pick **Français** in Options → Language and restart.

**What the base language provides.** Anything your language doesn't provide is taken from the `base` language. English has no help (`help.xml`), tips (`tips.xml`), `splash.ini` or menu images. A new language with English as its base ends up without those files. Spanish and Italian do have them: with `"base": "spanish"` or `"italian"` you get them, or you ship your own in `Local\<language>\`.

Maps ship their own texts per language. If a map doesn't have yours, the base language's are used.

**Characters.** The game's fonts should cover Latin (with European accents), Greek and Cyrillic. Chinese, Japanese and Korean can't be displayed without new fonts.

In multiplayer the language doesn't matter: players with different languages can play together.

## Files for a single language

Any file in `currentlang\` can be changed for one language only with your mod's `Local\<language>\` folder. `Local\spanish\help.xml` changes the help in Spanish only. If you put the file in `currentlang\help.xml` (outside `Local\`), it applies to all languages.

## If something goes wrong

Look in `Logs\imperivm\imperivm-mod.log` for the lines tagged `language`. The warnings (`W`) point at the lines with errors. To also see which text files were read and which `Local\<language>\` files are in use, put `LogChannels=language:debug` under `[Debug]` in `imperivm-mod.ini`.
