# Bonus iniziali

Un bonus iniziale è uno script che il gioco esegue per un giocatore all'inizio della partita. La stanza li elenca nella colonna "Bonus" di ogni giocatore, e una mod può aggiungere i suoi.

## Lo script

Va in `data\BonusScripts\` della mod, con il nome `NNN NOME.vs`:

```
data\BonusScripts\004 Tributo.vs
```

- Il numero ordina l'elenco ed è il valore che la stanza invia agli altri giocatori. Nella stanza compaiono solo i numeri da `000` a `099`; da `100` in poi sono script avviati da altri script (le conquiste del gioco li usano) e non vengono mai offerti.
- `000 None.vs` deve esistere ed essere vuoto: è la scelta "nessun bonus". Il gioco porta da `000` a `003`; un bonus nuovo prende il numero libero successivo.
- Il nome nell'elenco è quello del file senza il numero, con la maiuscola solo all'inizio di ogni parola (`004 BOAR TEETH.vs` è "Boar Teeth").
- La firma è fissa: `//void, Settlement set, int playerid`. Lo script viene eseguito una volta per ogni giocatore che lo ha scelto, con il suo insediamento principale.

Esempio, 3000 monete d'oro all'inizio:

```
//void, Settlement set, int playerid
//@name 3000 gold
//@name.it 3000 oro
//@description 3000 gold at the start.
//@description.it 3000 monete d'oro all'inizio.

Sleep(100);
set.SetGold(set.gold + 3000);
set.GoldConverted(3000);
```

## Nome e descrizione nella stanza

Le righe che iniziano con `//@` all'inizio dello script danno il testo del menu a tendina. Devono stare prima della prima riga di codice.

| Riga | Uso |
|---|---|
| `//@name Testo` | Il nome mostrato nel menu a tendina e nella riga del giocatore (breve: la colonna è stretta). |
| `//@description Testo` | Cosa fa il bonus. Viene mostrata accanto al menu quando il puntatore è sopra quel bonus. |
| `//@name.es`, `//@description.it`… | Lo stesso per una lingua (`en`, `es`, `it`). Se la lingua del giocatore non ha una riga, si usa la riga senza lingua e poi quella inglese. |

Senza queste righe il nome è quello del file e non c'è descrizione. I tre bonus del gioco hanno nome e descrizione propri finché il loro script è quello del gioco; una mod che cambia l'effetto di uno di essi riceve il nome del file, quindi deve aggiungere queste righe. Se due bonus finiscono con lo stesso nome, la stanza aggiunge il numero del file per distinguerli.

## Online

Tutti i giocatori hanno bisogno degli stessi file: il contenuto della stanza viene confrontato prima della partita, e il valore del bonus è una posizione dell'elenco.

Richiede Imperivm Resurrected 0.9.0 o successiva.
