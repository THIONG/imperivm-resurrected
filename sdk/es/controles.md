# Teclas de los mods

El jugador cambia las teclas del juego en **Opciones → Controles**. Las órdenes de tu mod salen allí solas, siempre que tengan nombre: no registras nada.

Ejemplo para copiar: [`ejemplos/ordenes-propias`](ejemplos/ordenes-propias).

## Cómo salen tus órdenes

Una orden es un `<cmd>` de un archivo de `data\commands`. Su tecla de fábrica es el atributo `key` (`key="j"`; con `ctrl="1"` hace falta Ctrl). El jugador puede cambiarla, y su cambio se guarda en `controls.ini`, no en tu mod.

La orden necesita `rollover`: es su nombre en la lista. Sin `rollover` no sale en Controles.

- Si la orden está en un archivo del juego que tu mod parchea (por ejemplo `RBarracks.xml`), sale en la pestaña de ese edificio o unidad, con las demás. Excepción: `Stonehenge.xml` no está en la tabla de archivos conocidos, así que una orden añadida ahí sale bajo «Edificios» con el nombre del archivo.
- Si tu mod trae un archivo de órdenes nuevo (por ejemplo `data\commands\MiCuartel.xml`), sale en la pestaña **Contenido**, en una sección con el nombre de tu mod. Con varios archivos de órdenes, cada uno tiene su sección.

La pestaña **Contenido** solo aparece si hay algún mod activo con órdenes nuevas.

## Ponerle nombre a la sección

Por defecto la sección se llama como tu mod (más el nombre del archivo si traes varios). Para dar nombres por idioma, usa `commands` en `mod.json`, con el nombre del archivo sin extensión:

```json
"commands": {
  "MiCuartel": {
    "name": {"spanish": "Cuartel romano extra", "english": "Extra Roman barracks", "italian": "Caserma romana extra"}
  }
}
```

Sale el nombre del idioma del juego; si no hay, el inglés; si no, el primero. También vale un texto solo (`"name": "Cuartel extra"`). En pantalla: `Nombre del mod · Cuartel romano extra`.

El nombre de cada orden es su `rollover`, y se traduce como cualquier otro texto: ver [Idiomas y textos](idiomas.md).

## Qué teclas se pueden usar

Una letra (A–Z) o un signo (punto y coma, igual, coma, guion, punto, barra, barra invertida, corchetes, comilla simple y la tilde grave), con Ctrl si quieres. Los números no valen: las órdenes solo admiten letras y signos, así que un `key="1"` no llega a ser una tecla en Controles. Tampoco valen Mayús ni Alt, solo Ctrl.

El jugador también puede usar el modo **Por su sitio en la barra** (Q el primer botón, W el segundo…). Tus órdenes entran ahí como las demás.

## Lo que no se puede

- Una tecla que haga algo que no sea una orden del juego (cambiar una opción del mod, lanzar un script suelto) no existe. Toda acción con tecla tiene que ser una orden.
- Un archivo de órdenes fuera de `data\commands` no se carga.
