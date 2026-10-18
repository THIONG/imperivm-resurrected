# SDK di Imperivm Resurrected

*Traduzione non ancora rivista da un madrelingua: se trovi errori, segnalali.*

[Español](../es/README.md) · [English](../en/README.md) · **Italiano**

Imperivm Resurrected è una mod per Imperivm HD (Great Battles of Rome) che permette di creare mod prima impossibili: civiltà nuove, abilità, oggetti, modalità di vittoria, lingue, script che reagiscono alla partita e altro. Qui trovi le guide e gli strumenti per farle. Ogni guida ha una mod di esempio in [`esempi/`](esempi).

## Parti da qui

1. Installa Imperivm Resurrected: copia il suo contenuto nella cartella del gioco (`...\steamapps\common\Imperivm - Great Battles of Rome HD`) e avvia il gioco da Steam.
2. Copia una cartella di [`esempi/`](esempi) (per esempio `esempio-patch`) in `mods\`, dentro la cartella del gioco. Per partire da zero, usa [`modello-mod/`](modello-mod).
3. Nel menu principale, pulsante **Contenuti aggiuntivi**: attiva la mod nel tuo profilo.
4. Riavvia il gioco, se non si è riavviato da solo.
5. Apri `Logs\imperivm\imperivm-mod.log`, nella cartella del gioco. Lì trovi gli errori della tua mod e cosa ha caricato.

Prima di aprire il gioco, passa la mod dal [validatore](validare.md).

## Com'è fatta una mod

Una cartella dentro `mods\` con un `mod.json` e i file che cambia o aggiunge, con gli stessi percorsi che usa il gioco:

```
mods\
  mi-mod\
    mod.json
    data\classes\MiUnidad.sc.xml     <- file nuovo, o che sostituisce quello del gioco
    loc\spanish.loc.xml              <- testi che si sommano a quelli del gioco
```

Non inserire eseguibili né script di sistema (`.exe .dll .asi .bat .cmd .com .scr .ps1 .vbs .js .msi .sys .cpl .lnk .reg .jar .hta .wsf .pif`). Se ce n'è uno, la mod non si carica. Vale anche un `.js` sciolto, come un tuo generatore.

## `mod.json`

```json
{
  "id": "mi-mod",
  "name": {"spanish": "Mi mod", "english": "My mod", "italian": "Il mio mod"},
  "version": "1.0",
  "authors": ["Yo"],
  "description": {"spanish": "Qué hace mi mod.", "english": "What my mod does."},
  "category": "Gameplay & Balance",
  "redistributable": true
}
```

| Campo | A cosa serve |
|---|---|
| `id`, `name`, `version`, `authors`, `description` | La scheda della mod nel gestore. `name` e `description` valgono come testo singolo o come oggetto con un testo per lingua (si vede quello della lingua del gioco; se manca, l'inglese; se manca, il primo). |
| `category`, `tags` | La categoria principale di GameBanana e i tag: altre categorie (`Type`), giocatori (`Players`) e lingua (`Language`). Vedi [Pubblicare](pubblicare.md). |
| `redistributable` | Con `true`, l'host di una stanza può inviare la mod a chi non ce l'ha. Di default è `false`. |
| `license` | Testo libero con la licenza. |
| `requires` | Le mod che servono: un elenco di id, o di oggetti `{"id": "...", "version": "..."}`. |
| `incompatibleWith` | Elenco di id di mod con cui non si può attivare. |
| `loadAfter`, `loadBefore` | Ciò che chiedi all'[ordine di caricamento](riferimento.md#1-struttura-di-una-mod), per id; il resto lo calcola il gioco. Con `loadAfter` la tua mod sta sopra quella: vincono i suoi file, patch e testi (tranne un file che l'altra porta sciolto e la tua dentro un `.pak`: vince quello sciolto). |
| `sources` | `{"url": "...", "gamebanana": 123456}`. Lo scrive il gioco quando installi da Sfoglia contenuti; serve per avvisare delle versioni nuove. Se viene dalla sezione Sounds, il numero è in `gamebananaSound`. |
| `language` | Dichiara una lingua nuova. Vedi [Lingue](lingue.md). |
| `commands` | Nome di ogni file di comandi nella schermata Controlli. Vedi [Tasti](tasti.md). |

> Fino alla 0.31.50 i nomi di questi campi erano in spagnolo (`nombre`, `autores`, `descripcion`, `etiquetas`, `categoria`, `redistribuible`, `licencia`, `requiere`, `incompatibleCon`, `cargarDespuesDe`, `cargarAntesDe`, `fuentes`, `idioma`, `ordenes`…). I vecchi vengono ancora letti, ma scrivi i nuovi.

## Guide

- [Lingue e testi](lingue.md): cambiare i testi del gioco o aggiungere una lingua senza sostituire l'intero file.
- [Tasti delle mod](tasti.md): come compaiono i comandi della tua mod in Controlli e come chiamarli.
- [Patch](patch.md): cambiare una parte di un XML, INI o script del gioco senza sostituirlo; due mod possono toccare lo stesso file.
- [Script](script.md): script `.vs`, eventi e funzioni `Mod_...`.
- [Civiltà](civilta.md): dichiarare una civiltà nuova o clonarne una del gioco, con la sua IA, e giocarci nella stanza.
- [Modelli di città e villaggio](modelli-citta.md): città e villaggio iniziali propri per una civiltà di mod (`crea-modello.js`).
- [Unità nuove](unita.md): classe, comando per reclutarla, immagini e testi, riusando il modello di un'altra unità.
- [Statistiche delle unità](statistiche.md): cosa tocca ogni attributo, quali non fanno nulla e quali bloccano il gioco.
- [Specialità delle unità](specialita.md): specialità nuove e il loro effetto con gli script.
- [Abilità delle truppe](abilita.md): abilità nuove per unità o eroi (comando con pulsante e tasto, più un metodo con script).
- [Oggetti nuovi](oggetti.md): amuleti, erbe e gemme degli eroi.
- [Formazioni nuove](formazioni.md): formazioni da eroe con i loro bonus e il loro pulsante.
- [Avamposti ed edifici](avamposti.md): avamposti neutrali nuovi nelle mappe casuali (`crea-avamposto.js`) e classi di torri, mura e porte.
- [Modalità di vittoria](modalita-vittoria.md): uno script in `data\GameScripts` aggiunge una modalità al menu a tendina "Modalità" della stanza.
- [Opzioni di partita proprie](opzioni.md): interruttori nuovi nelle regole della stanza, che gli script leggono con `Mod_GetOption`.
- [Tipi di mappa casuale](mappe-casuali.md): un disegno a quattro colori diventa un nuovo tipo di mappa (`crea-mappa-casuale.js`).
- [Immagini proprie](immagini.md): pulsanti dei comandi e icone delle unità a partire da un PNG.
- [Grafica propria](grafica.md): sprite propri per unità ed edifici (`converti-sprite.js`, `crea-entita.js`, `leggi-rle.js`).
- [Validare una mod](validare.md): controllare una mod senza aprire il gioco, con file e riga di ogni errore.
- [Pubblicare su GameBanana](pubblicare.md): categorie, tag, caricamento e mod fatte prima dell'SDK.
- [Riferimento](riferimento.md): l'elenco completo di ciò che una mod può dichiarare (file, campi, patch, eventi, funzioni e limiti).

## Strumenti

Stanno in `strumenti\` e richiedono [Node.js](https://nodejs.org). Apri un terminale in questa cartella e lanciali da qui (`node strumenti\valida-mod.js ...`). Ognuno spiega come si usa se lo lanci senza argomenti, e la guida dell'argomento lo mostra con degli esempi. I loro messaggi escono in italiano; per averli in spagnolo o in inglese, aggiungi `--lang es` o `--lang en`. Ogni strumento accetta le sue opzioni nelle tre lingue (`--nome`, `--name`, `--nombre`); le guide usano quelle della propria lingua.

| Vuoi… | Usa |
|---|---|
| Un'unità nuova | `crea-unita.js` ([guida](unita.md)) |
| Un oggetto nuovo | `crea-oggetto.js` ([guida](oggetti.md)) |
| Un'abilità nuova | `crea-abilita.js` ([guida](abilita.md)) |
| Una formazione nuova | `crea-formazione.js` ([guida](formazioni.md)) |
| Un'opzione di partita | `crea-opzione.js` ([guida](opzioni.md)) |
| Una civiltà nuova | `clona-civilta.js` ([guida](civilta.md)) |
| Città e villaggio propri di una civiltà | `crea-modello.js` ([guida](modelli-citta.md)) |
| Un avamposto neutrale | `crea-avamposto.js` ([guida](avamposti.md)) |
| Un tipo di mappa casuale | `crea-mappa-casuale.js` ([guida](mappe-casuali.md)) |
| Pulsanti e icone da un PNG | `converti-immagine.js` ([guida](immagini.md)) |
| Sprite da PNG, e la scheda di un'unità o edificio | `converti-sprite.js`, `crea-entita.js`, `leggi-rle.js` ([guida](grafica.md)) |
| Controllare una mod senza aprire il gioco | `valida-mod.js` ([guida](validare.md)) |
| Adattare una mod fatta prima dell'SDK | `riduci-mod.js` ([guida](pubblicare.md)) |
| Estrarre i dati del gioco | `estrai-dati.js` (sotto) |

## Dati del gioco

Diversi strumenti chiedono la cartella `DATA` del gioco. La estrae `estrai-dati.js` (serve [Node.js](https://nodejs.org)):

```
node strumenti/estrai-dati.js "<cartella del gioco>" <cartella di output>
```

Lascia `data.pak` in `<cartella di output>\DATA`. È la cartella che chiedono `clona-civilta.js`, `crea-avamposto.js` e `valida-mod.js --dati`.

## Cosa conta come gameplay?

Nel multiplayer, tutti devono avere le stesse mod se cambiano ciò che succede in partita. Il gioco lo calcola dalle cartelle che la tua mod tocca: `data/`, `scenarios/`, `adventures/`, `conquests/`, `civs/`, `specials/`, `events/`, `options/`, `randommap/`, `randommapsettlements/`, i `.xml` e `.pass` di `units/`, `buildings/` e `mapobjects/`, gli `.xml` di `visuals/`, le mappe `.bfhp` di `packs/` e le patch (in base al file che correggono).

Grafica, suoni, testi (`loc/`) e lingue non contano: ogni giocatore può avere i suoi.

## Se qualcosa non va

Gli errori della tua mod compaiono nella sua scheda nel gestore e in `Logs\imperivm\imperivm-mod.log`. Ogni riga del registro ha l'ora, un livello (`I` informazione, `W` avviso, `E` errore) e un'etichetta con la parte della mod da cui viene: `patch`, `language`, `civilizations`, `specials`, `script`, `event`. Guarda prima le righe `W` ed `E` delle etichette della tua mod.

Il registro conserva le ultime cinque sessioni (da `imperivm-mod.1.log` a `imperivm-mod.4.log`) e, se il gioco si è chiuso male, `imperivm-mod.crashed.log`. Per più dettagli su una parte, metti la sua etichetta e un livello in `[Debug]` di `imperivm-mod.ini`, per esempio `LogChannels=patch:debug`; `LogLevel=debug` lo fa per tutto.

Per chiedere aiuto, clicca sul cuore in basso a sinistra del menu principale e poi **Crea rapporto**. Crea uno zip con i registri e il tuo sistema, senza il tuo nome utente di Windows, gli ID Steam né gli indirizzi IP, e non invia nulla: decidi tu se condividerlo.

## Versione e licenza

La versione dell'SDK e le novità di ognuna sono in [CAMBIAMENTI](CAMBIAMENTI.md). La licenza, in [LICENZA](LICENZA.md).
