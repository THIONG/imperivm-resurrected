# Modos de victoria propios

Las condiciones de victoria del juego (Eliminación, Límite de puntuación…) son scripts `.vs`. Un mod puede traer los suyos y salen solos en el desplegable "Modo" de la sala.

Ejemplo: [`ejemplos/victoria-ejemplo`](ejemplos/victoria-ejemplo) (modo "Dominio": gana quien controla N asentamientos).

## El script

Va en `data\GameScripts\` de tu mod:

```
data\GameScripts\6 Dominio.vs
```

- **El nombre del archivo es el id del modo**, número incluido. La sala lista los modos por nombre de archivo completo. Dos mods chocan solo si usan exactamente el mismo nombre (`6 Dominio.vs` en los dos). Los del juego son del 1 al 5.
- En la sala se muestra sin el número inicial y pasa por la traducción: si el texto "Dominio" tiene entrada en tu `loc`, saldrá traducido ([Idiomas](idiomas.md)).
- La firma es `//void, int player, str param`. El juego carga el script siempre con ella, así que la primera línea es opcional, pero conviene ponerla. `player` es el jugador (1 a 8). `param` es el valor del límite que se elige en la sala (ver más abajo).
- El juego lanza 8 copias del script, una por jugador, cada una en su hilo, aunque haya menos jugadores. Escríbelo desde el punto de vista de un solo jugador y que mire a los demás. Tiene que aguantar los huecos vacíos: el del ejemplo los da por eliminados porque no tienen ni asentamientos ni tropas.

## Funciones que se usan

| Función | Para qué |
|---|---|
| `EndGame(player, false)` / `EndGame(player, true)` | El jugador gana / pierde. Después, `return` |
| `SetPlayerStatus(player, linea, texto, important)` | Texto de estado arriba: línea 0 el nombre del modo, línea 1 el aviso. También existe `SetPlayerStatus(player, linea, bool)`; los scripts del juego usan las dos |
| `ClassPlayerObjs("BaseTownhall", player)` | Objetos de una clase de un jugador; `.count` los cuenta, `.IsEmpty()` dice si no hay ninguno |
| `MilUnits(player)` | Cuántas tropas tiene |
| `DiplAreAllied(a, b)` | Si dos jugadores son aliados |
| `Str2Int(param)` | Pasa `param` a número |
| `EnvReadInt` / `EnvWriteInt` | Variables compartidas entre los scripts. Con jugador (`EnvWriteInt(player, "clave", n)`) o sin él (`EnvWriteInt("modo", 1)`) |
| `GetConst("…")` | Constantes de `CONST.INI` |
| `Translate("…")` / `Translatef("…", a, b)` | Textos con idioma |

Para ver cómo lo hace el juego, saca sus scripts con `extraer-datos.js` (`node herramientas\extraer-datos.js "<carpeta del juego>" <salida>`) y mira `GAMESCRIPTS\1 ELIMINATION.VS` y `2 SCORE LIMIT.VS`. Con el volcado de [Parches](parches.md) también se puede, con la ruta completa: `Dump=data/GameScripts/1 ELIMINATION.VS`.

## El desplegable "Límite"

Lo rellena el juego con entradas de `CONST.INI`, sección `[GamePlay]`. La clave es el nombre del archivo con `_` en vez de espacios, más un número desde 0. El valor son dos partes separadas por una coma: el texto que se ve y lo que recibirá el script como `param`.

El ejemplo lo hace con un parche INI en `patches\data\const.ini`:

```ini
[GamePlay]
6_Dominio0 = 2 asentamientos, 2
6_Dominio1 = 3 asentamientos, 3
6_Dominio2 = 4 asentamientos, 4
6_Dominio3 = 5 asentamientos, 5
```

Si no pones entradas, el desplegable sale vacío y no hay límite que elegir. Qué recibe entonces `param` no se ha visto en partida: el ejemplo cae a 3 si `Str2Int(param)` da menos de 2.

## Ajustes fijos y civilizaciones del modo

Un modo puede fijar ajustes de la sala y las civilizaciones que se pueden elegir con él. Va en `modes\<nombre>.json`:

```json
{
  "mode": "2 LIGA",
  "gold": 5000,
  "population": 100,
  "noFog": false,
  "explored": false,
  "bonus": 1,
  "civilizations": ["RomeImperial", "Gaul", "Iberia", "CarthageNumidian"]
}
```

| Campo | Qué es |
|---|---|
| `mode` | El nombre del script del modo en `data\GameScripts`, sin `.vs` (`2 LIGA` para `2 LIGA.vs`), hasta 31 caracteres. Si no lo pones, el nombre del archivo json. Vale también para los modos del juego. |
| `gold` | (Opcional) El oro inicial: uno de los valores del desplegable (`5000`, `10000`…). |
| `population` | (Opcional) La población: `50`, `100` o `150` (`LowPop`, `NormalPop` y `HighPop` de `CONST.INI`). |
| `noFog` | (Opcional) `true` quita la niebla, `false` la deja. |
| `explored` | (Opcional) `true` empieza con el mapa explorado. |
| `bonus` | (Opcional) El bono de todos los jugadores, del 0 (ninguno) al 3, el mismo número que el botón de bono de la sala. Desactivar bonos deja de poder marcarse. |
| `civilizations` | (Opcional) Las civilizaciones que se pueden jugar en ese modo: las del juego (`Gaul`, `RepublicanRome`…) o el `id` de una del mod. Una base del juego deja jugar todas sus variantes; una variante, solo esa. |

Mientras ese modo está elegido, la sala no deja cambiar lo que fija: el anfitrión ve esas filas sin poder tocarlas. Un jugador con una civilización que el modo no permite pasa a la primera de la lista, y la civilización aleatoria se sortea solo entre las permitidas. Lo aplica el anfitrión al crear la partida, así que todos juegan con lo mismo. Las partidas guardadas y los escenarios con civilizaciones fijas no cambian.

## Online

Un modo nuevo cambia la jugabilidad: todos los jugadores tienen que llevar el mismo mod.
