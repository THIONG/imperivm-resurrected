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

En los scripts, cada civilización declarada es una constante con su `id` en mayúscula inicial:

```
if (GetPlayerRace(player) == Dacia)
	...
```

Los guiones de IA del juego comparan con `Gaul`, `RepublicanRome`… y no conocen las tuyas. Con `"ai": true` ven la base. Para saber con qué
civilización juega de verdad un objeto, usa `Mod_TrueRace(objeto)`.

## En la sala y en el mapa

Las civilizaciones declaradas salen en los desplegables de la sala de multijugador, detrás de las del juego. En un mapa aleatorio la tuya usa el terreno de su bioma y empieza con una ciudad y una aldea hechas con sus clases
(`DTownhall`, `DBarracks`…). Si no le das plantillas propias, la distribución de los edificios es la de la base.

Si algo no sale, mira `Logs\imperivm\imperivm-mod.log`: las líneas con la etiqueta `civilizations` dicen qué se ha registrado y qué ha fallado (un `id`
que no vale, un `index` que falta o se repite, una `base` que no existe). Los errores salen también en la ficha del mod en el gestor.

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
