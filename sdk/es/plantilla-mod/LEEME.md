# Plantilla de mod

Una carpeta mínima para empezar. Cópiala a `<juego>\mods\`, cámbiale el nombre y rellena `mod.json`.

1. Cambia `id` (letras minúsculas, números y guiones: es lo que te identifica), `name`, `authors` y `description`.
2. Elige `category` de la lista de [Publicar](../publicar.md). `tags` solo hace falta en un mapa (jugadores) o en una campaña, traducción o audio (idioma).
3. Añade las carpetas que necesites. Con las mismas rutas que usa el juego:

```
data\classes\       clases nuevas o copias de las del juego
data\commands\      órdenes (botones)
patches\data\...    parches sobre archivos del juego
loc\                textos
events\             scripts de eventos
civs\               civilizaciones nuevas
options\            opciones de partida
```

Qué va en cada carpeta, y el detalle de cada cosa, está en las guías ([índice](../README.md)); la lista completa de campos, en la [referencia](../referencia.md).

Antes de abrir el juego: `node herramientas\validar-mod.js <tu carpeta> --datos <carpeta DATA>`. La carpeta DATA se saca con `extraer-datos.js` (ver [Validar un mod](../validar.md)).
