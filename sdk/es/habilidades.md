# Habilidades de tropas

Cualquier unidad, normal o héroe, puede tener habilidades nuevas. Una habilidad es una orden (el botón) más un método de la
clase (el script que la ejecuta), como las de druidas y sacerdotes. Todo son archivos del mod.

Ejemplo listo para copiar: [`ejemplos/habilidades-ejemplo`](ejemplos/habilidades-ejemplo), «Respiro» (tecla `Y`) para los soldados cuerpo a cuerpo.

## Camino rápido

`crear-habilidad.js` escribe las tres piezas. Necesitas la carpeta `DATA` del juego ([Datos del juego](README.md#datos-del-juego)):

```
node herramientas/crear-habilidad.js <DATA> mods/mi-mod respiro --clase Melee --tecla y --coste-energia 5 --nombre Respiro --nombre-en "Second wind"
```

| Opción | Para qué |
|---|---|
| `--clase` | La clase que recibe el botón. Vale una base (`Melee`, `Military`, `Unit`) y las hijas la heredan, o una concreta (`RHastatus`, `Hero`). Tiene que existir en el juego |
| `--tecla` | Una letra |
| `--coste-energia` | Energía de 1 a 9. Si la pones, el script ya trae el cobro |
| `--nombre`, `--nombre-en` | El nombre en español y en inglés. Sin `--nombre-en`, el botón dice el id o el nombre si no lleva acentos |
| `--descripcion`, `--descripcion-en` | El texto de ayuda, igual |
| `--boton` | Un botón del juego (`Actions/Mass Healing.bmp`, el de por defecto) o el tuyo ([Imágenes propias](imagenes.md)) |

Genera:

```
data\commands\Respiro.xml              la orden
patches\data\classes\Melee.sc.xml      el método en la clase (si ya hay un parche, se suma)
data\mod\respiro.vs                    el script, con la firma y el cobro de energía; el cuerpo lo escribes tú
loc\spanish.loc.xml                    los textos en español (se suman a los que haya)
mod.json                               solo si la carpeta no tiene
```

Avisa si la tecla ya la usa otra orden de esa clase, de una clase padre o de una hija, y te dice qué teclas quedan libres. Se genera igual. No pisa archivos: si el id ya existe en tu mod o en el juego, se para sin escribir nada.

Después escribe el cuerpo de `data\mod\respiro.vs` y pasa el [validador](validar.md).

## Lo que hace por dentro (cómo hacerlo a mano)

### Las tres piezas

**1. La orden.** Un `<cmd>` en un archivo nuevo de `data\commands` (el juego carga todos los `.xml` de esa carpeta):

```xml
<cmd name="respiro" priority="10" offset="1"
	button="Actions/Mass Healing.bmp" key="y"
	rollover="Second wind" description="..."
	groupverifier="" groupdispatch="" voidgroupdispatch="1"
	coststamina="5">
	<src obj="Melee"/>
</cmd>
```

- `name` es el nombre de la orden y también el `sig` del método.
- `button` es la imagen del botón: una del juego, o una tuya ([Imágenes propias](imagenes.md)).
- `<src obj="…"/>` son las clases que tienen el botón. Vale una clase base, y las hijas lo heredan: `Melee` (cuerpo a cuerpo), `Military`,
  `Unit` (todas) o una concreta (`RHastatus`). Ojo con `Military`: incluye a los héroes, y la tecla `Y` ya la usa la habilidad `skill_battle_cry`
  del héroe. Con `Melee` no choca.
- `coststamina` es la energía que cuesta. La barra `stamina` de un soldado del juego tiene 10, así que el 5 del ejemplo gasta la mitad. El juego
  no la cobra solo: la cobra tu script. Usa un valor de 1 a 9. El tooltip muestra el coste como energía si es menor de 10 y como lealtad si es 10 o más.
- `key` es la tecla. Sale en Controles, Contenido, y se cambia ahí ([Teclas de los mods](controles.md)).

**2. El método.** En la clase, con un [parche](parches.md) para no sustituir el archivo entero. Va en `patches\data\classes\Melee.sc.xml`:

```xml
<patch>
  <add select="/class"><method sig="respiro" vs="data/mod/respiro.vs"/></add>
</patch>
```

Las clases hijas heredan los métodos.

**3. El script.** `data\mod\respiro.vs`. Una orden inmediata recibe `// void, Obj me`, la unidad que la ejecuta:

```
// void, Obj me

Unit this;
int cost;

this = me.AsUnit();
if (!.IsAlive)
	return;

cost = GetCmdStaminaCost("respiro");
if (.stamina < cost)
	return;
.SetStamina(.stamina - cost);
.Heal(60);
```

Las órdenes con objetivo (un clic sobre otra unidad) reciben `// void, Obj This, Obj target`. Mira cómo lo hacen las del juego en
`data\subai\druid_*.vs` y `data\commands\DRUID.XML`. Todas las funciones están en [funciones-del-juego.txt](funciones-del-juego.txt).

### Nombre y descripción

La orden se traduce con su nombre como contexto. Un `loc\<idioma>.loc.xml` ([Idiomas y textos](idiomas.md)):

```xml
<translationtableentry text="Second wind" context="respiro" result="Respiro"/>
<translationtableentry text="The soldier recovers 60 health (costs stamina)" context="respiro" result="El soldado recupera 60 de vida (gasta energía)"/>
```

El `text` es el de `rollover` y `description` de la orden. En [Unidades nuevas](unidades.md) está la otra forma (`text="…@contexto"`); `idiomas.md` dice
que valen las dos.

## Habilidades pasivas

Una habilidad sin botón (curarse al matar, daño extra, un aura…) no necesita orden. Usa un [evento](scripts.md) (`OnObjectKill`, `OnObjectDie`) o
un `<behavior>` en la clase.

## Multijugador

Sigue las reglas de [Scripts](scripts.md#multijugador): no leas nada local. Todos los jugadores llevan el mismo mod.

## Lo que no se puede

- Crear especialidades (`unit_specials`) ni habilidades de héroe (`HeroSkills`) nativas nuevas con efecto propio: su efecto lo trae el juego. Puedes usar y combinar las 25 que existen. Las órdenes con script de arriba cubren casi todo lo demás, y las especialidades propias
  van por otro lado ([Especialidades](especialidades.md)).
- Efectos visuales y sonidos nuevos: reutiliza los del juego.
