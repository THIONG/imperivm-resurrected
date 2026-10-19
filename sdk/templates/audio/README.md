# Audio pack template

Music, sounds and unit voices. Each player can use their own: audio never has to match between players.

## Steps

1. Copy this folder into `<game folder>\mods\` and rename it.
2. In `mod.json` change `id` (lowercase letters, numbers and hyphens: it identifies your mod), `name`, `authors` and `description`.
3. In `tags`, `Language` is the language of the voices. For music and sounds only, leave `"Language: Multilingual"`.
4. Put your files in the folder that matches what you change and delete the other folders.
5. Enable the mod in your profile and restart the game.

## Folder layout

```
my-audio-pack\
  mod.json
  README.md
  music\
    GBR_TRACK_1.ogg            replaces the game's track
  sounds\
    FIGHT\
      ...wav                   same path and name as the game's sound
  local\
    french\
      voices\
        <unit>\1.wav          up to 5.wav, only with that language
```

## Where to find the names

Extract the game's sounds to see the exact names and folders: `node tools\extract-data.js "<game folder>" <output folder> Sounds.pak`. The game's music is in `<game folder>\Music`.

## Notes

- Audio goes in GameBanana's **Sounds** section, not in Mods. See [Publishing](../../publishing.md).
- For new sounds that a unit class uses, see [Custom graphics](../../graphics.md) (`<sounds attack="…" die="…"/>`).
- Cutscenes and music are the same in every language: only voices, texts, help and some menu images change with the language ([Languages](../../languages.md)).

## Check and publish

```
node tools\validate-mod.js <your mod folder>
```

The validator reports problems with file and line without opening the game ([Validating a mod](../../validating.md)). Zip the mod folder with its `mod.json` inside and publish it as described in [Publishing on GameBanana](../../publishing.md).
