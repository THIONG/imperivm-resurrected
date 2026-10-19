# Cambiamenti

Versione dell'SDK e quali mod si possono fare con ognuna. La versione dell'API degli script (`Mod_ApiVersion()`) è a parte: ora è la 7.

## 1.6

Richiede Imperivm Resurrected 0.9.0 o successiva.

- Una variante che sostituisce la sua base (`"replacesBase": true`) può cambiare con `letter` e `template` la lettera e i modelli di
  città e villaggio di quella base per tutta la partita, anche quelli degli insediamenti neutrali della mappa casuale. Così si
  sostituisce una civiltà intera del gioco ([Civiltà](civilta.md#varianti)). Il validatore cerca i modelli anche nel
  `packs\randommapsettlements.bfhp` della mod.
- `modes\<nome>.json` fissa le impostazioni della stanza e le civiltà di una modalità di vittoria ([Modalità di vittoria](modalita-vittoria.md)).
- `rules\game.json` fissa regole del motore per tutte le partite della mod: l'esercito resta con il suo eroe alle mura, le sentinelle
  non si possono selezionare e alcune classi non si possono attaccare ([Opzioni](opzioni.md)).
- Uno script di bonus numerato da `004` a `099` in `data\BonusScripts` compare nella stanza, con il nome e la descrizione delle sue righe `//@` ([Bonus iniziali](bonus.md)).
- Il validatore conosce `Mod_PlayerCiv` e `Mod_CanExecCmd`.

## 1.5

Richiede Imperivm Resurrected 0.9.0 o successiva.

- `riduci-mod.js` converte anche le mod il cui `rle.mmp` cambia la grafica del gioco, non solo quelle che aggiungono grafica
  in fondo. L'`rle.mmp` del gioco resta intero: ciò che la mod cambia o aggiunge va dopo, in `rle.mmp.tail`, e le sue immagini
  si aggiornano per trovarlo nel suo nuovo posto. Le immagini del gioco che la mod non sostituisce si vedono ancora come nel gioco.
- `riduci-mod.js` rispetta meglio ciò che il gioco mostrava con la mod vecchia: un file sciolto in `Currentlang\` vince su
  quello del pacchetto di lingua, e un file sciolto uguale a quello dei pacchetti del gioco si conserva se qualche lingua ne
  mostrava un altro.
- Il validatore non dà più errore per due comandi con lo stesso nome per edifici diversi, perché il gioco lega ognuno agli edifici
  del suo `<src>`; avvisa solo se sono tutti e due per lo stesso edificio. Non chiede più nemmeno la firma nella prima riga delle
  modalità di vittoria (`data\GameScripts`) né degli script degli oggetti: gliela passa il gioco.

## 1.4

Richiede Imperivm Resurrected 0.9.0 o successiva.

- Una mod può comparire in più categorie di Sfoglia contenuti: quella principale è la sua categoria di GameBanana e le altre si
  aggiungono con tag `Type`, per esempio `Type: Civilizations`.
- I tag hanno un titolo, come su GameBanana: `Players: 2-4` e `Language: Italian`. Le forme senza titolo funzionano ancora e
  il validatore chiede quella nuova.
- Nel gioco le categorie si chiamano come su GameBanana: **Gameplay e bilanciamento** (prima Regole e bilanciamento),
  **Grafica** (prima Grafica e interfaccia) e **Traduzioni** (prima Testi e traduzioni).

## 1.3

Richiede Imperivm Resurrected 0.9.0 o successiva.

- Le categorie delle mappe si chiamano come le cartelle del gioco: **Scenarios** (Scenari), **Adventures** (Avventure) e **Conquests**
  (Conquiste), al posto di Maps, Campaigns e Conquest. I nomi precedenti funzionano ancora e il validatore chiede quello nuovo.
- Un `.bfhp` sciolto nella radice dello `.zip` viene sistemato, durante l'installazione, in `scenarios\`, `adventures\` o `conquests\` secondo
  il tipo indicato dalla mappa stessa (`game_type`); la categoria decide solo se una mappa normale è una missione di Adventures. Il
  validatore avvisa delle mappe che sono in una cartella che non è la loro.
- L'audio si pubblica nella sezione **Sounds** di GameBanana, come chiede GameBanana, e Sfoglia contenuti lo mostra in Audio e voci.
  Ciò che si installa da lì conserva il suo numero in `sources.gamebananaSound`.

## 1.2

Richiede Imperivm Resurrected 0.9.0 o successiva.

- `Mod_CanExecCmd(oggetto, comando)` (API 7): lo stesso controllo che fa il pulsante di un comando (l'oggetto lo ha, non è disattivato e
  il suo verificatore lo accetta). Il menu dei trucchi elenca le truppe delle classi caricate, anche quelle delle mod (`edittree_pos` sotto
  `Units/`), e ricerca ciò che i pulsanti permettono, nell'ordine dei loro requisiti e senza scegliere due opzioni di una scelta.
- Civiltà: `"variant": true` dichiara una variante di una civiltà del gioco, con le stesse classi e un altro nome nella lobby;
  con `"replacesBase": true` prende il posto della sua base nella lobby e nel sorteggio casuale.
  `GetPlayerRace` dà la base e `Mod_PlayerCiv(giocatore)` (API 7) quella scelta ([Civiltà](civilta.md#varianti)). L'interfaccia di una
  civiltà di mod (barre, angoli, icone rapide, schermata finale) è quella della sua base, non quella dei Galli. La costante degli script
  rispetta le maiuscole dell'`id` (`RomeImperial`).
- Un `.bfhp` di `packs\` portato da una mod (per esempio `RandomMapSettlements.bfhp`, le città iniziali della mappa casuale) si usa
  davvero: il gioco lo apre in scrittura e prima teneva il suo.
- I file sciolti di `Local\<lingua>\` (testi, guida, consigli) si usano sempre: prima si perdevano se il gioco li leggeva prima di montare
  le mod.
- Mod vecchie: `riduci-mod.js` rispetta l'ordine dei loro pacchetti, mette in `hidden.txt` ciò che toglievano dal gioco, scrive `rle.mmp.tail` se ampliano `rle.mmp` e `Local\<lingua>.pak` per i file di una lingua fuori da `currentlang\`, e trasforma `data\const.ini` in una patch senza `[Resolutions]`. Nuove opzioni `--escludi` e `--nascondi`. I filmati si cercano anche nei pacchetti.
- Nove categorie di GameBanana in tre tipi: Gameplay & Balance, Units & Heroes, Civilizations · Maps, Campaigns, Conquest · Graphics, Audio & Voices, Translations. `Scenarios`, `Interface` e `Total Conversions` valgono ancora come Maps, Graphics e Gameplay & Balance, ma il validatore chiede il nome nuovo.
- I tag servono solo per il filtro dei giocatori (`4 players`, `2-4 players`, nelle mappe) e per quello della lingua (`Spanish`…, in campagne, traduzioni e audio). `Gameplay`, `Cosmetic` e `Online tested` non servono più. Il validatore avvisa di una mappa senza giocatori e di una campagna, traduzione o audio senza lingua, e un tag sconosciuto ora è un avviso, non un errore.
- Il gestore non ha più il pulsante Pubblica: lo zip si fa a mano (vedi [Pubblicare](pubblicare.md)).
- L'ordine delle mod lo calcola il gioco ([Riferimento](riferimento.md#1-struttura-di-una-mod)): non si ordina più a mano nel profilo, e il controllo del multiplayer non dipende dall'ordine. La chiave di `imperivm-mod.ini` è `[Mods] Profile`.

## 1.1

Richiede Imperivm Resurrected 0.9.0 o successiva.

- I nomi dei campi di `mod.json`, `civs\*.json`, `specials\*.json` e `options\*.json` passano all'inglese (`name`, `authors`, `description`, `tags`, `category`, `index`, `biome`, `template`, `letter`, `values`, `default`…). I nomi in spagnolo continuano a valere.
- Le direttive delle patch degli script (`.vs`) diventano `@top`, `@bottom`, `@before`, `@after`, `@replace` e `@end`. Le vecchie continuano a valere.
- Le chiavi di `imperivm-mod.ini` passano all'inglese (`[Debug]`, `Dump`, `LogFiles`, `[Mods] Override`…). Il gioco rinomina le vecchie all'avvio.
- Il registro passa a `Logs\imperivm\imperivm-mod.log` (il gioco cancella all'avvio i file sciolti di `Logs\`). Le sue righe hanno un livello (`I`, `W`, `E`) e un'etichetta (`patch`, `language`, `civilizations`, `specials`, `script`, `event`) al posto di un prefisso di testo come `Patch:`, e vengono conservate le ultime cinque sessioni. `LogLevel` e `LogChannels` in `[Debug]` di `imperivm-mod.ini` chiedono più dettagli. Il cuore nel menu principale crea un rapporto con i registri da condividere.

## 1.0

Richiede Imperivm Resurrected 0.9.0 o successivo.

Una mod può, solo con file e senza compilare nulla:

- Cambiare qualsiasi file XML, INI o script del gioco con le patch, e sommare le modifiche di più mod.
- Aggiungere unità, con le loro statistiche, e grafica propria convertita da PNG.
- Aggiungere abilità di truppe ed eroi, e specialità.
- Aggiungere civiltà, con città e villaggio iniziali, la loro IA e i loro eroi.
- Aggiungere avamposti neutrali.
- Aggiungere oggetti.
- Aggiungere modalità di vittoria, con i loro limiti.
- Aggiungere opzioni di partita alla stanza: interruttori ed elenchi di valori.
- Aggiungere tipi di mappa casuale.
- Aggiungere formazioni da eroe.
- Aggiungere lingue e testi, comandi e pulsanti propri.
- Reagire a ciò che succede in partita con eventi di script e funzioni `Mod_*`.

Strumenti: validare una mod, estrarre i dati del gioco, creare un'unità, un oggetto, un'abilità, una formazione o un'opzione di partita, clonare una civiltà, creare un avamposto, modelli di città, tipi di mappa, convertire immagini e sprite, creare l'entità di un'unità o edificio, adattare una mod vecchia.
