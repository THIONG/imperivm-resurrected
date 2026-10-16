# Ejemplo: táctica de IA propia

Una civilización de mod con `"ai": true` juega por defecto como su base: los guiones de IA del juego ven la civilización base, así que Dacia usa
lo mismo que Galia. Este ejemplo hace que la IA de Dacia elija otra táctica.

`GetTacticScript.vs` decide al empezar qué táctica usa cada IA según su civilización. Para Galia devuelve `TS_GaulTactic`, que no compila, así que la IA gala no sigue ninguna táctica. El juego trae otra, `TS_GaulTactic2` (orden de construcción con héroe, cuartel,
arqueros y druidas), que ninguna IA usa. Dacia la usa aquí.

| Archivo | Qué es |
|---|---|
| `patches\data\ai\GetTacticScript.vs` | Un [parche de script](../../parches.md): añade un trozo al `GetTacticScript.vs` del juego, justo antes de su primera línea de código (las variables se declaran antes). Si la civilización real del edificio central es Dacia (`Mod_TrueRace`), devuelve `TS_GaulTactic2`. El resto del script no se toca. |

`Mod_TrueRace(objeto)` da la civilización de verdad de un objeto. `objeto.race` no sirve aquí: con `"ai": true` la IA ve la base (Galia).

Necesita el mod «Ejemplo: civilización» (Dacia). Para probarlo: copia esta carpeta a `<juego>\mods\`, actívala junto a `civilizacion-ejemplo` en el
gestor de contenido, reinicia y juega con una IA de Dacia (o una IA aleatoria: Dacia está en el sorteo). La IA debería construir un cuartel, contratar un
héroe, entrenar arqueros y druidas, y subir la población, en ese orden. En `Logs\imperivm\imperivm-mod.log` sale `script: GetTacticScript: la IA de Dacia usa
TS_GaulTactic2`.

Como es un parche y no una copia del archivo, dos mods pueden dar táctica a su civilización a la vez sin pisarse. Cambia la jugabilidad: todos los
jugadores llevan el mod. Ver [Civilizaciones](../../civilizaciones.md#la-ia) y [Parches](../../parches.md).
