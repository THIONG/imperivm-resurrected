# Esempio: specialità personalizzata

Una nuova specialità di unità, Golpe crítico (bit 36), assegnata all'hastatus romano: un colpo su quattro fa il triplo del danno.

| File | Cos'è |
|---|---|
| `specials\critical.json` | La dichiarazione: nome, bit e descrizione. |
| `patches\data\classes\RHastatus.sc.xml` | La assegna all'hastatus con `unit_specials="Deflection, Critical"`. |
| `events\OnAttackResolved.vs` | L'effetto: a ogni colpo, se l'attaccante ce l'ha, il 25 % fa il triplo del danno. |

Per provarlo: copia questa cartella in `<gioco>\mods\`, attivala nel gestore dei contenuti e riavvia. In `Logs\imperivm\imperivm-mod.log` compare `specials: registered Critical (bit 36…)` all'avvio, e `script: Golpe crítico: daño N` ogni volta che un hastatus ne mette a segno uno.

Cambia il gameplay: tutti i giocatori hanno la mod. Guida: [Specialità di unità](../../specialita.md).
