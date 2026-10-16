# Mod keys

Players change the game's keys in **Options → Controls**. Your mod's commands show up there on their own, as long as they have a name: you don't register anything.

Example to copy: [`examples/custom-orders`](examples/custom-orders).

## How your commands show up

A command is a `<cmd>` in a file under `data\commands`. Its default key is the `key` attribute (`key="j"`; with `ctrl="1"` it needs Ctrl). The player can change it, and their change is saved in `controls.ini`, not in your mod.

A command needs `rollover`: that's its name in the list. Without `rollover` it doesn't appear in Controls.

- If the command is in a game file that your mod patches (for example `RBarracks.xml`), it appears in the tab for that building or unit, with the others. Exception: `Stonehenge.xml` isn't in the table of known files, so a command added there appears under "Buildings" with the file name.
- If your mod ships a new commands file (for example `data\commands\MyBarracks.xml`), it appears in the **Content** tab, in a section named after your mod. With several commands files, each gets its own section.

The **Content** tab only appears if some active mod has new commands.

## Naming the section

By default the section is named after your mod (plus the file name if you ship several). To give per-language names, use `commands` in `mod.json`, keyed by the file name without extension:

```json
"commands": {
  "MyBarracks": {
    "name": {"spanish": "Cuartel romano extra", "english": "Extra Roman barracks", "italian": "Caserma romana extra"}
  }
}
```

It shows the name in the game's language; if there isn't one, English; if not, the first. A plain string also works (`"name": "Cuartel extra"`). On screen: `Mod name · Extra Roman barracks`.

Each command's name is its `rollover`, and it's translated like any other text: see [Languages and texts](languages.md).

## Which keys you can use

A letter (A–Z) or a symbol (semicolon, equals, comma, hyphen, period, slash, backslash, brackets, single quote and the backtick), optionally with Ctrl. Digits don't work: commands only accept letters and symbols, so `key="1"` never becomes a key in Controls. Shift and Alt don't work either, only Ctrl.

Players can also use the **By position on the bar** mode (Q is the first button, W the second…). Your commands go in there like all the others.

## What you can't do

- A key that does anything other than a game command (changing a mod option, firing a loose script) doesn't exist. Every keyed action has to be a command.
- A commands file outside `data\commands` doesn't load.
