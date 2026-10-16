# Publicar un mod en GameBanana

Los mods de Imperivm Resurrected se publican en https://gamebanana.com/games/26069. La pestaña **Explorar contenido** del gestor lee esa página: busca, filtra, enseña la ficha e instala con un clic. Para que tu mod salga ahí en su tipo y con sus filtros, sigue esto.

## Antes de subirlo

1. Pon la categoría en tu `mod.json` (la de la tabla de más abajo). Si es un mapa, una campaña, una traducción o un paquete de audio, pon también sus etiquetas:

   ```json
   {
     "id": "duelo-en-el-nilo",
     "name": "Duelo en el Nilo",
     "version": "1.0",
     "authors": ["Tu nombre"],
     "category": "Maps",
     "tags": ["2 players"]
   }
   ```

2. Pásalo por el [validador](validar.md). Te avisa si la categoría no es de las que entiende el juego o tiene un nombre antiguo, si a un mapa le faltan los jugadores y si a una campaña, traducción o audio le falta el idioma.

3. Haz el `.zip` de la carpeta del mod, con su `mod.json` dentro. No metas ejecutables ni scripts del sistema (`.exe .dll .asi .bat .cmd .com .scr .ps1 .vbs .js .msi .sys .cpl .lnk .reg .jar .hta .wsf .pif`): un mod con alguno no se carga.

## En GameBanana

1. Entra en **Add → Mod** desde la página del juego (`https://gamebanana.com/mods/add?_idGameRow=26069`). Siempre en **Mod**: lo que se suba como Sound, Tool o Script no sale en el juego.
2. **Category**: la misma de tu `mod.json`.
3. **Tags**: las mismas etiquetas, escritas igual.
4. **Files**: tu `.zip`, y en el campo **Version** la misma versión que en `mod.json`. Cada versión nueva, un archivo nuevo: no sustituyas el anterior, porque quien juegue online con la versión vieja la sigue necesitando.
5. **Requirements**: si tu mod necesita otro, enlaza su página (`https://gamebanana.com/mods/...`). El juego lo instala a la vez.
6. Al menos una captura. La primera es la imagen que se ve en el juego.
7. Para las notas de cambios, publica una **Update** con la versión como título (por ejemplo `1.1`). El juego la enseña junto a esa versión.

El aviso de versión nueva solo le llega a quien instaló el mod desde Explorar contenido (eso escribe `sources.gamebanana` en su `mod.json`), o a quien tenga un mod con el mismo `id` que la entrada. Quien descargue el zip a mano y lo descomprima no recibe aviso.

## Categorías

Una por mod: la de lo que más hace. Son nueve, en los tres tipos que distingue el juego. El tipo no lo decide la categoría sino los archivos del mod (ver [¿Qué cuenta como jugabilidad?](README.md#qué-cuenta-como-jugabilidad)); la categoría sirve para encontrarlo en Explorar contenido.

| Categoría | En el juego | Qué va aquí | Dónde van sus archivos | Ejemplo |
|---|---|---|---|---|
| **Cambian la jugabilidad** | | *En el online todos tienen que llevarlos* | | |
| **Gameplay & Balance** | Reglas y balance | Reglas, estadísticas, costes, IA; también los mods grandes que cambian el juego entero | `data\`, parches | [parches-ejemplo](ejemplos/parches-ejemplo), [scripts-ejemplo](ejemplos/scripts-ejemplo), [ordenes-propias](ejemplos/ordenes-propias) |
| **Units & Heroes** | Unidades y héroes | Unidades y héroes nuevos o cambiados | `data\classes\`, `specials\` | [habilidades-ejemplo](ejemplos/habilidades-ejemplo), [especialidad-ejemplo](ejemplos/especialidad-ejemplo) |
| **Civilizations** | Civilizaciones | Civilizaciones nuevas o rehechas | `civs\`, `data\` | [civilizacion-ejemplo](ejemplos/civilizacion-ejemplo) |
| **Contenido para jugar** | | *Solo lo necesita quien lo juega* | | |
| **Maps** | Mapas | Mapas para escaramuza y multijugador | `scenarios\MiMapa.bfhp` | [mapa-aleatorio-ejemplo](ejemplos/mapa-aleatorio-ejemplo) |
| **Campaigns** | Batallas y campañas | Batallas de un jugador con historia y objetivos, sueltas o encadenadas | `adventures\` | |
| **Conquest** | Conquista | Mapas y partidas de Conquista | `conquests\` | |
| **Estéticos** | | *Cada jugador lleva los suyos* | | |
| **Graphics** | Gráficos e interfaz | Texturas, aspecto de unidades, terreno, menús, iconos, fuentes | las rutas de los paquetes del juego, `menuini\` | |
| **Audio & Voices** | Audio y voces | Música, sonidos y voces | `sounds\`, `currentlang\voices\` | |
| **Translations** | Textos y traducciones | El juego o un mod en otro idioma, textos cambiados | `loc\` | [idioma-frances](ejemplos/idioma-frances), [textos-propios](ejemplos/textos-propios) |

Para las categorías sin ejemplo (mapas, campañas, gráficos, sonido), basta con copiar el archivo a su carpeta, con el mismo nombre si sustituye uno del juego.

**Nombres antiguos.** En GameBanana sigue existiendo la categoría **Scenarios**: no la uses (el juego la enseña como Mapas). Los `mod.json` con `Scenarios`, `Interface` o `Total Conversions` siguen funcionando como Maps, Graphics y Gameplay & Balance, pero el validador te pide el nombre nuevo.

## Etiquetas

Solo hacen falta en dos casos: son los filtros de jugadores e idioma de Explorar contenido.

| Categoría | Etiqueta | Filtro en el juego |
|---|---|---|
| Maps | `4 players`, o un intervalo como `2-4 players` (de 2 a 8) | Jugadores |
| Campaigns, Audio & Voices, Translations | `Spanish`, `English`, `Italian`, `German`, `French`, `Polish` o `Multilingual` (puedes poner varias) | Idioma |

Escríbelas en inglés tal cual, en `tags` de tu `mod.json` y en el campo **Tags** de GameBanana: el juego las enseña en el idioma de cada jugador. También entiende otras formas habituales (`2v2`, `4 jugadores`, `Español`), pero así te aseguras. Un mapa sin etiqueta de jugadores no sale al filtrar por jugadores, y una campaña, traducción o audio sin idioma no sale al filtrar por idioma.

Las etiquetas `Gameplay`, `Cosmetic` y `Online tested` de versiones anteriores ya no hacen falta: si un mod cambia la jugabilidad, el juego lo sabe por sus archivos.

## Mods hechos antes de Imperivm Resurrected

Los mods de antes traen paquetes `.pak` enteros, y casi todo lo que llevan son archivos del juego sin tocar. Conviértelos para que lleven solo lo que cambian o añaden:

```
node herramientas/reducir-mod.js <mod antiguo> <carpeta del juego> <carpeta nueva> --id mi-mod --nombre "Mi mod" --version 1.0 --categoria "Gameplay & Balance"
```

Saca de cada paquete solo los archivos distintos a los del juego, deja fuera las copias `.bak` y pone los textos y sonidos de cada idioma en `Local\<idioma>\` (o en `currentlang\` si son iguales en todos). Con `--rutas` en vez de la carpeta de salida solo enseña qué carpetas hay dentro de los paquetes, sin escribir nada.

- La carpeta de salida tiene que estar vacía.
- Si el mod antiguo ya trae `mod.json`, la herramienta lo conserva, menos `sources` (de dónde se baja el mod original), para que los avisos de versión no apunten a la página de otra persona.

Pruébalo en el juego antes de subirlo.
