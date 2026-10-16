# Ejemplo: habilidad de tropa

Una habilidad nueva, Respiro, para todos los soldados cuerpo a cuerpo (las clases que cuelgan de `Melee`): tecla `Y`, gasta 5 de energía (de los
10 que tiene un soldado) y cura 60 de vida.

| Archivo | Qué es |
|---|---|
| `data\commands\Respiro.xml` | La orden: el botón y su tecla. |
| `patches\data\classes\Melee.sc.xml` | Añade el método `respiro` a la clase `Melee` con un parche, sin sustituir su archivo. |
| `data\mod\respiro.vs` | El script que la ejecuta. |
| `loc\spanish.loc.xml` | El nombre y la descripción de la orden, con `respiro` como contexto. |
| `arte\respiro.png` | El dibujo del botón (51 x 51). |
| `gameres\cmdbar\Actions\Respiro.bmp` | El botón en formato del juego, hecho con `herramientas/convertir-imagen.js` desde el PNG. |

Para probarlo: copia esta carpeta a `<juego>\mods\`, actívala en el gestor de contenido y reinicia. Selecciona un legionario o un guerrero y pulsa `Y`
(o su botón). Cada uso escribe `Respiro: un soldado se ha curado` en `Logs\imperivm\imperivm-mod.log`.

Cambia la jugabilidad: todos los jugadores llevan el mod. Guía: [Habilidades de tropas](../../habilidades.md).
