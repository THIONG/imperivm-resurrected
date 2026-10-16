# Ejemplo: unidad nueva

El Veterano romano: una unidad nueva que se recluta en el cuartel de Roma republicana (tecla `V`). Es más fuerte que el hastatus (260 de vida,
ataque 20, armadura 14) y usa su modelo. Tiene retrato y botón propios.

| Archivo | Qué es |
|---|---|
| `data\classes\RVeteran.sc.xml` | La clase: copia entera de la del hastatus con otras estadísticas. |
| `data\commands\Veterano.xml` | La orden de reclutarlo en el cuartel (180 de oro). |
| `gameres\icons\RVeteran.bmp` | El retrato (65 x 76), hecho con `herramientas/convertir-imagen.js` desde `arte\veterano.png`. |
| `gameres\cmdbar\Actions\train RVeteran.bmp` | El botón (153 x 51), desde `arte\boton.png`. |
| `loc\spanish.loc.xml` | El nombre de la unidad y el texto del botón, en español. |

Para probarlo: copia esta carpeta a `<juego>\mods\`, actívala en el gestor de contenido y reinicia. Juega con Roma republicana y busca el botón
nuevo (tecla `V`) en el cuartel. Cambia la jugabilidad: todos los jugadores llevan el mod.

Para una unidad con sprites propios, mira [Gráficos propios](../../graficos.md). Guía de esta: [Unidades nuevas](../../unidades.md).
