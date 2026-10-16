# Imágenes propias

Un mod puede traer sus propios botones de órdenes y retratos de unidades en vez de reutilizar los del juego. Dibujas en el programa que quieras, guardas un PNG y lo conviertes con una herramienta. Hace falta [Node.js](https://nodejs.org).

Ejemplo para copiar: [`ejemplos/habilidades-ejemplo`](ejemplos/habilidades-ejemplo) (el botón de «Respiro»).

## Convertir un PNG

```
node herramientas/convertir-imagen.js <entrada.png> <salida.bmp> --tipo boton
node herramientas/convertir-imagen.js <entrada.png> <salida.bmp> --tipo icono
```

- `boton`: el botón de una orden (`button="..."` de un `<cmd>`). En el juego mide 153 x 51: tres cuadros de 51 x 51 (normal, con el ratón encima y desactivado). Tú das un PNG de 51 x 51 y la herramienta hace los otros dos cuadros (uno más claro y otro gris). Si ya tienes los tres, un PNG de 153 x 51 se usa tal cual. Otro tamaño se escala sin deformarlo.
- `icon`: el retrato de una unidad (`icon="..."`) o el de su cola de entrenamiento (`queueicon="..."`). Mide 65 x 76. Vale un PNG de cualquier tamaño: se escala sin deformarlo, y lo transparente pasa al verde puro que el juego toma como transparente.

Con `--fondo #RRGGBB` eliges el color con el que se rellena lo transparente de un botón (por defecto, un marrón oscuro como el de los botones del juego).

El PNG tiene que ser de 8 bits por canal y sin entrelazar, que es lo normal al guardar desde cualquier programa. Si no, la herramienta lo dice.

## Dónde va el archivo

Con la misma ruta que usa el juego, dentro de la carpeta del mod. La regla para los botones es `gameres/cmdbar/<lo que ponga button=>`:

| Qué | En el XML | Archivo en tu mod |
|---|---|---|
| Botón de una orden | `button="Actions/Respiro.bmp"` | `gameres/cmdbar/Actions/Respiro.bmp` |
| Botón de una mejora | `button="research/Mejora.bmp"` | `gameres/cmdbar/research/Mejora.bmp` |
| Retrato de una unidad | `<properties icon="gameres/icons/MiUnidad.bmp"/>` | `gameres/icons/MiUnidad.bmp` |
| Retrato en la cola | `queueicon="gameres/icons/MiUnidad.bmp"` | `gameres/icons/MiUnidad.bmp` |

Las órdenes del juego también usan las carpetas `upgrades`, `skills`, `states` y `specials` dentro de `cmdbar`. Funcionan igual.

Las mayúsculas no cuentan en las rutas. Si el juego no encuentra una imagen, el botón sale como un cuadro azul liso.

## Si no sale

1. Pon `LogFiles=1` en `[Debug]` de `imperivm-mod.ini`. El log anota cada archivo que abre el juego y marca los que no existen (`VFS: opens … -> does not exist`), así ves el nombre exacto que pide.

## Lo que no se puede

- Las imágenes de ayuda (`help/...`), las de la barra de información de cada civilización y los fondos de menú no tienen conversor.
- Los modelos y animaciones de unidades y edificios (`.rle` y `.ent.xml`) son otra cosa: ver [Gráficos propios](graficos.md).
