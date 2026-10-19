# SDK de Imperivm Resurrected

**Español** · [English](../en/README.md) · [Italiano](../it/README.md)

Imperivm Resurrected es un mod para Imperivm HD (Great Battles of Rome) que deja hacer mods que antes no se podían: civilizaciones nuevas, habilidades, objetos, modos de victoria, idiomas, scripts que reaccionan a la partida y más. Esto son las guías y herramientas para hacerlos. Cada guía trae un mod de ejemplo en [`ejemplos/`](ejemplos).

## Empieza aquí

1. Instala Imperivm Resurrected: copia su contenido dentro de la carpeta del juego (`...\steamapps\common\Imperivm - Great Battles of Rome HD`) y abre el juego desde Steam.
2. Copia una carpeta de [`ejemplos/`](ejemplos) (por ejemplo `parches-ejemplo`) a `mods\`, dentro de la carpeta del juego. Para empezar un mod desde cero, usa [`plantilla-mod/`](plantilla-mod).
3. En el menú principal, botón **Contenido adicional**: activa el mod en tu perfil.
4. Reinicia el juego si no se ha reiniciado solo.
5. Abre `Logs\imperivm\imperivm-mod.log`, en la carpeta del juego. Ahí salen los errores de tu mod y lo que ha cargado.

Antes de abrir el juego, pasa tu mod por el [validador](validar.md).

## Cómo es un mod

Una carpeta dentro de `mods\` con un `mod.json` y los archivos que cambia o añade, con las mismas rutas que usa el juego:

```
mods\
  mi-mod\
    mod.json
    data\classes\MiUnidad.sc.xml     <- archivo nuevo o que sustituye al del juego
    loc\spanish.loc.xml              <- textos que se suman a los del juego
```

No metas ejecutables ni scripts del sistema (`.exe .dll .asi .bat .cmd .com .scr .ps1 .vbs .js .msi .sys .cpl .lnk .reg .jar .hta .wsf .pif`). Si hay uno, el mod no se carga. Un `.js` suelto, como un generador tuyo, cuenta.

## `mod.json`

```json
{
  "id": "mi-mod",
  "name": {"spanish": "Mi mod", "english": "My mod", "italian": "Il mio mod"},
  "version": "1.0",
  "authors": ["Yo"],
  "description": {"spanish": "Qué hace mi mod.", "english": "What my mod does."},
  "category": "Gameplay & Balance",
  "redistributable": true
}
```

| Campo | Para qué |
|---|---|
| `id`, `name`, `version`, `authors`, `description` | La ficha del mod en el gestor. `name` y `description` valen como un texto solo o como un objeto con un texto por idioma (se ve el del idioma del juego; si no, el inglés; si no, el primero). |
| `category`, `tags` | La categoría principal de GameBanana y las etiquetas: más categorías (`Type`), jugadores (`Players`) e idioma (`Language`). Ver [Publicar](publicar.md). |
| `redistributable` | Con `true`, el anfitrión de una sala puede enviar el mod a quien no lo tenga. Por defecto es `false`. |
| `license` | Texto libre con la licencia. |
| `requires` | Mods que necesita: una lista de ids, o de objetos `{"id": "...", "version": "..."}`. |
| `incompatibleWith` | Lista de ids de mods con los que no se puede activar. |
| `loadAfter`, `loadBefore` | Lo que pides al [orden de carga](referencia.md#1-estructura-de-un-mod), por ids; el resto lo calcula el juego. Con `loadAfter` tu mod va por encima de ese: sus archivos, parches y textos ganan (salvo un archivo que el otro trae suelto y el tuyo dentro de un `.pak`: gana el suelto). |
| `sources` | `{"url": "...", "gamebanana": 123456}`. Lo escribe el juego al instalar desde Explorar contenido; sirve para avisar de versiones nuevas. Si viene de la sección Sounds, el número va en `gamebananaSound`. |
| `language` | Declara un idioma nuevo. Ver [Idiomas](idiomas.md). |
| `commands` | Nombre de cada archivo de órdenes en la pantalla de Controles. Ver [Teclas](controles.md). |

> Hasta la 0.31.50 los nombres de estos campos eran en español (`nombre`, `autores`, `descripcion`, `etiquetas`, `categoria`, `redistribuible`, `licencia`, `requiere`, `incompatibleCon`, `cargarDespuesDe`, `cargarAntesDe`, `fuentes`, `idioma`, `ordenes`…). Los antiguos se siguen leyendo, pero escribe los nuevos.

## Guías

- [Idiomas y textos](idiomas.md): cambiar textos del juego o añadir un idioma sin sustituir el archivo entero.
- [Teclas de los mods](controles.md): cómo salen las órdenes de tu mod en Controles y cómo nombrarlas.
- [Parches](parches.md): cambiar parte de un XML, INI o script del juego sin sustituirlo; dos mods pueden tocar el mismo archivo.
- [Scripts](scripts.md): scripts `.vs`, eventos y las funciones `Mod_...`.
- [Civilizaciones](civilizaciones.md): declarar una civilización nueva o clonar una del juego, con su IA, y jugar con ella en la sala.
- [Plantillas de ciudad y aldea](plantillas.md): ciudad y aldea iniciales propias de una civilización de mod (`crear-plantilla.js`).
- [Unidades nuevas](unidades.md): clase, orden de reclutarla, imágenes y textos, reutilizando el modelo de otra unidad.
- [Estadísticas de unidades](estadisticas.md): qué atributo toca cada cosa, cuáles no hacen nada y cuáles cuelgan el juego.
- [Especialidades de unidad](especialidades.md): especialidades nuevas y su efecto con scripts.
- [Habilidades de tropas](habilidades.md): habilidades nuevas para unidades o héroes (orden con botón y tecla, más un método con script).
- [Objetos nuevos](objetos.md): amuletos, hierbas y gemas de los héroes.
- [Formaciones nuevas](formaciones.md): formaciones de héroe con sus bonos y su botón.
- [Fortines y edificios](fortines.md): fortines neutrales nuevos en los mapas aleatorios (`crear-fortin.js`) y clases de torres, muros y puertas.
- [Modos de victoria](victoria.md): un script en `data\GameScripts` añade un modo al desplegable "Modo" de la sala.
- [Bonificaciones iniciales](bonificaciones.md): scripts en `data\BonusScripts` añaden opciones a la columna "Bonif." de la sala.
- [Opciones de partida propias](opciones.md): interruptores nuevos en las reglas de la sala, que los scripts leen con `Mod_GetOption`.
- [Tipos de mapa aleatorio](mapas-aleatorios.md): un dibujo de cuatro colores se convierte en un tipo nuevo de mapa (`crear-mapa-aleatorio.js`).
- [Imágenes propias](imagenes.md): botones de órdenes e iconos de unidades a partir de un PNG.
- [Gráficos propios](graficos.md): sprites propios para unidades y edificios (`convertir-sprite.js`, `crear-entidad.js`, `leer-rle.js`).
- [Validar un mod](validar.md): revisar un mod sin abrir el juego, con archivo y línea de cada fallo.
- [Publicar en GameBanana](publicar.md): categorías, etiquetas, subida y mods hechos antes del SDK.
- [Referencia](referencia.md): la lista completa de lo que un mod puede declarar (archivos, campos, parches, eventos, funciones y límites).

## Herramientas

Están en `herramientas\` y necesitan [Node.js](https://nodejs.org). Abre una terminal en esta carpeta y ejecútalas desde aquí (`node herramientas\validar-mod.js ...`). Cada una explica su uso al ejecutarla sin argumentos, y la guía del tema la trae con ejemplos. Sus mensajes salen en español; con `--lang en` o `--lang it` salen en inglés o en italiano. Cada herramienta acepta sus opciones en los tres idiomas (`--nombre`, `--name`, `--nome`); las guías usan las de su idioma.

| Quieres… | Usa |
|---|---|
| Una unidad nueva | `crear-unidad.js` ([guía](unidades.md)) |
| Un objeto nuevo | `crear-objeto.js` ([guía](objetos.md)) |
| Una habilidad nueva | `crear-habilidad.js` ([guía](habilidades.md)) |
| Una formación nueva | `crear-formacion.js` ([guía](formaciones.md)) |
| Una opción de partida | `crear-opcion.js` ([guía](opciones.md)) |
| Una civilización nueva | `clonar-civilizacion.js` ([guía](civilizaciones.md)) |
| Ciudad y aldea propias de una civilización | `crear-plantilla.js` ([guía](plantillas.md)) |
| Un fortín neutral | `crear-fortin.js` ([guía](fortines.md)) |
| Un tipo de mapa aleatorio | `crear-mapa-aleatorio.js` ([guía](mapas-aleatorios.md)) |
| Botones e iconos desde un PNG | `convertir-imagen.js` ([guía](imagenes.md)) |
| Sprites desde PNG, y la ficha de una unidad o edificio | `convertir-sprite.js`, `crear-entidad.js`, `leer-rle.js` ([guía](graficos.md)) |
| Revisar un mod sin abrir el juego | `validar-mod.js` ([guía](validar.md)) |
| Adaptar un mod hecho antes del SDK | `reducir-mod.js` ([guía](publicar.md)) |
| Sacar los datos del juego | `extraer-datos.js` (abajo) |

## Datos del juego

Varias herramientas piden la carpeta `DATA` del juego. La saca `extraer-datos.js` (necesitas [Node.js](https://nodejs.org)):

```
node herramientas/extraer-datos.js "<carpeta del juego>" <salida>
```

Deja `data.pak` en `<salida>\DATA`. Esa es la carpeta que piden `clonar-civilizacion.js`, `crear-fortin.js` y `validar-mod.js --datos`.

## ¿Qué cuenta como jugabilidad?

En multijugador, todos tienen que llevar los mismos mods si cambian lo que pasa en la partida. El juego lo calcula por las carpetas que toca tu mod: `data/`, `scenarios/`, `adventures/`, `conquests/`, `civs/`, `specials/`, `events/`, `options/`, `rules/`, `modes/`, `randommap/`, `randommapsettlements/`, los `.xml` y `.pass` de `units/`, `buildings/` y `mapobjects/`, los `.xml` de `visuals/`, los mapas `.bfhp` de `packs/` y los parches (según el archivo que parchean).

Los gráficos, los sonidos, los textos (`loc/`) y los idiomas no cuentan: cada jugador puede llevar los suyos.

## Si algo falla

Los errores de tu mod salen en su ficha del gestor y en `Logs\imperivm\imperivm-mod.log`. Cada línea del registro lleva la hora, un nivel (`I` información, `W` aviso, `E` error) y una etiqueta con la parte del mod de la que viene: `patch`, `language`, `civilizations`, `specials`, `script`, `event`. Mira primero las líneas `W` y `E` de las etiquetas de tu mod.

El registro guarda las cinco últimas sesiones (`imperivm-mod.1.log` a `imperivm-mod.4.log`) y, si el juego se cerró mal, `imperivm-mod.crashed.log`. Para más detalle de una parte, pon su etiqueta y un nivel en `[Debug]` de `imperivm-mod.ini`, por ejemplo `LogChannels=patch:debug`; `LogLevel=debug` lo hace con todo.

Para pedir ayuda, pulsa el corazón de abajo a la izquierda del menú principal y luego **Crear informe**. Crea un zip con los registros y tu sistema, sin tu usuario de Windows, IDs de Steam ni direcciones IP, y no envía nada: tú decides si lo compartes.

## Versión y licencia

La versión del SDK y lo que cambia en cada una están en [CAMBIOS](CAMBIOS.md). La licencia, en [LICENCIA](LICENCIA.md).
