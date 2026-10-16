# Ejemplo: tipo de mapa aleatorio

Un tipo de mapa aleatorio nuevo, **Anillo**: un anillo de tierra con un mar en el centro y montañas entre medias.

Esta carpeta solo trae el dibujo, `dibujos\anillo.png` (negro mar, blanco tierra, magenta montañas; los puntos de sitios los reparte la herramienta). Copiada tal cual a `mods\` no añade ningún mapa: antes hay que generar los archivos que lee el juego. Desde la raíz del repo:

```
node herramientas\crear-mapa-aleatorio.js ejemplos\mapa-aleatorio-ejemplo Anillo ejemplos\mapa-aleatorio-ejemplo\dibujos\anillo.png --jugadores 8
```

Eso crea dentro del mod `data\randommap\Anillo.rm.xml` y `randommap\Anillo\010.bmp … 120.bmp`. Después copia la carpeta a `<juego>\mods\`, actívala y reinicia. En una escaramuza, "Tipo de mapa" ofrece "Aleatorio: Anillo".

Guía: [Mapas aleatorios](../../mapas-aleatorios.md).
