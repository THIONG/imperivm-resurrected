# Ejemplo: modo de victoria

Añade un modo nuevo, **Dominio**: gana quien controla un número de asentamientos. Ese número sale del desplegable "Límite" de la sala (2, 3, 4 o 5); si el script no recibe un valor válido, usa 3. La eliminación sigue valiendo: quien se queda sin ayuntamientos y sin tropas pierde.

- `data\GameScripts\6 Dominio.vs`: el script. El juego lanza una copia por jugador (`//void, int player, str param`). El nombre del archivo, con su número, es el id del modo: no repitas el de otro mod ni uno del juego (1 a 5).
- `patches\data\const.ini`: rellena el desplegable "Límite" de este modo. Cada línea, como `6_Dominio0 = 2 asentamientos, 2`, lleva la clave (nombre del archivo con `_` en vez de espacios, más un número desde 0), el texto que se ve y, tras la coma, el valor que recibe el script como `param`.
- El script termina la partida de un jugador con `EndGame(player, false)` (gana) o `EndGame(player, true)` (pierde).

Los textos del script están en inglés y el ejemplo no trae traducciones, así que en el juego en español saldrán en inglés. Se traducen con un `loc`: ver [Idiomas](../../idiomas.md).

Para probarlo, copia esta carpeta a `<juego>\mods\`, actívala en el gestor de contenido, reinicia y mira el desplegable "Modo" en una escaramuza. Guía: [Modos de victoria](../../victoria.md).
