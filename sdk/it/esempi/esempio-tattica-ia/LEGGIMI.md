# Esempio: tattica di IA personalizzata

Una civiltà di mod con `"ai": true` gioca per default come la sua base: gli script di IA del gioco vedono la civiltà base, quindi Dacia usa
le stesse cose di Gaul. Questo esempio fa scegliere all'IA di Dacia un'altra tattica.

`GetTacticScript.vs` decide all'inizio quale tattica usa ogni IA in base alla sua civiltà. Per Gaul restituisce `TS_GaulTactic`, che non compila, quindi l'IA gallica non segue nessuna tattica. Il gioco ne ha un'altra, `TS_GaulTactic2` (ordine di costruzione con eroe, caserma,
arcieri e druidi), che nessuna IA usa. Qui la usa Dacia.

| File | Cos'è |
|---|---|
| `patches\data\ai\GetTacticScript.vs` | Una [patch di script](../../patch.md): aggiunge un pezzo al `GetTacticScript.vs` del gioco, subito prima della sua prima riga di codice (le variabili si dichiarano prima). Se la vera civiltà dell'edificio centrale è Dacia (`Mod_TrueRace`), restituisce `TS_GaulTactic2`. Il resto dello script non viene toccato. |

`Mod_TrueRace(objeto)` dà la vera civiltà di un oggetto. `objeto.race` qui non va bene: con `"ai": true` l'IA vede la base (Gaul).

Richiede la mod «Ejemplo: civilización» (Dacia). Per provarlo: copia questa cartella in `<gioco>\mods\`, attivala insieme a `esempio-civilta` nel
gestore dei contenuti, riavvia e gioca contro un'IA di Dacia (o un'IA casuale: Dacia è nel sorteggio). L'IA dovrebbe costruire una caserma, ingaggiare un
eroe, addestrare arcieri e druidi, e far crescere la popolazione, in quest'ordine. In `Logs\imperivm\imperivm-mod.log` compare `script: GetTacticScript: la IA de Dacia usa
TS_GaulTactic2`.

Poiché è una patch e non una copia del file, due mod possono dare una tattica alla propria civiltà contemporaneamente senza sovrapporsi. Cambia il gameplay: tutti i
giocatori hanno la mod. Vedi [Civiltà](../../civilta.md#lia) e [Patch](../../patch.md).
