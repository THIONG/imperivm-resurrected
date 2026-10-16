# Ejemplo: especialidad propia

Una especialidad de unidad nueva, Golpe crítico (bit 36), que se asigna al hastatus romano: uno de cada cuatro de sus golpes hace el triple de daño.

| Archivo | Qué es |
|---|---|
| `specials\critical.json` | La declaración: nombre, bit y descripción. |
| `patches\data\classes\RHastatus.sc.xml` | Se la asigna al hastatus con `unit_specials="Deflection, Critical"`. |
| `events\OnAttackResolved.vs` | El efecto: con cada golpe, si el atacante la tiene, el 25 % hace el triple de daño. |

Para probarlo: copia esta carpeta a `<juego>\mods\`, actívala en el gestor de contenido y reinicia. En `Logs\imperivm\imperivm-mod.log` sale `specials: registered Critical (bit 36…)` al empezar, y `script: Golpe crítico: daño N` cada vez que un hastatus hace uno.

Cambia la jugabilidad: todos los jugadores llevan el mod. Guía: [Especialidades de unidad](../../especialidades.md).
