# Publicar un mod en GameBanana

Los mods de Imperivm Resurrected se publican en https://gamebanana.com/games/26069. La pestaña **Explorar contenido** del gestor lee esa página: busca, filtra, enseña la ficha e instala con un clic. Para que tu mod salga ahí en su tipo y con sus filtros, sigue esto.

## Antes de subirlo

1. Pon la categoría en tu `mod.json` (la de la tabla de más abajo) y, si hace falta, sus [etiquetas](#etiquetas): las categorías de más si tu mod hace varias cosas, los jugadores si es un escenario y el idioma si es una aventura, una traducción o un paquete de audio:

   ```json
   {
     "id": "duelo-en-el-nilo",
     "name": "Duelo en el Nilo",
     "version": "1.0",
     "authors": ["Tu nombre"],
     "category": "Scenarios",
     "tags": ["Players: 2"]
   }
   ```

2. Pásalo por el [validador](validar.md). Te avisa si la categoría no es de las que entiende el juego o tiene un nombre antiguo, si una etiqueta está mal escrita, si a un escenario le faltan los jugadores y si a una aventura, traducción o audio le falta el idioma.

3. Haz el `.zip` de la carpeta del mod, con su `mod.json` dentro. No metas ejecutables ni scripts del sistema (`.exe .dll .asi .bat .cmd .com .scr .ps1 .vbs .js .msi .sys .cpl .lnk .reg .jar .hta .wsf .pif`): un mod con alguno no se carga.

## En GameBanana

1. Entra en **Add → Mod** desde la página del juego (`https://gamebanana.com/mods/add?_idGameRow=26069`). El audio es la excepción: va en **Add → Sound** (`https://gamebanana.com/sounds/add?_idGameRow=26069`). Lo que se suba como Tool o Script no sale en el juego.
2. **Category**: la misma de tu `mod.json`.
3. **Tags**: las mismas etiquetas. Cada una tiene un título y un valor: en `Type: Civilizations`, el título es `Type` y el valor `Civilizations`. No hay que crearlas antes: se escriben en cada mod.
4. **Files**: tu `.zip`, y en el campo **Version** la misma versión que en `mod.json`. Cada versión nueva, un archivo nuevo: no sustituyas el anterior, porque quien juegue online con la versión vieja la sigue necesitando.
5. **Requirements**: si tu mod necesita otro, enlaza su página (`https://gamebanana.com/mods/...`, o `https://gamebanana.com/sounds/...` si es de audio). El juego lo instala a la vez.
6. Al menos una captura. La primera es la imagen que se ve en el juego.
7. Para las notas de cambios, publica una **Update** con la versión como título (por ejemplo `1.1`). El juego la enseña junto a esa versión.

El aviso de versión nueva solo le llega a quien instaló el mod desde Explorar contenido (eso escribe `sources.gamebanana` en su `mod.json`, o `sources.gamebananaSound` si viene de Sounds), o a quien tenga un mod con el mismo `id` que la entrada. Quien descargue el zip a mano y lo descomprima no recibe aviso.

## Categorías

**En resumen:** en GameBanana eliges **una categoría**, la que mejor describe el mod en general. Todo lo demás que trae va en **etiquetas** `Type`, `Players` y `Language`: pon todas las que le correspondan, y solo esas.

GameBanana deja una sola por mod: elige la de lo que más hace. Si tu mod hace más cosas, por ejemplo uno grande que trae civilizaciones y mapas, añade las demás con etiquetas `Type` (ver [Etiquetas](#etiquetas)) y saldrá en todas. Son nueve, en los tres tipos que distingue el juego. El tipo no lo decide la categoría sino los archivos del mod (ver [¿Qué cuenta como jugabilidad?](README.md#qué-cuenta-como-jugabilidad)); la categoría sirve para encontrarlo en Explorar contenido.

| Categoría | En el juego | Qué va aquí | Dónde van sus archivos | Ejemplo |
|---|---|---|---|---|
| **Cambian la jugabilidad** | | *En el online todos tienen que llevarlos* | | |
| **Gameplay & Balance** | Jugabilidad y balance | Reglas, estadísticas, costes, IA; también los mods grandes que cambian el juego entero | `data\`, parches | [parches-ejemplo](ejemplos/parches-ejemplo), [scripts-ejemplo](ejemplos/scripts-ejemplo), [ordenes-propias](ejemplos/ordenes-propias) |
| **Units & Heroes** | Unidades y héroes | Unidades y héroes nuevos o cambiados | `data\classes\`, `specials\` | [habilidades-ejemplo](ejemplos/habilidades-ejemplo), [especialidad-ejemplo](ejemplos/especialidad-ejemplo) |
| **Civilizations** | Civilizaciones | Civilizaciones nuevas o rehechas | `civs\`, `data\` | [civilizacion-ejemplo](ejemplos/civilizacion-ejemplo) |
| **Contenido para jugar** | | *Solo lo necesita quien lo juega* | | |
| **Scenarios** | Escenarios | Mapas para escaramuza y multijugador | `scenarios\MiMapa.bfhp` | [mapa-aleatorio-ejemplo](ejemplos/mapa-aleatorio-ejemplo) |
| **Adventures** | Aventuras | Grandes batallas, desafíos y campañas de un jugador, con historia y objetivos | `adventures\` | |
| **Conquests** | Conquistas | Mapas y partidas de Conquista | `conquests\` | |
| **Estéticos** | | *Cada jugador lleva los suyos* | | |
| **Graphics** | Gráficos | Texturas, aspecto de unidades, terreno, menús, iconos, fuentes | las rutas de los paquetes del juego, `menuini\` | |
| **Audio & Voices** | Audio y voces | Música, sonidos y voces. Se publica en la sección **Sounds** de GameBanana, no en Mods | `sounds\`, `currentlang\voices\` | |
| **Translations** | Traducciones | El juego o un mod en otro idioma, textos cambiados | `loc\` | [idioma-frances](ejemplos/idioma-frances), [textos-propios](ejemplos/textos-propios) |

Para las categorías sin ejemplo (aventuras, conquistas, gráficos, sonido), basta con copiar el archivo a su carpeta, con el mismo nombre si sustituye uno del juego.

**Mapas sueltos.** Si el `.zip` trae solo el `.bfhp`, sin carpetas, el juego lo coloca él solo al instalarlo según el tipo que dice el propio mapa (lo que eligió su autor en el editor): una aventura va a `adventures\` (con su `.bmp` del mismo nombre), una conquista a `conquests\` y un mapa de escaramuza a `scenarios\`, o a `adventures\` si la categoría es Adventures (misiones de un jugador hechas como mapa normal). Aun así, lo recomendable es subirlo ya dentro de su carpeta; el [validador](validar.md) avisa si un mapa está en la que no le toca, por ejemplo una aventura en `scenarios\`, que la lista de Escenarios no enseña.

**Vista previa de un escenario.** Al elegir un escenario en la sala, el juego dibuja una vista previa con el terreno del propio mapa, los jugadores que tiene y su civilización, y de dónde viene: no hace falta que subas ninguna imagen. Los colores salen de las losetas del minimapa del juego, así que un terreno nuevo se ve bien si trae su loseta en `minimapzoom8	errain`.

**Nombres antiguos.** Escenarios, Aventuras y Conquistas se llaman como las carpetas del juego (`scenarios\`, `adventures\`, `conquests\`). Antes eran **Maps**, **Campaigns** y **Conquest**: un `mod.json` con esos nombres, o con `Interface` o `Total Conversions`, sigue funcionando, pero el validador te pide el nombre nuevo.

## Etiquetas

Cada etiqueta tiene un título y un valor. El juego usa tres:

| Etiqueta | Para qué | Ejemplo |
|---|---|---|
| `Type: <categoría>` | Más categorías, además de la principal. Una etiqueta por categoría | `Type: Civilizations`, `Type: Scenarios` |
| `Players: <jugadores>` | Jugadores de un escenario, de 2 a 8. Puedes poner varias | `Players: 4`, `Players: 2-4` |
| `Language: <idioma>` | Idioma de una aventura, una traducción o un paquete de audio: `Spanish`, `English`, `Italian`, `German`, `French`, `Polish` o `Multilingual`. Puedes poner varias | `Language: Spanish` |

Escríbelas en inglés tal cual, en `tags` de tu `mod.json` (por ejemplo `"tags": ["Type: Scenarios", "Players: 2-4"]`) y en el campo **Tags** de GameBanana: el juego las enseña en el idioma de cada jugador. Pon solo estas tres: el juego no usa ninguna otra, y el validador te avisa si hay alguna más.

Un escenario sin `Players` no sale al filtrar por jugadores, y una aventura, traducción o audio sin `Language` no sale al filtrar por idioma. Lo mismo vale para las categorías de `Type`: un mod con `Type: Scenarios` necesita `Players` para salir al filtrar por jugadores.

Las formas sin título de versiones anteriores (`4 players`, `Spanish`) siguen funcionando, pero el validador te pide la nueva. Las etiquetas `Gameplay`, `Cosmetic` y `Online tested` ya no hacen falta: si un mod cambia la jugabilidad, el juego lo sabe por sus archivos.

## Mods hechos antes de Imperivm Resurrected

Los mods de antes se instalaban copiándolos encima del juego: traen paquetes `.pak` enteros que sustituyen a los del juego, y casi todo lo que llevan son archivos del juego sin tocar. Conviértelos para que lleven solo lo que cambian o añaden:

```
node herramientas/reducir-mod.js <mod antiguo> <carpeta del juego> <carpeta nueva> --id mi-mod --nombre "Mi mod" --version 1.0 --categoria "Gameplay & Balance"
```

La herramienta calcula qué ve el juego con el mod copiado encima y qué ve sin él, y guarda solo la diferencia:

- De los paquetes, los archivos distintos a los del juego, respetando su orden (en una carpeta gana el paquete con el nombre mayor, como `zzMod.pak` sobre `Data.pak`).
- Los textos y sonidos de cada idioma en `Local\<idioma>\` (o en `currentlang\` si son iguales en todos), y lo demás de un paquete de idioma en `Local\<idioma>.pak`.
- Lo que el mod antiguo quitaba del juego al sustituir sus paquetes, en `hidden.txt`, para que el juego tampoco lo vea.
- `data\const.ini` como parche y sin la sección `[Resolutions]`: la resolución la elige el jugador en las opciones.
- Si el mod trae su propio `rle.mmp`, solo lo que cambia o añade al del juego, en `rle.mmp.tail`; el del juego no se toca.
- Deja fuera las copias `.bak`, `Config.ini` y `Settings.ini` (configuración de la instalación) y los ejecutables, con los que el mod no se cargaría.

Opciones:

- `--fuera <patrones>` deja fuera más archivos del mod antiguo, como guías de instalación (rutas con `*` y `?`, separadas por comas).
- `--ocultar <patrones>` oculta archivos sueltos del juego que el léeme del mod antiguo mandaba borrar (por ejemplo `currentlang/*,units/*`).
- `--rutas`, en vez de la carpeta de salida, solo enseña qué carpetas hay dentro de los paquetes, sin escribir nada.

La carpeta de salida tiene que estar vacía. Si el mod antiguo ya trae `mod.json`, la herramienta lo conserva, menos `sources` (de dónde se baja el mod original), para que los avisos de versión no apunten a la página de otra persona. Al final dice qué ha dejado fuera y qué oculta.

Pruébalo en el juego antes de subirlo.
