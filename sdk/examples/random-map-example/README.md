# Example: random map type

A new random map type, **Anillo**: a ring of land with a sea in the middle and mountains in between.

This folder only ships the drawing, `drawings\ring.png` (black sea, white land, magenta mountains; the tool spreads the site points). Copied as is to `mods\`, it adds no map: you first have to generate the files the game reads. From the repo root:

```
node tools\create-random-map.js examples\random-map-example Anillo examples\random-map-example\drawings\ring.png --players 8
```

That creates `data\randommap\Anillo.rm.xml` and `randommap\Anillo\010.bmp … 120.bmp` inside the mod. Then copy the folder to `<game>\mods\`, enable it and restart. In a skirmish, "Map type" offers "Random: Anillo".

Guide: [Random maps](../../random-maps.md).
