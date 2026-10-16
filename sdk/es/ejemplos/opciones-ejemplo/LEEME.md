# Ejemplo: opciones de partida propias

Añade dos opciones a las REGLAS de la sala: un interruptor, **Héroes veloces** (+50 %), y una lista, **Velocidad de héroes** (Normal, Rápida +25 %, Muy rápida +50 %).

- `options\heroes-veloces.json` y `options\velocidad-heroes.json`: las opciones, con nombre y ayuda por idioma y su valor por defecto. El `id` es el que usan los scripts.
- `data\mod\heroes_veloces.vs` y `patches\data\classes\Hero.sc.xml`: un comportamiento de la clase `Hero` que lee `Mod_GetOption("heroes-veloces")` y `Mod_GetOptionValue("velocidad-heroes")` y acelera al héroe.

Aparecen en la sala para todos los jugadores, pero solo las cambia el anfitrión. Viajan con las demás reglas y se guardan con la partida. Entre todos los mods activos hay 8 bits para opciones: un interruptor gasta 1 y una lista, de 1 a 4. Fuera de la sala (aventuras, campañas) valen siempre 0.

Para probarlo, copia esta carpeta a `<juego>\mods\`, actívala en el gestor de contenido, reinicia, crea una escaramuza y marca "Héroes veloces" en REGLAS. Con `Mod_LogInt` el script deja una línea por héroe en `Logs\imperivm\imperivm-mod.log`. Guía: [Opciones de partida](../../opciones.md).
