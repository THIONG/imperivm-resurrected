# Mod template

A bare-bones folder to start from. Copy it to `<game>\mods\`, rename it and fill in `mod.json`.

1. Change `id` (lowercase letters, numbers and hyphens: it's what identifies you), `name`, `authors` and `description`.
2. Pick `category` and `tags` from the list in [Publishing](../publishing.md).
3. Add the folders you need, using the same paths the game does:

```
data\classes\       new classes, or copies of the game's
data\commands\      orders (buttons)
patches\data\...    patches over game files
loc\                texts
events\             event scripts
civs\               new civilizations
options\            game options
```

What goes in each folder, and the details of each thing, are in the guides ([index](../README.md)); the complete field list is in the [reference](../reference.md).

Before opening the game: `node tools\validate-mod.js <your folder> --data <DATA folder>`. You get the DATA folder with `extract-data.js` (see [Validating a mod](../validating.md)).
