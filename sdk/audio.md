# Music, sounds and voices

An audio pack replaces or adds music, sounds and unit voices. Audio never has to match between players, so it does not count as gameplay. Start from the [audio template](templates/audio).

| What | Folder in the mod | Format |
|---|---|---|
| Music | `music\` | `.ogg` |
| Sounds | `sounds\<group>\` | `.wav` |
| Unit voices | `local\<language>\voices\<unit>\` | `.wav`, `1.wav` to `5.wav` |

A file with the same path and name as one of the game's replaces it. A new name adds a file that your own classes and maps can use.

## Music

The game's music is in `<game folder>\Music`: `GBR_TRACK_1.ogg` to `GBR_TRACK_7.ogg` and `_menu.ogg` for the menu. A mod with `music\GBR_TRACK_1.ogg` replaces that track.

## Sounds

The game's sounds are in `Packs\Sounds.pak`, inside `sounds\`, grouped in folders: `AMBIENT`, `ANIMALS`, `DEATH`, `EFFECTS`, `FIGHT`, `GATES`, `NOTIFICATIONS`, `SELECTION`, `UI` and `WALK`. To see the exact names, extract them:

```
node tools\extract-data.js "<game folder>" <output folder> Sounds.pak
```

Your files go in the same folders: `sounds\FIGHT\<name>.wav`. A class names its sounds with `<sounds attack="…" die="…"/>`; see [Custom graphics](graphics.md#sounds) for how to point it to new ones.

## Voices

Voices belong to a language, so they go under `local\<language>\voices\<unit>\`, with the language's English name in lowercase (`english`, `spanish`, `french`…): `local\french\voices\<unit>\1.wav`, up to `5.wav` depending on the unit. Only the language the player has selected is used. See [Languages and texts](languages.md#files-for-a-single-language).

## In `mod.json`

```json
{
  "id": "my-audio-pack",
  "name": "My audio pack",
  "version": "1.0",
  "authors": ["Your name"],
  "category": "Audio & Voices",
  "tags": ["Language: Multilingual"]
}
```

`Language` is the language of the voices. For music and sounds only, use `Multilingual`.

Audio is published in GameBanana's **Sounds** section, not in Mods. See [Publishing](publishing.md).
