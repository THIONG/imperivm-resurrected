# Ejemplo: scripts

Muestra los eventos y los comportamientos de la guía [Scripts](../../scripts.md). Ojo: cambia el equilibrio. Los hastatus de todos los jugadores hacen el doble de daño y se curan 25 al matar a un soldado. No lo actives en una partida en serio.

Qué hay:

- `data\mod\hello.vs`: comportamiento de la clase `RHastatus`. Escribe líneas en el registro con `Mod_Log`, `Mod_LogInt`, `Mod_IsActive` y `Mod_ApiVersion`.
- `patches\data\classes\Military.sc.xml` y `data\mod\soldier.vs`: un comportamiento en la clase base de los soldados, que heredan todos. Apunta los diez primeros soldados que entran en el mapa. Es la forma de reaccionar a que aparezca un objeto.
- `events\OnGameStart.vs`: una línea al empezar la partida.
- `events\OnObjectDie.vs`: una línea cada vez que muere un soldado.
- `events\OnObjectKill.vs`: una línea por muerte, y los hastatus se curan 25 al matar a un soldado.
- `events\OnAttackResolved.vs`: duplica el daño de los hastatus con `Mod_SetAttackDamage`. Apunta los tres primeros golpes.
- `events\OnPlayerDefeated.vs`: una línea cuando un jugador es derrotado.
- `events\OnSettlementCaptured.vs`: una línea cuando un asentamiento cambia de dueño, y cuenta las capturas con una variable de mod.

Para probarlo, copia esta carpeta a `<juego>\mods\`, actívala en el gestor de contenido y reinicia. Empieza una partida, entrena un hastatus con Roma republicana, lucha con él y mira `Logs\imperivm\imperivm-mod.log`: las líneas con la etiqueta `script` son de este mod.
