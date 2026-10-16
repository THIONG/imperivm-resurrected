# Scripts

Los scripts (`.vs`) son el lenguaje con el que el juego programa la IA, las habilidades, las condiciones de victoria y el comportamiento de edificios y unidades. Un mod puede traer los suyos y usar las funciones nuevas (`Mod_...`) de Imperivm Resurrected.

Ejemplo para copiar: [`ejemplos/scripts-ejemplo`](ejemplos/scripts-ejemplo). Ojo: ese ejemplo cambia el equilibrio (los hastatus hacen el doble de daño y se curan al matar), así que no lo actives en una partida en serio.

## Cómo es un script

Un archivo de texto `.vs`. La primera línea es un comentario con la firma: lo que devuelve y los parámetros que recibe. Cada sitio donde se usa un script espera una firma concreta.

```
// void, Obj This

Sleep(rand(500) + 100);
Mod_Log("Ha entrado en el mapa un hastatus romano");
while(true) Sleep(50000);
```

Para ver cómo escribe el juego los suyos, vuélcalos con `Dump=` (ver [Parches](parches.md); hay que dar cada archivo por su nombre, sin comodines) o míralos en la carpeta `DATA` que saca `extraer-datos.js`: `data/subai/*.vs` (comportamientos y habilidades), `data/ai/*.vs` (IA) y `data/GameScripts/*.vs` (victoria). Todas las funciones del lenguaje, con su firma, están en [funciones-del-juego.txt](funciones-del-juego.txt).

## Dónde se engancha un script

| Sitio | Cómo | Firma |
|---|---|---|
| Comportamiento de una clase | `<behavior script="data/mod/mi.vs"/>` en el `.sc.xml` de la clase (con un [parche](parches.md) si es del juego). Corre con cada objeto de la clase mientras existe. | `// void, Obj This` |
| Orden o habilidad | `<method sig="miOrden" vs="data/mod/mi.vs"/>` en la clase, y un `<cmd name="miOrden">` en `data\commands` | según la orden: mira las del juego |
| Al morir una unidad | `<method sig="ondie" vs="data/mod/mi.vs"/>` en la clase (y `sig="onkill"` para cuando mata) | mira `data/subai/unit_on_kill.vs` |
| Condición de victoria | un `.vs` en `data/GameScripts` (ver [Modos de victoria](victoria.md)) | `// void, int player, str param` |
| Objetos de `items.xml` | atributos como `use_script` o `equip_script` (ver [Objetos](objetos.md)) | según el atributo |

`ondie` y `onkill` son métodos (`<method sig=...>`), no atributos. Si escribes `ondie="..."` en la clase no pasa nada, y el juego no avisa.

Un script de comportamiento no debe terminar: acábalo con `while(true) Sleep(50000);`.

Una clase solo admite 8 `<behavior>`.

## Eventos

Pon un script en `events\<Evento>.vs` dentro de tu mod y el juego lo ejecuta cuando ocurre el evento, en todos los PCs a la vez. Si varios mods tienen el mismo evento, se ejecutan todos. Para probar sin crear un mod, ponlo en `<juego>\mod\events\<Evento>.vs`.

| Evento | Cuándo | Firma |
|---|---|---|
| `OnGameStart` | Al empezar una partida de escaramuza, escenario o multijugador, justo después de arrancar los scripts de victoria. Puede usar `Sleep` y se guarda con la partida. | `// void` |
| `OnObjectDie` | Cuando muere cualquier objeto del mapa (unidades, héroes, edificios, adornos…), justo antes de que el juego procese su muerte: el objeto todavía existe. Es el mismo momento que el `ondie` de su clase. | `// void, Obj victim` |
| `OnObjectKill` | Cuando un golpe mata a un objeto. `killer` dio el golpe, `victim` murió. Mismo momento que el `onkill` de la clase del atacante. | `// void, Obj killer, Obj victim` |
| `OnSettlementCaptured` | Cuando un asentamiento cambia de dueño durante la partida (captura, o un script que se lo da a otro). No salta mientras se prepara el mapa. `building` es su edificio central. Jugadores de 1 a 16: los asentamientos sin dueño empiezan en poder de los jugadores 15 y 16; el 0 solo sale si no hay dueño. | `// void, Obj building, int oldPlayer, int newPlayer` |
| `OnAttackResolved` | Con cada golpe que hace daño, justo antes de aplicarlo: ya cuentan la armadura, las especialidades y los bonos. `damage` es lo que va a recibir la víctima; `Mod_SetAttackDamage(n)` lo cambia (0 lo anula). No salta si el golpe no hace daño (desvíos, parry). | `// void, Obj attacker, Obj victim, int damage` |
| `OnPlayerDefeated` | Cuando el juego declara derrotado a un jugador (1 a 8), justo después de marcarlo. Una vez por jugador. | `// void, int player` |

Salvo `OnGameStart`, los eventos se ejecutan al momento y no pueden esperar: nada de `Sleep`. `OnAttackResolved` y `OnObjectDie` saltan muchas veces por segundo en una batalla. Filtra pronto lo que no te interesa (`if (!victim.IsValid) return;`, `victim.IsMilitary`, `victim.player`, `victim.IsHeirOf("RHastatus")`…) y no hagas trabajo caro.

En `OnSettlementCaptured`, `building` es un `Obj`. Para llegar a su asentamiento pasa por `AsBuilding`: `building.AsBuilding.settlement`.

Con `OnObjectKill` se hacen habilidades pasivas. Un hastatus que se cura al matar:

```
// void, Obj killer, Obj victim
if (!killer.IsValid) return;
if (!killer.IsHeirOf("RHastatus")) return;
if (!victim.IsMilitary) return;
killer.Heal(25);
```

Con varios mods en `OnAttackResolved`, `damage` es siempre el daño original: se calcula una vez. Dos mods que hagan `Mod_SetAttackDamage(damage * 2)` no se acumulan. Si quieres sumarte a lo que ya cambió otro mod, parte de `Mod_AttackDamage()`.

### Reaccionar a que aparezca un objeto

No hay un evento para esto (los objetos se crean en decenas de sitios, también al cargar un mapa o una partida). Se hace con un comportamiento: un `<behavior>` añadido con un parche a una clase base (`Military` para todos los soldados, `Unit` para todas las unidades) lo heredan todas las clases que cuelgan de ella, y corre una vez con cada objeto cuando aparece. Mira `patches\data\classes\Military.sc.xml` y `data\mod\soldier.vs` en el ejemplo.

## Funciones `Mod_`

La versión actual de la API es la 6. `Mod_ApiVersion()` te la da, y un mod puede exigir una mínima.

| Función | Qué hace |
|---|---|
| `Mod_Log(str texto)` | Escribe `script: texto` en `Logs\imperivm\imperivm-mod.log` (nivel `I`). Hasta 2.000 líneas por sesión. |
| `Mod_LogInt(str texto, int valor)` | Escribe `script: texto: valor`. |
| `Mod_IsActive(str id)` : `bool` | ¿Está activo en el perfil el mod con ese `id` (el de su `mod.json`)? |
| `Mod_ApiVersion()` : `int` | La versión de la API. |
| `Mod_SetInt(str nombre, int valor)` | Guarda una variable (API 2). |
| `Mod_GetInt(str nombre)` : `int` | Lee una variable; si no existe vale 0 (API 2). |
| `Mod_AddInt(str nombre, int suma)` : `int` | Suma a una variable y devuelve el valor nuevo (API 2). |
| `Mod_HasKey(str nombre)` : `bool` | ¿Existe la variable? (API 2). |
| `Mod_AttackDamage()` : `int` | Solo en `OnAttackResolved`: el daño del golpe que se resuelve. Fuera de él da 0 (API 3). |
| `Mod_SetAttackDamage(int daño)` | Solo en `OnAttackResolved`: cambia el daño antes de aplicarlo. Fuera de él no hace nada (API 3). |
| `Mod_HasSpecial(Unit unidad, int bit)` : `bool` | ¿Tiene la unidad esa especialidad (bit 0 a 63)? (API 4). Ver [Especialidades](especialidades.md). |
| `Mod_SetSpecial(Unit unidad, int bit, bool puesta)` | Pone o quita una especialidad (API 4). |
| `Mod_TrueRace(Obj objeto)` : `int` | La civilización real de un objeto (10 o más si es de un mod), aunque para la IA su raza sea la de su base (API 5). Ver [Civilizaciones](civilizaciones.md). |
| `Mod_GetOption(str id)` : `bool` | ¿Está marcada en la sala la opción `id` (la de `options\<id>.json`)? Da lo mismo en todos los PCs (API 6). Ver [Opciones de partida](opciones.md). |
| `Mod_GetOptionValue(str id)` : `int` | El valor elegido de la opción `id`: 0 o 1 en un interruptor, y el número de la opción elegida (desde 0) en una lista (API 6). |

Las funciones nuevas se añaden al final y las que ya existen no cambian. Si tu mod usa alguna, necesita una versión de Imperivm Resurrected que la traiga. La versión de la API entra en la comprobación de multijugador cuando hay mods de jugabilidad: todos tienen que tener la misma.

### Variables de mods

Una tabla de nombre → entero para que tu mod se acuerde de cosas (contadores, marcas, tiempos):

```
Mod_AddInt("capturas", 1);
if (Mod_GetInt("capturas") >= 3) ...
```

- Se guardan con la partida (también con los autoguardados) y vuelven al cargarla. Una partida nueva empieza sin variables.
- Son iguales en todos los PCs porque los scripts de partida corren igual en todos. Cámbialas solo desde scripts de partida, nunca según algo local.
- Nombres de hasta 64 letras, hasta 1.024 variables, enteros de 32 bits.
- Son de la partida entera, no de un jugador. Para una por jugador, ponle el jugador al nombre (`"oro1"`, `"oro2"`).
- Se comparten entre todos los mods activos: usa un prefijo propio (`"mimod_capturas"`).

## Multijugador

Los scripts forman parte de la simulación: se ejecutan en todos los PCs, en el mismo momento, y tienen que hacer lo mismo en todos.

- Lee solo cosas iguales para todos: los objetos del mapa, el tiempo de juego, el `rand()` del juego.
- No leas nada local: la cámara, el jugador que mira, la música, el ratón, la hora del sistema. Si el script cambia la partida según algo local, la partida se desincroniza.
- Si algo nace de una tecla o de la interfaz, tiene que ir como una orden del juego, que viaja por la red. Ver [Teclas de los mods](controles.md).

## Partidas guardadas

Una partida guardada lleva dentro el código de los scripts que estaban corriendo. Por eso las funciones del SDK no cambian de número y solo se añaden al final: lo guardado sigue cargando. No reutilices nombres de funciones del juego (`Run`, `Write`…) como si fueran tuyos.

## Errores

- Un script que no compila deja el error (archivo, línea y columna) en `Logs\imperivm\imperivm-mod.log`, como `script: compilation error: …` (nivel `E`), y también en `Logs\comp_out.txt` y `Logs\vxlog.log`. El juego sigue, pero ese script no se ejecuta.
- Los errores al ejecutarse el juego casi nunca los registra. En los eventos sí: el log apunta `Event X (…): the script ended with an error`, hasta 5 veces por script. Para lo demás, usa `Mod_Log` y mira el registro.
