# Civiltà

Il gioco ha 8 civiltà, numerate da 0 a 7: Gaul, Republican Rome, Carthage, Iberia, Imperial Rome, Britain, Egypt e Germany.

Imperivm Resurrected permette di dichiarare civiltà nuove con numeri a partire da 10. Ognuna eredita da una civiltà del gioco (la sua
*base*) tutto ciò che la tua mod non definisce. Gli strumenti richiedono Node.js.

Esempio pronto da copiare: [`esempi/esempio-civilta`](esempi/esempio-civilta).

## Dichiarare una civiltà

Un file `civs\<id>.json` nella tua mod:

```json
{
  "id": "dacia",
  "index": 10,
  "base": "Gaul",
  "name": {"spanish": "Dacia", "english": "Dacia", "italian": "Dacia"},
  "letter": "D"
}
```

| Campo | Cos'è |
|---|---|
| `id` | Il nome: lettere, numeri e `_`, inizia con una lettera, fino a 24 caratteri. Lo usi in `race="..."` e negli script. |
| `index` | Un numero da 10 a 99. Non cambiarlo dopo la pubblicazione: le partite salvate lo ricordano. Due mod non possono usare lo stesso. |
| `base` | La civiltà del gioco da cui eredita: `Gaul`, `RepublicanRome`, `Carthage`, `Iberia`, `ImperialRome`, `Britain`, `Egypt` o `Germany`. Se non la metti, `Gaul`. |
| `name` | Come si chiama in ogni lingua, oppure un solo testo. |
| `letter` | La lettera che fa da prefisso a classi e comandi (`DSwordsman`, `trainDSwordsman`). Se non la metti, usa quella della base. Mettine una tua solo se hai classi e comandi con quella lettera: il gioco ci costruisce nomi come quello dell'addestramento dei contadini. |
| `template` | (Opzionale) Prefisso dei tuoi modelli di città e villaggio, perché la mappa casuale non usi quelli della base. Vedi [Modelli di città](modelli-citta.md). |
| `internalName` | (Opzionale) Il nome con cui il gioco cerca i file dell'interfaccia della civiltà; di default, quello della base. |
| `folder` | (Opzionale) Il nome che compare nei testi di selezione. |
| `random` | (Opzionale, di default `true`) Se entra nel sorteggio della civiltà casuale, anche nelle mappe casuali. L'IA la pesca solo se ha anche `"ai": true`. Metti `false` per escluderla. |
| `biome` | (Opzionale) Una civiltà del gioco: terreno, alberi e oggetti che la mappa casuale mette intorno al suo inizio sono quelli di quella civiltà. Senza, quelli della base. Vedi *Il bioma*. |
| `ai` | (Opzionale) Con `true`, l'IA gioca con le tue classi e i tuoi comandi. Altrimenti gioca con la civiltà base. Lo strumento di clonazione lo imposta a `true`. Vedi *L'IA*. |
| `ownAi` | (Opzionale, con `ai`) Per le mod che portano tutti i propri script di IA e vogliono vedere la civiltà così com'è (`Dacia`, non la sua base). |
| `alias` | (Opzionale, lo scrive lo strumento di clonazione) `classes` e `commands`: nome nella base → nome nella tua civiltà (`"GArcher": "DArcher"`, `"trainGSwordsman": "trainGSwordsman Dacia"`). L'IA li usa per tradurre. |
| `variant` | (Opzionale) Con `true`, è una variante della sua base: gioca con le classi della base e gli script la vedono come la base. Vedi *Varianti*. |
| `replacesBase` | (Opzionale) Con `true`, la base non compare più nella lobby e questa civiltà prende il suo posto. Vedi *Varianti*. |

Con solo `id`, `index` e `base` hai una copia della base con un altro nome. Da lì cambi quello che vuoi.

## Usarla nelle tue classi e nei tuoi script

Nel `.sc.xml` di ogni unità o edificio:

```xml
<properties race="Dacia"/>
```

Il gioco conosce solo i nomi delle sue 8 civiltà. La mod riconosce l'`id` delle tue (senza distinguere maiuscole) e lo assegna alla
classe. Se imposti `internalName` a mano, vale anche quello. Quello di default (della base) non conta, quindi `race="Gaul"` resta
Gaul anche se hai una civiltà clonata da essa.

Una classe con `parent=` eredita statistiche e comportamenti, ma non l'entità (`entity`, il modello) né i suoni: rimettili, altrimenti
l'unità viene creata senza modello e sparisce. Per questo lo strumento di clonazione copia le classi per intero.

Negli script, ogni civiltà dichiarata è una costante con il suo `id` così come lo scrivi, con l'iniziale maiuscola (`dacia` dà `Dacia`,
`romeImperial` dà `RomeImperial`). Vale anche con il resto in minuscolo (`Romeimperial`):

```
if (GetPlayerRace(player) == Dacia)
	...
```

Gli script di IA del gioco confrontano con `Gaul`, `RepublicanRome`… e non conoscono le tue. Con `"ai": true` vedono la base. Per sapere con quale
civiltà gioca davvero un oggetto, usa `Mod_TrueRace(objeto)`.

## Nella lobby e sulla mappa

Le civiltà dichiarate compaiono nei menu a tendina della lobby multigiocatore, dopo quelle del gioco. In una mappa casuale la tua usa il terreno del suo bioma e parte con una città e un villaggio fatti con le sue classi
(`DTownhall`, `DBarracks`…). Se non le dai modelli propri, la disposizione degli edifici è quella della base. L'interfaccia del giocatore (barre dei comandi, angoli,
icone rapide e schermata finale) è quella della sua base.

Se qualcosa non compare, guarda `Logs\imperivm\imperivm-mod.log`: le righe con l'etichetta `civilizations` dicono cosa è stato registrato e cosa è fallito (un `id`
non valido, un `index` mancante o ripetuto, una `base` inesistente). Gli errori compaiono anche nella scheda della mod nel gestore dei contenuti.

## Varianti

Una variante è una civiltà del gioco con un altro nome nella lobby e qualcosa di suo all'inizio della partita. Per esempio, una mod in cui
Roma sceglie all'inizio tra repubblicana, imperiale e punica può offrire tutte e tre nella lobby:

```json
{
  "id": "RomeImperial",
  "index": 21,
  "base": "RepublicanRome",
  "name": {"spanish": "Roma imperial", "english": "Roman Empire", "italian": "Roma imperiale"},
  "variant": true,
  "replacesBase": true
}
```

Con `"variant": true`:

- Gioca con le classi e i comandi della base. Non ha classi proprie né `letter`, a meno che sostituisca tutta la base (vedi sotto).
- `GetPlayerRace` restituisce la base, quindi gli script del gioco e di altre mod la trattano come la base (navi, sentinelle, IA).
- `Mod_PlayerCiv(giocatore)` restituisce la civiltà scelta nella lobby (21 nell'esempio). Quello che la distingue va in una
  [patch di script](patch.md), per esempio ricercare all'inizio la Roma giusta:

```
@before while (EnvReadInt(set, "ChooseRomanRace") != 1)
	if (Mod_PlayerCiv(.player) == RomeImperial && set.CanResearch("ChooseImperialRome"))
		set.Research("ChooseImperialRome");
```

- Anche l'IA la gioca: `ai` vale `true` se non dici altro.
- Con `"replacesBase": true` la base non compare più nella lobby e le sue varianti prendono il suo posto. Nel sorteggio casuale, la
  quota della base si divide tra loro. Un giocatore che aveva scelto la base passa alla prima variante. Le mappe e le partite salvate
  che fissano la base continuano a usarla.
- Una variante con `"replacesBase": true` può cambiare anche la lettera e i modelli della sua base per tutta la partita. Con `letter`, il
  gioco costruisce con quella lettera i nomi delle classi e dei comandi della base (`GetRaceStrPref`, l'addestramento dei contadini, le
  caserme, le sentinelle, l'ingaggio degli eroi). Con `template`, le città e i villaggi della base, anche quelli neutrali della mappa
  casuale, vengono dai tuoi modelli `<template> Town N` e `<template> Village N`. È quello che serve per sostituire una civiltà intera.
  Per esempio, la Grecia prende il posto di Roma imperiale con `"base": "ImperialRome"`, `"letter": "H"` e `"template": "Greek"`, e le
  sue classi hanno `race="ImperialRome"` e la lettera `H` (`HTownhall`, `HHoplite`). Se due varianti attive danno alla stessa base
  lettere o modelli diversi, vale la prima e il gestore avvisa sull'altra.

## Clonare una civiltà

Scrivere a mano le decine di classi e comandi di una civiltà non è divertente. `strumenti/clona-civilta.js` ne clona una del gioco.
Richiede la cartella `DATA` del gioco, che ottieni con [`estrai-dati.js`](strumenti/estrai-dati.js):

```
node estrai-dati.js "<cartella del gioco>" <cartella di output>
node clona-civilta.js <cartella di output>\DATA <cartella del mod> Dacia D Gaul 10
```

Genera `civs\dacia.json`, una copia di ogni classe della base (`DSwordsman` è una copia di `GSwordsman`, con `race="Dacia"`), i comandi che
nominano classi della base (`trainGSwordsman Dacia`, `trainpeasantd`, `hireheroD`…) e le patch che aggiungono le classi nuove ai comandi
condivisi. Clona anche gli eroi `GHero1`…`GHero7`, che discendono da `GaulHero` e non hanno `race`: senza eroi con un modello, assoldarne uno
blocca il gioco. Da lì modifichi quello che vuoi. Il risultato di esempio è in
[`esempi/esempio-civilta`](esempi/esempio-civilta).

Lo strumento non scrive il campo `template`. Se vuoi modelli propri, lo aggiungi tu ([Modelli di città](modelli-citta.md)).

Le copie non hanno l'`altid` degli originali. L'`altid` è un alias unico (`Barrack`, `Village`) con cui gli script e l'IA cercano
«la caserma» di una civiltà. Se due classi lo ripetono, l'IA di una delle civiltà non vede più i suoi edifici. Se scrivi le classi a mano, non copiarlo.

## L'IA

Di default, l'IA che sceglie una civiltà di mod gioca con la sua base. Con `"ai": true` gioca con le tue classi e i tuoi comandi. Gli script di IA
del gioco decidono in base alla civiltà dell'edificio centrale e costruiscono i nomi a partire da essa, quindi il gioco fa loro vedere la base e
traduce quello che chiedono:

- I comandi e le ricerche che chiedono per nome (`trainGSwordsman`, `Gaul Axeman`, `hireheroG`…) passano a quelli della tua civiltà, secondo
  `alias.ordenes`, solo negli oggetti di un'IA della tua civiltà.
- Le tue classi contano come quelle della base: `DArcher` è una `GArcher` per conteggi, caserme, templi e contro-unità.

Lo strumento di clonazione compila le due tabelle. Se scrivi le classi a mano, compilale tu. Per il giocatore umano non cambia niente.

Limiti dell'IA:

- La tua IA si comporta come quella della base, a meno che non le dai un'altra tattica.
- Un'unità che non sta nelle sei caselle della base non viene reclutata.
- Una ricerca nuova (non clonata da una della base) l'IA non la conosce.
- I costi che legge prima di pagare sono quelli della base, anche se il comando addebita i tuoi.

**Un'altra tattica per la tua civiltà.** Il gioco sceglie la tattica di ogni IA all'inizio, in `data\ai\GetTacticScript.vs`. Una
[patch di script](patch.md) può aggiungervi un pezzo per la tua civiltà. Poiché l'IA vede la base, la civiltà reale si chiede con
`Mod_TrueRace(set.GetCentralBuilding)`. L'esempio [`esempi/esempio-tattica-ia`](esempi/esempio-tattica-ia) fa usare alla Dacia una tattica
del gioco che nessuna IA usa. Le patch di più mod si sommano, quindi due civiltà possono scegliere la propria tattica contemporaneamente.

Con `"ownAi": true` la mod deve portare tutti gli script di IA, perché quelli del gioco vedono la civiltà 10 e le loro tabelle non la conoscono.
Di solito conviene lasciare `"ai": true` e cambiare solo la tattica.

## Il bioma

In una mappa casuale, ciò che circonda l'inizio di ogni giocatore (erba, sabbia, alberi, decorazioni) lo decide la sua civiltà. La tua usa quello della sua
`base`, o quello che metti in `biome`:

```json
{ "id": "dacia", "index": 10, "base": "Gaul", "biome": "Britain" }
```

Il gioco distingue solo quattro biomi di terreno e alberi, quindi le civiltà li condividono a due a due:

| Terreno e alberi | Lo usano |
|---|---|
| 1º | Gaul e Imperial Rome |
| 2º | Republican Rome e Britain |
| 3º | Carthage ed Egypt |
| 4º | Iberia e Germany |

Gli oggetti della mappa (rovine, palme, rocce) distinguono invece tutte e otto; Carthage ed Egypt fanno da interruttore «deserto» contro «temperato».
La città iniziale resta quella della base, a meno che usi `template`. Un bioma completamente tuo, con terreno nuovo, non si può fare.

## Cosa non si può fare

- Partite contro l'IA fuori dalla lobby multigiocatore (scenari e campagne) con civiltà di mod.
- Editor di mappe: le classi della tua civiltà compaiono mescolate nelle cartelle della sua base.
- Script di IA nuovi senza portarli tutti (vedi *L'IA*).
