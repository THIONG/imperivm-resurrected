# Gráficos propios para unidades y edificios

El SDK no dibuja nada: usa Photoshop, Paint.NET, Aseprite, Blender… El juego solo lee su formato `.rle`, que ningún programa exporta, así que el SDK trae un conversor.

Un mod puede traer sus sprites como archivos sueltos, con las mismas rutas que usa el juego. Una unidad o un edificio nuevo cambia la jugabilidad: todos los jugadores de la partida tienen que llevar el mod.

Ejemplo: [`ejemplos/unidad-graficos-ejemplo`](ejemplos/unidad-graficos-ejemplo) (un soldado de cubitos, 8 direcciones, color de jugador). Lo dibuja `ejemplos\unidad-graficos-generador\dibujar-soldado.js`, que es lo único que sustituirías por tus dibujos.

Todos los comandos de esta guía se lanzan desde la raíz del repo. Las herramientas son Node.js y no necesitan nada más.

## Qué se puede hacer

- Una unidad nueva con dibujos propios: sí, en una carpeta nueva `units\MiUnidad\`.
- Cambiar el dibujo de una unidad que ya existe: no con un archivo suelto, porque el juego prefiere el suyo. Crea una unidad nueva.
- Un edificio nuevo con dibujos propios: sí, con más trabajo (hojas grandes, fases de construcción, huella `.pass` y puntos). Ver Edificios.
- Iconos y botones de interfaz: ya resuelto en [Imágenes propias](imagenes.md).
- Sonidos nuevos: `.wav` y `.ogg`.

## Las herramientas

- `herramientas\convertir-sprite.js`: tus PNG a `.rle`. Recorta, reduce a 256 colores, marca el color de jugador y hace la sombra.
- `herramientas\crear-entidad.js`: escribe el `.ent.xml` (imágenes, capas, estados, animaciones) de una unidad o un edificio, y la huella `.pass` de un edificio.
- `herramientas\leer-rle.js`: lo contrario, de `.rle` a PNG. Sirve para mirar un sprite del juego y para partir de él.
- `herramientas\validar-mod.js`: avisa de imágenes que faltan, `idx` repetidos, filas y columnas que no cuadran ([Validar](validar.md)).

## Una unidad paso a paso

### 1. Dibuja y guarda los PNG

Un PNG por animación, dirección y fotograma, todos del mismo tamaño (el lienzo, 150 x 150 para una unidad) y con fondo transparente:

```
Idle_d0_f00.png  Idle_d0_f01.png  ...  Idle_d7_f07.png      Nombre_d<dirección>_f<fotograma>.png, contando desde 0
Walk_d0_f00.png  ...
Fight_..., Death_..., Transition_..., Taunt_...
```

- **Animaciones.** Obligatorias: `Idle` (quieto), `Walk` (andar), `Fight` (atacar) y `Death` (morir). Opcionales: `Transition` (de reposo a combate) y `Taunt` (provocar). Si faltan, se usa `Idle` en su lugar.
- **Direcciones.** 8 para soldados, 12 para héroes y animales, 4 para barcos. Todas las animaciones de una unidad llevan las mismas.
- **Orden de las direcciones.** En las hojas del juego, la columna 0 mira a la cámara (abajo en pantalla) y siguen en sentido horario: sur, oeste, norte, este con las diagonales en medio. Con un sprite tuyo, mira primero una hoja del juego con `leer-rle.js` antes de dibujar las ocho.
- **Lienzo y pies.** Mismo lienzo en todos los fotogramas y los pies en el mismo punto. En las unidades del juego es el píxel (75, 100) de 150 x 150. Se cambia con `--lienzo` y `--pies`.
- **Tamaño.** El motor no impone ninguno: cada fotograma se recorta a lo que dibujes. Como referencia, un soldado del juego mide unos 40 x 52 píxeles y un héroe unos 40 x 88 (medidas aproximadas).
- **Fotogramas.** Hasta 62 por animación. En el hastatus: andar 15, atacar 14, morir 20, reposo 8.
- **Transparencia.** Todo o nada: un alfa menor que 128 es transparente. El juego no tiene semitransparencia.
- **Colores.** Hasta 256 por conjunto; con color de jugador, 192 (los otros 64 son el tinte). Si usas más, el conversor los reduce y avisa.
- **Color de jugador.** Pinta de magenta lo que debe cambiar con el equipo. El conversor toma como tinte los magentas (rojo y azul parecidos, casi sin verde). `(255, 0, 255)` es el color del jugador casi puro; `(128, 0, 128)`, `(64, 0, 64)`… son sus sombras (7 niveles útiles). Evita violetas y rosas en el resto de la figura: pueden caer como tinte sin querer. No suavices los bordes: un borde semitransparente mezclado queda rosa, no tinte.
- **Sombra.** Opcional: un PNG por fotograma con el mismo nombre más `_sombra` (`Walk_d3_f05_sombra.png`), negro o gris oscuro opaco sobre transparente. Si no lo das, el conversor dibuja una elipse bajo los pies. El juego oscurece a la mitad el suelo bajo la sombra.

También puedes dar una hoja ya montada por animación: `Walk.png` con las direcciones en columnas y los fotogramas en filas, cada celda del tamaño del lienzo (`--lienzo AnchoxAlto`). La sombra va en `Walk_sombra.png` con la misma disposición.

### Partir de un sprite del juego

Los sprites del juego están en los `.rle.mmp` de los `.pak`, y sus píxeles en `rle.mmp`, en la carpeta del juego. Primero sácalos con `extraer-datos.js`:

```
node herramientas\extraer-datos.js "<carpeta del juego>" <salida> Units.pak HASTATUS
```

Y conviértelos a PNG con `--convenio`, que escribe justo lo que espera `convertir-sprite.js` (un PNG por dirección y fotograma, a lienzo completo, el tinte de jugador en magenta y la sombra en `_sombra`):

```
node herramientas\leer-rle.js <salida>\UNITS\RHASTATUS\WALK.RLE.MMP --rle-mmp "<carpeta del juego>\rle.mmp" --convenio --salida <carpeta de PNG>
node herramientas\convertir-sprite.js <carpeta de PNG> --jugador --salida <mod>\units\MiUnidad
```

Sin `--convenio`, `leer-rle.js` escribe una hoja con celdas del tamaño de cada recorte, y `convertir-sprite.js` no la acepta. `--lienzo AnchoxAlto` cambia el lienzo (150 x 150 por defecto). La sombra del juego es otro archivo (`WALK_SHADOW.RLE.MMP`): si no la sacas, el conversor dibuja la elipse. Son tus archivos del juego para estudiar o partir de ellos: no los redistribuyas.

### 2. Convierte a `.rle`

```
node herramientas\convertir-sprite.js <carpeta con los PNG> --salida <mod>\units\MiUnidad --jugador
```

- `--jugador`: usa color de jugador (el magenta pasa a ser el tinte del equipo). Sin él, la paleta es libre de 256 colores, sin color de jugador; es la más sencilla y la que usan lobos y ciervos.
- `--sombra auto|archivos|ninguna`: por defecto `auto` (tus `_sombra` si los hay, si no una elipse).
- `--paleta-por-animacion`: una paleta por `.rle`. Por defecto hay una para toda la unidad, con la pérdida de calidad de reducir a 192 o 256 colores si usas más. El juego usa una por animación.
- `--lienzo AnchoxAlto` (por defecto el de los PNG), `--pies X,Y` (solo para la sombra automática), `--umbral-alfa N`.

Escribe `Idle.rle`, `Idle_shadow.rle`, `Walk.rle`…

### 3. Crea la entidad (`.ent.xml`)

```
node herramientas\crear-entidad.js unidad <mod>\units\MiUnidad --nombre MiUnidad
```

Lee los `.rle` de la carpeta y escribe `MiUnidad.ent.xml`: 2 capas (unidad `z=1000`, sombra `z=800`), 2 estados (reposo y combate) y las 7 animaciones del motor (andar 1, atacar 5, morir 9, `Idle` 13, `ToIdle` 18, `ToAttack` 19, `Taunt` 20) con los tiempos del hastatus (andar 55 ms por fotograma, atacar 71, morir 80 con la última parte lenta, reposo 150…). Son 6 hojas de animación y 7 animaciones del motor, porque la hoja `Transition` sirve para `ToAttack` y, al revés, para `ToIdle`.

Límites del motor: ver *Referencia del formato*. Para cambiar el ritmo, `--config animaciones.json`:

```json
{ "Walk": { "duracion": 55, "paso": 42 }, "Fight": { "duracion": 71, "golpe": 426 } }
```

`duracion` son milisegundos por fotograma. `paso` (`step`) y `golpe` (`action_time`, los milisegundos hasta que sale el golpe) forman parte de la simulación y afectan a la jugabilidad.

### 4. La clase y el resto

La clase (`data\classes\MiUnidad.sc.xml`) es una copia entera de una del juego con `entity="Units/MiUnidad/MiUnidad.ent.xml"`. La orden de reclutar, el botón y los textos están en [Unidades nuevas](unidades.md). No hagas la clase hija de otra: el juego no hereda `entity` del padre y la unidad desaparece. El nombre de la carpeta tiene que ser nuevo: si repites el de una unidad del juego, el juego usa su `.rle` y el tuyo se ignora.

### 5. Valida y prueba

```
node herramientas\validar-mod.js <mod> --datos <DATA> --juego "<carpeta del juego>"
```

Una imagen que no existe tumba el juego al cargar la clase (un `idx` repetido o un `.rle` corrupto, lo mismo), y el validador lo detecta antes. Luego copia el mod a `<juego>\mods\`, actívalo y reinicia. Si el juego se cierra, `Logs\imperivm\imperivm-mod.log` apunta la imagen que falló.

## Edificios

Misma forma que una unidad, con estas diferencias:

- **Una dirección** (`_d0`) y varias filas, que son las fases de construcción (los del juego usan 4). Una hoja por capa del edificio, `layer1`, `layer2`… (hasta 5 en el juego), más `shadow`. PNG: `layer1_d0_f00.png` (capa 1, fase 0), `layer1_d0_f01.png`… y `shadow_d0_f00.png`… Una animación llamada `shadow` se toma directamente como máscara de sombra (cualquier píxel opaco cuenta).
- **Lienzos grandes**, de unos 700 x 700 en el juego. El punto donde se clava el edificio (`--pies`) lo eliges tú; en el cuartel romano es (305, 330).

```
node herramientas\convertir-sprite.js <PNG> --salida <mod>\buildings\MiEdificio --jugador --sombra ninguna
node herramientas\crear-entidad.js edificio <mod>\buildings\MiEdificio --nombre MiEdificio --pies 305,330 --huella 8x8 --puntos-de <otro .ent.xml>
```

- **La huella (`.pass`)** es la rejilla de 1 bit que dice por dónde no se puede pasar. Cada celda son 16 unidades de mapa (algo menos de medio soldado de ancho). `--huella AnchoxAlto` hace un rectángulo centrado en el punto de anclaje. `--huella-png máscara.png` usa un dibujo (1 píxel = 1 celda, norte arriba; opaco y oscuro = bloqueado, transparente o blanco = libre). Mantenla en unos 120 x 120 celdas como mucho. Una casa del juego mide unas 15 celdas por lado y el cuartel romano unas 29.
- **Los puntos (`<point>`)** son las puertas, los sitios de los centinelas, el humo… El conversor no los puede inventar. Con `--puntos-de <.ent.xml de un edificio parecido del juego>` copia los de ese edificio; sus coordenadas son relativas al punto de anclaje, así que ajústalas si el tuyo tiene otra forma.
- La clase del edificio (`CVXBuilding`, `parent="Building"`, órdenes, costes…) se hace como en [Unidades nuevas](unidades.md): copia entera de una del juego.
- Sin hacer: versiones de invierno (`entity_winter`) y animaciones de edificios (humo, fuego). Se pueden añadir a mano.
- El mensaje final de `convertir-sprite.js` sugiere `crear-entidad.js unidad` aunque conviertas un edificio: usa `edificio`.

## Referencia del formato

Si quieres leer o escribir los formatos por tu cuenta, el código está en `herramientas\lib\rle.js` (`.rle`) y `herramientas\lib\pass.js` (`.pass`). Las herramientas escriben `.rle` de 256 colores (con o sin color de jugador) y sombra; no hay color verdadero.

Límites de `.ent.xml`:

- `<image idx file rows columns drawmode>`: 32 imágenes; `idx` de 1 a 32 sin repetir; `file` sin extensión o con `.rle`.
- `<layer idx name image z …>`: 32. Unidad `z=1000`, sombra `z=800`.
- `<state idx …>`: 8.
- `<anim idx name frames duration action_time step>`: `idx` de 1 a 28; `frames` = filas + 2 (dos fotogramas centinela de duración 0); hasta 64 fotogramas.
- `variations` de `<entity>`: igual a las columnas de todas las imágenes.

## Sonidos

La clase nombra sus sonidos con `<sounds attack="…" die="…"/>`. Cada nombre es un archivo `data\sound entities\<Nombre>.xml` que lista los `.wav` o `.ogg`; puedes poner los tuyos en el mod. Si no quieres sonidos nuevos, copia los de una unidad parecida (`attack="UnitSwordFight" die="UnitDeath"`).
