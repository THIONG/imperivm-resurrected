# Publicar un mod en GameBanana

Los mods de Imperivm Resurrected se publican en https://gamebanana.com/games/26069. La pestaña **Explorar contenido** del gestor lee esa página: busca, filtra, enseña la ficha e instala con un clic. Para que tu mod salga ahí en su tipo y con sus filtros, sigue esto.

## Antes de subirlo

1. Pon la categoría y las etiquetas en tu `mod.json` (las de las tablas de más abajo):

   ```json
   {
     "id": "duelo-en-el-nilo",
     "name": "Duelo en el Nilo",
     "version": "1.0",
     "authors": ["Tu nombre"],
     "category": "Maps",
     "tags": ["Cosmetic", "2 players", "Spanish"]
   }
   ```

2. Pásalo por el [validador](validar.md). Si la categoría o una etiqueta no son de las que entiende el juego, te lo dice.

3. Haz el `.zip` de la carpeta del mod, con su `mod.json` dentro. El botón **Publicar** del gestor lo prepara en `mods\_publish\<id>-<versión>.zip` y abre la página de subida. Mete en el zip todo lo que haya en la carpeta del mod (también un `arte\` con PNG fuente, si lo dejaste), y se niega a hacerlo si hay extensiones prohibidas o el `mod.json` está roto. Las prohibidas son ejecutables y scripts del sistema (`.exe .dll .asi .bat .cmd .com .scr .ps1 .vbs .js .msi .sys .cpl .lnk .reg .jar .hta .wsf .pif`): un mod con alguno no se carga ni se puede publicar.

## En GameBanana

1. Entra en **Add → Mod** desde la página del juego. Siempre en **Mod**: lo que se suba como Sound, Tool o Script no sale en el juego. El botón Publicar del gestor abre `https://gamebanana.com/mods/add?_idGameRow=26069`.
2. **Category**: la misma de tu `mod.json`.
3. **Tags**: las mismas etiquetas.
4. **Files**: tu `.zip`, y en el campo **Version** la misma versión que en `mod.json`. Cada versión nueva, un archivo nuevo: no sustituyas el anterior, porque quien juegue online con la versión vieja la sigue necesitando.
5. **Requirements**: si tu mod necesita otro, enlaza su página (`https://gamebanana.com/mods/...`). El juego lo instala a la vez.
6. Al menos una captura. La primera es la imagen que se ve en el juego.
7. Para las notas de cambios, publica una **Update** con la versión como título (por ejemplo `1.1`). El juego la enseña junto a esa versión.

El aviso de versión nueva solo le llega a quien instaló el mod desde Explorar contenido (eso escribe `fuentes.gamebanana` en su `mod.json`), o a quien tenga un mod con el mismo `id` que la entrada. Quien descargue el zip a mano y lo descomprima no recibe aviso.

## Categorías

Una por mod: la de lo que más hace.

| Categoría | Qué va aquí | Dónde van sus archivos | Ejemplo |
|---|---|---|---|
| **Maps** | Mapas para escaramuza y multijugador | `scenarios\MiMapa.bfhp` | |
| **Scenarios** | Escenarios de un jugador con historia y objetivos | `scenarios\` (con sus scripts) | |
| **Campaigns** | Varios escenarios encadenados | `adventures\` | |
| **Conquest** | Mapas y partidas de Conquista | `conquests\` | |
| **Gameplay & Balance** | Reglas, estadísticas, costes, IA | `data\`, parches | [parches-ejemplo](ejemplos/parches-ejemplo), [scripts-ejemplo](ejemplos/scripts-ejemplo), [ordenes-propias](ejemplos/ordenes-propias) |
| **Units & Heroes** | Unidades y héroes nuevos o cambiados | `data\classes\`, `specials\` | [habilidades-ejemplo](ejemplos/habilidades-ejemplo), [especialidad-ejemplo](ejemplos/especialidad-ejemplo) |
| **Civilizations** | Civilizaciones nuevas o rehechas | `civs\`, `data\` | [civilizacion-ejemplo](ejemplos/civilizacion-ejemplo) |
| **Graphics** | Texturas, aspecto de unidades, terreno | las rutas de los paquetes del juego | |
| **Audio & Voices** | Música, sonidos y voces | `sounds\`, `currentlang\voices\` | |
| **Interface** | Menús, iconos, fuentes, textos de la interfaz | `menuini\`, `loc\` | [textos-propios](ejemplos/textos-propios) |
| **Translations** | El juego o un mod en otro idioma | `loc\` | [idioma-frances](ejemplos/idioma-frances) |
| **Total Conversions** | Mods grandes que cambian el juego entero | de todo | |

Para las categorías de contenido sin ejemplo (mapas, escenarios, gráficos, sonido), basta con copiar el archivo a su carpeta, con el mismo nombre si sustituye uno del juego.

## Etiquetas

Son opcionales, pero son los filtros del juego: sin ellas, tu mod no sale al filtrar.

| Filtro en el juego | Etiqueta |
|---|---|
| Cambia la jugabilidad | `Gameplay` |
| Solo estético | `Cosmetic` |
| Jugadores (mapas) | `2 players` … `8 players` |
| Idioma | `Spanish`, `English`, `Italian`, `German`, `French`, `Polish`, `Multilingual` |
| Verificado | `Online tested` (solo si lo has jugado online con otra persona) |

Pon `Gameplay` o `Cosmetic`, una de las dos. Un mod que cambia la partida lo tiene que llevar todo el mundo en el online; uno estético, no (ver [¿Qué cuenta como jugabilidad?](README.md#qué-cuenta-como-jugabilidad)).

Escríbelas tal cual, en inglés: el juego las enseña en el idioma de cada jugador. El filtro busca por subcadena y las categorías cuentan también como etiquetas. Un mod de «Gameplay & Balance» sale al filtrar «Cambia la jugabilidad» aunque lleve `Cosmetic`.

## Mods hechos antes de Imperivm Resurrected

Los mods de antes traen paquetes `.pak` enteros, y casi todo lo que llevan son archivos del juego sin tocar. Conviértelos para que lleven solo lo que cambian o añaden:

```
node herramientas/reducir-mod.js <mod antiguo> <carpeta del juego> <carpeta nueva> --id mi-mod --nombre "Mi mod" --version 1.0 --categoria "Gameplay & Balance" --etiquetas Gameplay
```

Saca de cada paquete solo los archivos distintos a los del juego, deja fuera las copias `.bak` y pone los textos y sonidos de cada idioma en `Local\<idioma>\` (o en `currentlang\` si son iguales en todos). Con `--rutas` en vez de la carpeta de salida solo enseña qué carpetas hay dentro de los paquetes, sin escribir nada.

- La carpeta de salida tiene que estar vacía.
- Si el mod antiguo ya trae `mod.json`, la herramienta lo conserva, menos `sources` (de dónde se baja el mod original), para que los avisos de versión no apunten a la página de otra persona.

Pruébalo en el juego antes de subirlo.
