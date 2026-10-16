# Validare una mod

Il validatore controlla la tua mod senza aprire il gioco e ti dice gli errori con file e riga. Serve [Node.js](https://nodejs.org).

```
node strumenti/valida-mod.js <cartella del mod>
node strumenti/valida-mod.js <cartella del mod> --dati <cartella DATA>
```

`--dati` è la cartella `DATA` del `data.pak` estratto (la ricava `strumenti/estrai-dati.js`, vedi il [README](README.md#dati-del-gioco)). Con essa controlla anche che i file che la tua mod patcha esistano e che esistano gli script a cui puntano le tue classi. Guarda solo i percorsi che iniziano con `data/`: se patchi `units/…`, `buildings/…` o `currentlang/…`, non li controlla. Senza `--dati` si controlla tutto il resto.

Con `--gioco <cartella del gioco>` (sola lettura) guarda i nomi di `Units.pak`, `Buildings.pak`, `MapObjects.pak` e `Visuals.pak` per sapere se esistono le immagini e le entità che la tua mod prende dal gioco, e avvisa se il tuo `.rle` ha lo stesso nome di uno del gioco:

```
node strumenti/valida-mod.js <cartella del mod> --dati <DATA> --gioco "C:\Program Files (x86)\Steam\steamapps\common\Imperivm - Great Battles of Rome HD"
```

Esce con codice 1 se ci sono errori, con 0 se ci sono solo avvisi o niente, e con 2 se lo chiami male. Serve per automatizzare.

Un errore è qualcosa che il gioco rifiuterebbe o che fallirà. Un avviso è un sospetto: qualcosa che il gioco ignora o che probabilmente non volevi.

## Cosa controlla

- **`mod.json`**: JSON rotto, categoria che non è di GameBanana, testi con il tipo sbagliato (errori). Mancanza di `id`, `name`, `version` o `category`, nome di categoria vecchio, tag che il gioco non usa, mappa senza tag dei giocatori e campagna, traduzione o audio senza tag della lingua (avvisi).
- **File**: estensioni di eseguibili e script di sistema (errore: la mod non si caricherebbe) e percorsi con `..`. Un'estensione strana ma innocua (`.psd`, `.gif`, `.zip`…) dà solo un avviso: la mod si carica, il file occupa solo spazio.
- **`civs\*.json`**: `id` non valido, `index` fuori da 10–99 o ripetuto, `base` che non esiste, `letter` di più di una lettera, `biome` sconosciuto, `template` scritta male o senza il suo file. Con `"ai": true` senza tabella `alias` avvisa, e controlla che l'`alias` punti a classi e comandi che la mod ha.
- **`specials\*.json`**: `id` non valido o che è una specialità del gioco, `bit` fuori da 36–63 o ripetuto.
- **`options\*.json`**: la ripartizione dei bit (tra tutte le mod ne stanno 8).
- **Patch XML**: XML malformato (la patch non si applicherebbe per intero), operazione senza `select`, selettore scritto male, tag che non sono operazioni. Un selettore che non inizia con `/` o `//` dà errore anche se la DLL lo accetterebbe.
- **Patch di script (`.vs`)**: direttiva che non esiste, `@before`, `@after` o `@replace` senza testo, direttiva senza codice sotto, e `@top` con codice che non sia dichiarazione di variabili. Non controlla il codice della patch, né le funzioni `Mod_*`, né la firma.
- **Patch INI**: righe che non si capiscono, e una sezione nuova in `UNIT_SPECIALS.INI` (il gioco può chiudersi). Non avvisa dei commenti a fine riga di una patch, che il motore non supporta (vedi [Patch](patch.md)).
- **Classi (`data\classes`)**: XML malformato, `race` sconosciuta, `unit_specials` scritte male o sconosciute (il gioco distingue le maiuscole: `Parry`, non `parry`), e una classe senza `entity` (avviso: il gioco non la eredita dal padre e l'unità sparisce).
- **Codifica**: un XML di classe in Latin-1 con caratteri accentati. Il gioco non carica la classe e non avvisa; salvalo come UTF-8.
- **Comandi (`data\commands`)**: XML malformato e comandi ripetuti in un file.
- **Entità (`.ent.xml`)**: errore se l'immagine non esiste (il gioco si blocca caricando la classe; con la cartella della mod se porta i suoi `.rle`, e sempre con `--gioco`), `idx` ripetuto o fuori da 1–32, livelli e animazioni che puntano a qualcosa che non c'è, limiti del motore (32 immagini, 32 livelli, 8 stati, animazioni 1–28, 64 fotogrammi), e `columns`, `rows` o numero di `<frame>` che non tornano. Un'animazione che legge più righe di quante ne ha la sua immagine è solo un avviso.
- **Grafica (`.rle`, `.pass`)**: `.rle` tagliato o corrotto, `.rle.mmp` rinominato (senza pixel, da una mod non si carica), palette vuota, `.pass` malformato o vuoto, `pass_file` che non esiste, e un `.rle` con lo stesso nome di uno del gioco (vince quello del gioco; con `--gioco`).
- **`placeobjects.xml`** (mappe casuali): una classe nominata che non esiste (con `--dati`). La generazione della mappa si blocca senza messaggio.
- **Testi (`loc`)**: XML malformato, voci senza `text` o senza `result`.
- **Script (`.vs`)**: manca la firma nella prima riga, evento sconosciuto o con la firma sbagliata, `Sleep` in un evento che non può aspettare, funzioni `Mod_*` che non esistono, `Mod_HasSpecial` con un `Obj` invece di una `Unit`.

## Esempio di output

```
AVVISO  specials/c.json  senza "name": si vedrà l'id
ERRORE  specials/d.json  il bit 40 è già usato da specials/c.json
AVVISO  data/classes/G.sc.xml:2  specialità "Zzz" sconosciuta: il gioco la ignora

5 file controllati: 1 errori, 5 avvisi.
```

## Cosa non fa

Non applica le patch né esegue gli script: questo si vede solo nel gioco (usa `Dump=` di [Patch](patch.md) per vedere il risultato, e `Logs\imperivm\imperivm-mod.log` per gli errori in corso). Non giudica nemmeno la qualità delle immagini.

È pensato per mod nuove, fatte con questo SDK. Una mod vecchia che funziona nel gioco, come il Community Mod, dà errori (script senza firma, script di classe che non esistono, righe di animazione) senza che questo voglia dire che non si carichi.
