# Example: new formation

Adds a hero formation, the **Wedge** (+3 damage and +1 armor; cavalry in front). You pick it like the others, on the hero's formation button.

- `patches\data\formations.xml`: the formation (size, bonuses and where each unit class goes).
- `patches\data\commands\Hero.xml`: its two buttons, `form-wedge-menu` (the one you see when it's the current formation) and `form-wedge` (the one that selects it, key `i`). They use the Front formation's image.
- `patches\data\subai\hero_formation_verify.vs`: a patch for the game script, which only allows picking a formation from its four menus; it adds the new one.

The bonuses apply only while troops stand still ("stand ground"). The game provides the button text; the XML `Description` isn't shown.

To test it, copy this folder to `<game>\mods\`, enable it in the content manager, restart, start a skirmish, put troops under a hero's command and open his formation menu. Guide: [Formations](../../formations.md).
