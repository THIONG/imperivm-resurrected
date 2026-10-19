# Civilizaciones

El juego trae 8 civilizaciones, numeradas del 0 al 7: Galia, Roma republicana, Cartago, Iberia, Roma imperial, Britania, Egipto y Germania.

Imperivm Resurrected deja declarar civilizaciones nuevas con números a partir del 10. Cada una hereda de una civilización del juego (su
*base*) todo lo que tu mod no defina. Las herramientas necesitan Node.js.

Ejemplo listo para copiar: [`ejemplos/civilizacion-ejemplo`](ejemplos/civilizacion-ejemplo).

## Declarar una civilización

Un archivo `civs\<id>.json` en tu mod:

```json
{
  "id": "dacia",
  "index": 10,
  "base": "Gaul",
  "name": {"spanish": "Dacia", "english": "Dacia", "italian": "Dacia"},
  "letter": "D"
}
```

| Campo | Qué es |
|---|---|
| `id` | El nombre: letras, números y `_`, empieza por letra, hasta 24. Lo usas en `race="..."` y en los scripts. |
| `index` | Un número del 10 al 99. No lo cambies una vez publicado: las partidas guardadas lo recuerdan. Dos mods no pueden usar el mismo. |
| `base` | La civilización del juego de la que hereda: `Gaul`, `RepublicanRome`, `Carthage`, `Iberia`, `ImperialRome`, `Britain`, `Egypt` o `Germany`. Si no la pones, `Gaul`. |
| `name` | Cómo se llama en cada idioma, o un solo texto. |
| `letter` | La letra que prefija sus clases y órdenes (`DSwordsman`, `trainDSwordsman`). Si no la pones, usa la de la base. Pon una propia solo si tienes clases y órdenes con esa letra: el juego construye con ella nombres como el del entrenamiento de campesinos. |
| `template` | (Opcional) Prefijo de tus plantillas de ciudad y aldea, para que el mapa aleatorio no use las de la base. Ver [Plantillas](plantillas.md). |
| `internalName` | (Opcional) El nombre con el que el juego busca los archivos de interfaz de la civilización; por defecto, el de la base. |
| `folder` | (Opcional) El nombre que sale en los textos de selección. |
| `random` | (Opcional, por defecto `true`) Si entra en el sorteo de civilización aleatoria, también en mapas aleatorios. La IA solo la saca si además tiene `"ai": true`. Pon `false` para excluirla. |
| `biome` | (Opcional) Una civilización del juego: el terreno, los árboles y los objetos que el mapa aleatorio pone alrededor de su inicio son los de esa. Sin esto, los de la base. Ver *El bioma*. |
| `ai` | (Opcional) Con `true`, la IA juega con tus clases y tus órdenes. Si no, juega con la civilización base. La herramienta de clonado lo pone a `true`. Ver *La IA*. |
| `ownAi` | (Opcional, con `ai`) Para mods que traen todos sus guiones de IA y quieren ver la civilización tal cual (`Dacia`, no su base). |
| `alias` | (Opcional, lo escribe la herramienta de clonado) `classes` y `commands`: nombre en la base a nombre en tu civilización (`"GArcher": "DArcher"`, `"trainGSwordsman": "trainGSwordsman Dacia"`). La IA los usa para traducir. |
| `variant` | (Opcional) Con `true`, es una variante de su base: juega con las clases de la base y los scripts la ven como la base. Ver *Variantes*. |
| `replacesBase` | (Opcional) Con `true`, la base deja de salir en la sala y esta civilización ocupa su sitio. Ver *Variantes*. |

Con solo `id`, `index` y `base` tienes una copia de la base con otro nombre. Desde ahí cambias lo que quieras.

## Usarla en tus clases y scripts

En el `.sc.xml` de cada unidad o edificio:

```xml
<properties race="Dacia"/>
```

El juego solo conoce los nombres de sus 8 civilizaciones. El mod reconoce el `id` de las tuyas (sin distinguir mayúsculas) y se lo pone a la
clase. Si pones `internalName` a mano, también vale ese. El que viene por defecto (el de la base) no cuenta, así que `race="Gaul"` sigue
siendo Galia aunque tengas una civilización clonada de ella.

Una clase con `parent=` hereda estadísticas y comportamientos, pero no la entidad (`entity`, el modelo) ni los sonidos: vuélvelos a poner o
la unidad se crea sin modelo y desaparece. Por eso la herramienta de clonado copia las clases enteras.

En los scripts, cada civilización declarada es una constante con su `id` tal como lo escribes, con la primera letra en mayúscula (`dacia` da
`Dacia`, `romeImperial` da `RomeImperial`). También vale con el resto en minúsculas (`Romeimperial`):

```
if (GetPlayerRace(player) == Dacia)
	...
```

Los guiones de IA del juego comparan con `Gaul`, `RepublicanRome`… y no conocen las tuyas. Con `"ai": true` ven la base. Para saber con qué
civilización juega de verdad un objeto, usa `Mod_TrueRace(objeto)`.

## En la sala y en el mapa

Las civilizaciones declaradas salen en los desplegables de la sala de multijugador, detrás de las del juego. En un mapa aleatorio la tuya usa el terreno de su bioma y empieza con una ciudad y una aldea hechas con sus clases
(`DTownhall`, `DBarracks`…). Si no le das plantillas propias, la distribución de los edificios es la de la base. La interfaz del jugador (barras de órdenes, esquinas,
iconos rápidos y la pantalla del final) es la de su base.

Si algo no sale, mira `Logs\imperivm\imperivm-mod.log`: las líneas con la etiqueta `civilizations` dicen qué se ha registrado y qué ha fallado (un `id`
que no vale, un `index` que falta o se repite, una `base` que no existe). Los errores salen también en la ficha del mod en el gestor.

## Variantes

Una variante es una civilización del juego con otro nombre en la sala y algo propio al empezar la partida. Por ejemplo, un mod en el que
Roma elige al principio entre republicana, imperial y púnica puede ofrecer las tres en la sala:

```json
{
  "id": "RomeImperial",
  "index": 21,
  "base": "RepublicanRome",
  "name": {"spanish": "Roma imperial", "english": "Roman Empire", "italian": "Roma imperiale"},
  "variant": true,
  "replacesBase": true
}
```

Con `"variant": true`:

- Juega con las clases y las órdenes de la base. No lleva clases propias ni `letter`, salvo si sustituye a la base entera (ver abajo).
- `GetPlayerRace` devuelve la base, así que los scripts del juego y de otros mods la tratan como a la base (barcos, centinelas, IA).
- `Mod_PlayerCiv(jugador)` devuelve la civilización elegida en la sala (21 en el ejemplo). Lo que la distingue va en un
  [parche de script](parches.md), por ejemplo investigar al empezar la Roma que toca:

```
@before while (EnvReadInt(set, "ChooseRomanRace") != 1)
	if (Mod_PlayerCiv(.player) == RomeImperial && set.CanResearch("ChooseImperialRome"))
		set.Research("ChooseImperialRome");
```

- La IA también la juega: `ai` vale `true` si no pones otra cosa.
- Con `"replacesBase": true` la base deja de salir en la sala y sus variantes ocupan su sitio. En el sorteo aleatorio, la parte de la
  base se reparte entre ellas. Un jugador que tuviera elegida la base pasa a la primera variante. Los mapas y las partidas guardadas que
  fijan la base la siguen usando.
- Una variante con `"replacesBase": true` puede cambiar además la letra y las plantillas de su base durante toda la partida. Con
  `letter`, el juego construye con esa letra los nombres de las clases y las órdenes de la base (`GetRaceStrPref`, el entrenamiento de
  campesinos, los cuarteles, los centinelas, la contratación de héroes). Con `template`, las ciudades y las aldeas de la base, también
  las neutrales del mapa aleatorio, salen de tus plantillas `<template> Town N` y `<template> Village N`. Es lo que hace falta para
  sustituir una civilización entera. Por ejemplo, Grecia ocupa el sitio de Roma imperial con `"base": "ImperialRome"`, `"letter": "H"` y
  `"template": "Greek"`, y sus clases llevan `race="ImperialRome"` y la letra `H` (`HTownhall`, `HHoplite`). Si dos variantes activas
  dan a la misma base letras o plantillas distintas, vale la primera y el gestor avisa en la otra.

## Clonar una civilización

Escribir a mano las decenas de clases y órdenes de una civilización no tiene gracia. `herramientas/clonar-civilizacion.js` clona una del juego.
Necesita la carpeta `DATA` del juego, que sacas con [`extraer-datos.js`](herramientas/extraer-datos.js):

```
node extraer-datos.js "<carpeta del juego>" <salida>
node clonar-civilizacion.js <salida>\DATA <carpeta del mod> Dacia D Gaul 10
```

Genera `civs\dacia.json`, una copia de cada clase de la base (`DSwordsman` es copia de `GSwordsman`, con `race="Dacia"`), las órdenes que
nombran clases de la base (`trainGSwordsman Dacia`, `trainpeasantd`, `hireheroD`…) y parches que añaden las clases nuevas a las órdenes
compartidas. También clona los héroes `GHero1`…`GHero7`, que cuelgan de `GaulHero` y no llevan `race`: sin héroes con modelo, contratar uno
cuelga el juego. A partir de ahí editas lo que quieras. El resultado de ejemplo está en
[`ejemplos/civilizacion-ejemplo`](ejemplos/civilizacion-ejemplo).

La herramienta no escribe el campo `template`. Si quieres plantillas propias, lo añades tú ([Plantillas](plantillas.md)).

Las copias no llevan el `altid` de las originales. El `altid` es un alias único (`Barrack`, `Village`) con el que los scripts y la IA buscan
«el cuartel» de una civilización. Si dos clases lo repiten, la IA de una de las civilizaciones deja de ver sus edificios. Si escribes las clases a mano, no lo copies.

## La IA

Por defecto, la IA que elige una civilización de mod juega con su base. Con `"ai": true` juega con tus clases y tus órdenes. Los guiones de IA
del juego deciden por la civilización del edificio central y construyen los nombres a partir de ella, así que el juego les hace ver la base y
traduce lo que piden:

- Las órdenes e investigaciones que piden por nombre (`trainGSwordsman`, `Gaul Axeman`, `hireheroG`…) pasan a las de tu civilización, según
  `alias.ordenes`, solo en objetos de una IA de tu civilización.
- Tus clases cuentan como las de la base: `DArcher` es una `GArcher` para recuentos, cuarteles, templos y contraunidades.

La herramienta de clonado rellena las dos tablas. Si escribes a mano tus clases, rellénalas tú. Para el jugador humano no cambia nada.

Límites de la IA:

- Tu IA se comporta como la de la base, salvo que le des otra táctica.
- Una unidad que no esté en las seis ranuras de la base, no la recluta.
- Una investigación nueva (no clonada de una de la base) no la conoce.
- Los costes que lee antes de pagar son los de la base, aunque la orden cobre lo tuyo.

**Otra táctica para tu civilización.** El juego elige la táctica de cada IA al empezar, en `data\ai\GetTacticScript.vs`. Un
[parche de script](parches.md) puede añadirle un trozo para tu civilización. Como la IA ve la base, la civilización real se pregunta con
`Mod_TrueRace(set.GetCentralBuilding)`. El ejemplo [`ejemplos/ia-tactica-ejemplo`](ejemplos/ia-tactica-ejemplo) hace que Dacia use una táctica
del juego que ninguna IA usa. Los parches de varios mods se suman, así que dos civilizaciones pueden elegir su táctica a la vez.

Con `"ownAi": true` el mod tiene que traer todos los guiones de IA, porque los del juego ven la civilización 10 y sus tablas no la conocen.
Lo normal es dejar `"ai": true` y cambiar solo la táctica.

## El bioma

En un mapa aleatorio, lo que rodea el inicio de cada jugador (hierba, arena, árboles, adornos) lo decide su civilización. La tuya usa el de su
`base`, o el que pongas en `biome`:

```json
{ "id": "dacia", "index": 10, "base": "Gaul", "biome": "Britain" }
```

El juego solo distingue cuatro biomas de terreno y árboles, así que las civilizaciones los comparten de dos en dos:

| Terreno y árboles | Lo usan |
|---|---|
| 1.º | Galia y Roma imperial |
| 2.º | Roma republicana y Britania |
| 3.º | Cartago y Egipto |
| 4.º | Iberia y Germania |

Los objetos del mapa (ruinas, palmeras, rocas) sí distinguen las ocho; Cartago y Egipto hacen de interruptor «desierto» frente a «templado».
La ciudad inicial sigue siendo la de la base salvo que uses `template`. Un bioma totalmente propio, con terreno nuevo, no se puede.

## Lo que no se puede

- Partidas contra la IA fuera de la sala de multijugador (escenarios y campañas) con civilizaciones de mods.
- Editor de mapas: las clases de tu civilización salen mezcladas en las carpetas de su base.
- Guiones de IA nuevos sin traer todos (ver *La IA*).
