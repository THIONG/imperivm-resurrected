# Unidades nuevas

Una unidad nueva son tres cosas: una clase (las estadísticas), una orden de reclutar en un edificio, y sus imágenes y textos. Todo son
archivos del mod; no hay que programar nada.

Por defecto la unidad reutiliza el modelo y las animaciones de una del juego. Si quieres dibujos propios, mira [Gráficos propios](graficos.md):
convierte tus PNG a `.rle` y genera el `.ent.xml`.

Ejemplo listo para copiar: [`ejemplos/unidad-ejemplo`](ejemplos/unidad-ejemplo), el Veterano romano: más fuerte que el hastatus, se recluta
en el cuartel de Roma republicana. Con sprites propios: `ejemplos/unidad-graficos-ejemplo` (ver [Gráficos propios](graficos.md)).

## Camino rápido

Necesitas la carpeta `DATA` del juego ([`extraer-datos.js`](herramientas/extraer-datos.js)). El Veterano del ejemplo sale con:

```
node herramientas/crear-unidad.js <DATA> <carpeta del mod> RVeteran --base RHastatus --vida 260 --dano 20 --armadura 14 --edificio RBarracks --coste-oro 180 --tiempo 9000 --tecla v --nombre Veterano --nombre-en Veteran
```

Genera, dentro de la carpeta del mod:

- `data\classes\RVeteran.sc.xml`: copia entera de la clase base, con tu id y tus estadísticas.
- `data\commands\RVeteran.xml`: la orden de reclutarla, copia de la de la base.
- `loc\spanish.loc.xml` (y `english.loc.xml` con `--nombre-en`, `italian.loc.xml` con `--nombre-it`): nombre, plural y texto del botón. Si el archivo ya existe, añade las entradas.
- `mod.json`, solo si no había uno.

| Opción | Para qué |
|---|---|
| `--base` | La unidad del juego que se copia (obligatoria). Vale su id o su `altid` |
| `--vida`, `--dano`, `--armadura`, `--velocidad` | Estadísticas. `--armadura` pone `armor_slash` y `armor_pierce`. Sin ellas, las de la base |
| `--edificio` | Dónde se recluta. Sin él, donde se recluta la base (si son varios, el de su misma civilización) |
| `--coste-oro`, `--coste-comida`, `--tiempo` | Precio y tiempo de reclutar (en milisegundos) |
| `--tecla` | La tecla del botón. Sin ella, una letra del nombre que esté libre en ese edificio |
| `--nombre`, `--plural` | Nombre en español. El plural se adivina; corrígelo con `--plural` si no vale |
| `--nombre-en`, `--plural-en`, `--nombre-it` | Nombre en inglés y en italiano. `display_name` es la clave del texto, así que con `--nombre-en` la clave es el nombre en inglés |

Si algo está mal (id repetido, base que no es una unidad, edificio que no existe) dice por qué y no escribe nada. Tampoco pisa archivos que ya existan.

Límites:

- Solo copia unidades que el juego recluta con una orden normal (`method="train"`). Las que se contratan en grupo (gladiadores, vikingos…) no.
- Retrato y botón son los de la base. Para los tuyos, mira [Imágenes](#3-imágenes) y cambia las rutas en la clase y en la orden.
- La IA no reclutará la unidad.
- Pasa el resultado por el [validador](validar.md) con `--datos`. Lo que genera es igual que el ejemplo (salvo las imágenes).

## Lo que hace por dentro (cómo hacerlo a mano)

Si prefieres escribirlo tú, o necesitas algo que la herramienta no cubre, esto es lo que genera.

## 1. La clase

`data\classes\RVeteran.sc.xml`. Parte de una copia entera de una clase parecida del juego, por ejemplo `RHastatus.sc.xml`. Para ver los
archivos del juego, saca `data.pak` con [`extraer-datos.js`](herramientas/extraer-datos.js): `node extraer-datos.js "<carpeta del juego>"
<salida>` los deja en `<salida>\DATA`. Cambia el `id` y las estadísticas:

```xml
<class id="RVeteran" cpp_class="CVXUnit" parent="Melee" entity="Units/RHastatus/RHastatus.ent.xml">
	<properties race="RepublicanRome"/>
	<properties maxhealth="260"/>
	<properties damage="20" armor_slash="14" armor_pierce="14"/>
	<properties speed="60"/>
	<properties unit_specials="Deflection"/>
	<properties icon="gameres/icons/RVeteran.bmp"/>
	<properties display_name="Veteran" display_name_plural="Veterans"/>
	...
</class>
```

- Cópiala entera; no la hagas hija (`parent="RHastatus"`). El juego no hereda del padre la entidad (`entity`, el modelo) ni los sonidos, así que
  una hija sin su `entity` se crea sin modelo y desaparece.
- `entity` apunta al modelo y las animaciones. Aquí son los del hastatus.
- `race` es la civilización: `RepublicanRome`, `Gaul`… o una de tu mod ([Civilizaciones](civilizaciones.md)).
- Las estadísticas habituales son `maxhealth`, `damage`, `armor_slash`, `armor_pierce`, `speed`, `sight` y `range`. Qué hace cada una, y
  cuáles no sirven de nada, está en [Estadísticas](estadisticas.md). Una unidad copiada de una normal hereda `feeds=1` (come); las máquinas
  de asedio y los centinelas del juego lo ponen a 0.
- `unit_specials` son las [especialidades](especialidades.md), las del juego y las tuyas.
- No copies el `altid` de la original. Es un alias que usan los scripts para encontrar «el soldado» de una civilización; dos clases con el
  mismo alias se confunden.
- `edittree_pos` (por ejemplo `Units/Roman/Veteran`) es donde sale en el editor de mapas y en el menú de trucos: una clase con `entity`
  cuya `edittree_pos` empieza por `Units/` sale allí, en el grupo de su civilización. La herramienta la copia de la base y le cambia el
  último nombre.
- Un archivo nuevo en `data\classes\` se carga solo. No hay que registrarlo.

## 2. La orden de reclutar

`data\commands\Veterano.xml`. Es copia de la orden del juego (`trainRHastatus`, en `data\commands\RBARRACKS.XML`) con otra clase y otro
coste. El juego carga todos los `.xml` de `data\commands`.

```xml
<cmd name="trainRVeteran" priority="3"
	button="actions/train RVeteran.bmp" queueicon="gameres/icons/RVeteran.bmp"
	key="v" costgold="180" costfood="0" costpop="1" execdelay="9000"
	rollover="Equip Veteran" method="train" param="RVeteran" sclass="RVeteran" ...>
	<src obj="RBarracks"/>
</cmd>
```

- `<src obj="RBarracks"/>` es el edificio donde sale el botón. Para otro edificio, otra clase (`RBlacksmith`, `RArena`…).
- `param` y `sclass` son la clase que se recluta. `costgold`, `costpop` y `execdelay` (milisegundos) son precio y tiempo.
- `key` es la tecla. Sale en Controles, Contenido, y se cambia ahí ([Teclas de los mods](controles.md)).
- Un edificio nuevo se hace igual: copia entera de una clase de edificio, más sus órdenes con `<src obj="MiEdificio"/>`.

## 3. Imágenes

El botón mide 153 x 51 y el retrato 65 x 76. Se hacen desde un PNG con `herramientas/convertir-imagen.js` ([Imágenes propias](imagenes.md)). Sin
imágenes propias, usa las de la unidad del juego (`button="actions/train RHastatus.bmp"`, `icon="gameres/icons/RHastatus.bmp"`).

## 4. Textos

El nombre de la clase y el texto del botón se traducen con un `loc\<idioma>.loc.xml` ([Idiomas y textos](idiomas.md)):

```xml
<translationtableentry text="Veteran@RVeteran class name" result="Veterano"/>
<translationtableentry text="Veterans@RVeteran class name plural" result="Veteranos"/>
<translationtableentry text="Equip Veteran@trainRVeteran" result="Equipar veterano"/>
```

## Multijugador y límites

- Clases y órdenes son jugabilidad: todos los jugadores llevan el mismo mod.
- La IA no reclutará tu unidad salvo que traigas guiones de IA propios. Ver [Civilizaciones](civilizaciones.md#la-ia).
- Una unidad de una civilización nueva necesita también sus edificios y órdenes. La herramienta de clonado de [Civilizaciones](civilizaciones.md)
  los genera.
