# Validar un mod

El validador revisa tu mod sin abrir el juego y te dice los fallos con archivo y línea. Hace falta [Node.js](https://nodejs.org).

```
node herramientas/validar-mod.js <carpeta del mod>
node herramientas/validar-mod.js <carpeta del mod> --datos <carpeta DATA>
```

`--datos` es la carpeta `DATA` del `data.pak` extraído (la saca `herramientas/extraer-datos.js`, ver el [README](README.md#datos-del-juego)). Con ella también comprueba que los archivos que parchea tu mod existen y que los scripts a los que apuntan tus clases existen. Solo mira las rutas que empiezan por `data/`: si parcheas `units/…`, `buildings/…` o `currentlang/…`, no lo comprueba. Sin `--datos` se revisa todo lo demás.

Con `--juego <carpeta del juego>` (solo lee) mira los nombres de `Units.pak`, `Buildings.pak`, `MapObjects.pak` y `Visuals.pak` para saber si existen las imágenes y entidades que tu mod toma del juego, y avisa si tu `.rle` se llama como uno del juego:

```
node herramientas/validar-mod.js <carpeta del mod> --datos <DATA> --juego "C:\Program Files (x86)\Steam\steamapps\common\Imperivm - Great Battles of Rome HD"
```

Sale con código 1 si hay errores, con 0 si solo hay avisos o nada, y con 2 si lo llamas mal. Sirve para automatizar.

Un error es algo que el juego rechazaría o que va a fallar. Un aviso es una sospecha: algo que el juego ignora o que probablemente no querías.

## Qué revisa

- **`mod.json`**: JSON roto, categoría o etiquetas que no son de GameBanana, textos mal tipados (errores). Falta de `id`, `name`, `version` o `category` (avisos).
- **Archivos**: extensiones de ejecutables y scripts del sistema (error: el mod no se cargaría) y rutas con `..`. Una extensión rara pero inofensiva (`.psd`, `.gif`, `.zip`…) solo da un aviso: el mod se carga, el archivo solo ocupa sitio.
- **`civs\*.json`**: `id` que no vale, `index` fuera de 10–99 o repetido, `base` que no existe, `letter` de más de una letra, `biome` desconocido, `template` mal escrita o sin su archivo. Con `"ai": true` sin tabla `alias` avisa, y mira que el `alias` apunte a clases y órdenes que el mod tenga.
- **`specials\*.json`**: `id` que no vale o que es una especialidad del juego, `bit` fuera de 36–63 o repetido.
- **`options\*.json`**: el reparto de bits (entre todos los mods caben 8).
- **Parches XML**: XML mal formado (el parche no se aplicaría entero), operación sin `select`, selector mal escrito, etiquetas que no son operaciones. Un selector que no empieza por `/` o `//` da error aunque el DLL lo aceptaría.
- **Parches de scripts (`.vs`)**: directiva que no existe, `@before`, `@after` o `@replace` sin texto, directiva sin código debajo, y `@top` con código que no sea declaración de variables. No revisa el código del parche, ni las funciones `Mod_*`, ni la firma.
- **Parches INI**: líneas que no se entienden, y una sección nueva en `UNIT_SPECIALS.INI` (el juego puede cerrarse). No avisa de comentarios al final de una línea de parche, que el motor no soporta (ver [Parches](parches.md)).
- **Clases (`data\classes`)**: XML mal formado, `race` desconocida, `unit_specials` mal escritas o desconocidas (el juego distingue mayúsculas: `Parry`, no `parry`), y una clase sin `entity` (aviso: el juego no la hereda del padre y la unidad desaparece).
- **Codificación**: un XML de clase en Latin-1 con tildes. El juego no carga la clase y no avisa; guárdalo como UTF-8.
- **Órdenes (`data\commands`)**: XML mal formado y órdenes repetidas en un archivo.
- **Entidades (`.ent.xml`)**: error si la imagen no existe (el juego se cuelga al cargar la clase; con la carpeta del mod si trae sus `.rle`, y siempre con `--juego`), `idx` repetido o fuera de 1–32, capas y animaciones que apuntan a algo que no hay, límites del motor (32 imágenes, 32 capas, 8 estados, animaciones 1–28, 64 fotogramas), y `columns`, `rows` o número de `<frame>` que no cuadran. Una animación que lee más filas de las que tiene su imagen es solo un aviso.
- **Gráficos (`.rle`, `.pass`)**: `.rle` cortado o corrupto, `.rle.mmp` renombrado (sin píxeles, no carga desde un mod), paleta vacía, `.pass` mal formado o vacío, `pass_file` que no existe, y un `.rle` con el mismo nombre que uno del juego (gana el del juego; con `--juego`).
- **`placeobjects.xml`** (mapas aleatorios): una clase nombrada que no existe (con `--datos`). La generación del mapa se queda colgada sin mensaje.
- **Textos (`loc`)**: XML mal formado, entradas sin `text` o sin `result`.
- **Scripts (`.vs`)**: falta la firma en la primera línea, evento desconocido o con la firma equivocada, `Sleep` en un evento que no puede esperar, funciones `Mod_*` que no existen, `Mod_HasSpecial` con un `Obj` en vez de una `Unit`.

## Ejemplo de salida

```
AVISO  specials/c.json  sin "name": se verá el id
ERROR  specials/d.json  el bit 40 ya lo usa specials/c.json
AVISO  data/classes/G.sc.xml:2  especialidad "Zzz" desconocida: el juego la ignora

5 archivos revisados: 1 errores, 5 avisos.
```

## Lo que no hace

No aplica los parches ni ejecuta los scripts: eso solo se ve en el juego (usa `Dump=` de [Parches](parches.md) para ver el resultado, y `Logs\imperivm\imperivm-mod.log` para los errores en marcha). Tampoco juzga la calidad de las imágenes.

Está pensado para mods nuevos, hechos con este SDK. Un mod antiguo que funciona en el juego, como el Community Mod, da errores (scripts sin firma, scripts de clase que no existen, filas de animaciones) sin que eso quiera decir que no cargue.
