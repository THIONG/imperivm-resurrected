# Tipos de mapa aleatorio propios

Un mod puede añadir un tipo de mapa aleatorio (como Costero, Islas o Pasos de montaña) al desplegable "Tipo de mapa" de la sala. El tipo es una forma dibujada: tierra, mar, montañas y los sitios donde empieza cada jugador. El juego la deforma, la gira y la rellena de terreno, árboles y objetos con sus reglas de siempre.

Ejemplo: [`ejemplos/mapa-aleatorio-ejemplo`](ejemplos/mapa-aleatorio-ejemplo) (un anillo de tierra con un mar en el centro y montañas entre medias). El repo solo trae el dibujo: los archivos que lee el juego se generan con el comando del paso 2.

## Cómo se hace

1. **Dibuja la forma** en un PNG cuadrado, con el programa que quieras y cuatro colores:

   | Color | Significa |
   |---|---|
   | negro `(0, 0, 0)` | mar |
   | blanco `(255, 255, 255)` | tierra |
   | magenta `(255, 0, 255)` | montañas: líneas finas, de unos 3 píxeles (opcional) |
   | rojo `(255, 0, 0)` | puntos donde el juego puede poner sitios de partida y ciudades neutrales (opcional) |

   Un color que no sea exacto pasa al más parecido. Lo transparente cuenta como mar. Si el PNG no es cuadrado, se estira a 512 x 512 sin avisar. El juego lo estira luego al tamaño del mapa, así que usa trazos gruesos: los detalles finos se pierden.

   Los dibujos del juego traen cientos de puntos rojos sueltos por la tierra y el juego elige entre ellos. Lo normal es no pintarlos y dejar que la herramienta los reparta. Pinta rojo solo si quieres que los sitios salgan en ciertas zonas.

2. **Conviértelo en un tipo de mapa**, desde la raíz del repo:

   ```
   node herramientas\crear-mapa-aleatorio.js <carpeta del mod> Anillo dibujo.png --jugadores 8
   ```

   Para el ejemplo:

   ```
   node herramientas\crear-mapa-aleatorio.js ejemplos\mapa-aleatorio-ejemplo Anillo ejemplos\mapa-aleatorio-ejemplo\dibujos\anillo.png --jugadores 8
   ```

   La herramienta escribe en tu mod `data\randommap\Anillo.rm.xml` y `randommap\Anillo\010.bmp … 120.bmp`. El nombre (`Anillo`) lleva solo letras sin tilde, números y espacios, empieza por letra y tiene de 2 a 31 caracteres. No uses el de un tipo del juego (`Coastal`…): la herramienta no avisa y pisarías el suyo.

   | Opción | Para qué |
   |---|---|
   | `--jugadores N` | Máximo de jugadores en mapa grande (2 a 8). En el mediano, hasta 6 |
   | `--pequeno N` | Máximo en mapa pequeño (0 = no sale en pequeño; 2 por defecto) |
   | `--distorsion X` | Cuánto se deforma la forma al generarla, de 0 a 0.5 (0.1 casi nada, 0.25 mucho; 0.2 por defecto) |
   | `--sin-girar` | Que no se gire ni se refleje al azar |

   Puedes pasar varios PNG (`dibujo1.png dibujo2.png …`) y el juego elige uno al azar en cada partida. La herramienta rellena hasta 12 repitiendo los tuyos. Si 12 no es múltiplo del número de PNG, el reparto sale desigual (con 5, tres PNG salen 3 veces y dos salen 2). Con uno solo, siempre sale esa forma.

3. Activa el mod y reinicia. La sala ofrece "Aleatorio: Anillo".

## Cosas a tener en cuenta

- Cambia la jugabilidad: todos los jugadores deben llevar el mismo mod.
- "Buscar partida" usa una lista fija de cinco tipos del juego: un tipo nuevo solo sale en las partidas que crees tú.
- No se pueden crear biomas nuevos: el juego distingue cuatro zonas fijas. Sí puedes cambiar lo que pone cada una con [parches](parches.md) sobre `data\randommap\terrainmix.xml`, `placetrees.xml` y `placeobjects.xml`.
- Prueba con el número de jugadores que vayas a permitir.
