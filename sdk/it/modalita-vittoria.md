# Modalità di vittoria personalizzate

Le condizioni di vittoria del gioco (Eliminazione, Limite di punteggio…) sono script `.vs`. Una mod può portare le sue, e compaiono da sole nel menu a tendina "Modalità" della lobby.

Esempio: [`esempi/esempio-vittoria`](esempi/esempio-vittoria) (modalità "Dominio": vince chi controlla N insediamenti).

## Lo script

Va in `data\GameScripts\` della tua mod:

```
data\GameScripts\6 Dominio.vs
```

- **Il nome del file è l'id della modalità**, numero compreso. La lobby elenca le modalità per nome completo del file. Due mod entrano in conflitto solo se usano esattamente lo stesso nome (`6 Dominio.vs` in entrambi). Quelle del gioco vanno dall'1 al 5.
- Nella lobby compare senza il numero iniziale e passa per la traduzione: se il testo "Dominio" ha una voce nel tuo `loc`, apparirà tradotto ([Lingue](lingue.md)).
- La firma è `//void, int player, str param`. Il gioco carica sempre lo script con essa, quindi la prima riga è facoltativa, ma conviene scriverla. `player` è il giocatore (da 1 a 8). `param` è il valore del limite scelto nella lobby (vedi più avanti).
- Il gioco lancia 8 copie dello script, una per giocatore, ognuna nel suo thread, anche se i giocatori sono meno. Scrivilo dal punto di vista di un solo giocatore, che guarda gli altri. Deve reggere i posti vuoti: quello dell'esempio li considera eliminati perché non hanno né insediamenti né truppe.

## Funzioni che si usano

| Funzione | A cosa serve |
|---|---|
| `EndGame(player, false)` / `EndGame(player, true)` | Il giocatore vince / perde. Poi, `return` |
| `SetPlayerStatus(player, linea, texto, important)` | Testo di stato in alto: riga 0 il nome della modalità, riga 1 l'avviso. Esiste anche `SetPlayerStatus(player, linea, bool)`; gli script del gioco usano entrambe |
| `ClassPlayerObjs("BaseTownhall", player)` | Oggetti di una classe di un giocatore; `.count` li conta, `.IsEmpty()` dice se non ce n'è nessuno |
| `MilUnits(player)` | Quante truppe ha |
| `DiplAreAllied(a, b)` | Se due giocatori sono alleati |
| `Str2Int(param)` | Converte `param` in numero |
| `EnvReadInt` / `EnvWriteInt` | Variabili condivise tra gli script. Con giocatore (`EnvWriteInt(player, "clave", n)`) o senza (`EnvWriteInt("modo", 1)`) |
| `GetConst("…")` | Costanti di `CONST.INI` |
| `Translate("…")` / `Translatef("…", a, b)` | Testi con lingua |

Per vedere come fa il gioco, estrai i suoi script con `estrai-dati.js` (`node strumenti\estrai-dati.js "<cartella del gioco>" <cartella di output>`) e guarda `GAMESCRIPTS\1 ELIMINATION.VS` e `2 SCORE LIMIT.VS`. Si può anche con il dump delle [Patch](patch.md), con il percorso completo: `Dump=data/GameScripts/1 ELIMINATION.VS`.

## Il menu a tendina "Limite"

Lo riempie il gioco con voci di `CONST.INI`, sezione `[GamePlay]`. La chiave è il nome del file con `_` al posto degli spazi, più un numero a partire da 0. Il valore è fatto di due parti separate da una virgola: il testo che si vede e quello che lo script riceverà come `param`.

L'esempio lo fa con una patch INI in `patches\data\const.ini`:

```ini
[GamePlay]
6_Dominio0 = 2 asentamientos, 2
6_Dominio1 = 3 asentamientos, 3
6_Dominio2 = 4 asentamientos, 4
6_Dominio3 = 5 asentamientos, 5
```

Se non metti voci, il menu a tendina resta vuoto e non c'è nessun limite da scegliere. Cosa riceva allora `param` non si è visto in partita: l'esempio ripiega su 3 se `Str2Int(param)` dà meno di 2.

## Online

Una modalità nuova cambia il gameplay: tutti i giocatori devono avere la stessa mod.
