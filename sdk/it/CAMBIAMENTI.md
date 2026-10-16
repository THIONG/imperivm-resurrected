# Cambios

Versione dell'SDK e quali mod si possono fare con ognuna. La versione dell'API degli script (`Mod_ApiVersion()`) è a parte: ora è la 6.

## 1.1

Richiede Imperivm Resurrected 0.31.58 o successiva.

- I nomi dei campi di `mod.json`, `civs\*.json`, `specials\*.json` e `options\*.json` passano all'inglese (`name`, `authors`, `description`, `tags`, `category`, `index`, `biome`, `template`, `letter`, `values`, `default`…). I nomi in spagnolo continuano a valere.
- Le direttive delle patch degli script (`.vs`) diventano `@top`, `@bottom`, `@before`, `@after`, `@replace` e `@end`. Le vecchie continuano a valere.
- Le chiavi di `imperivm-mod.ini` passano all'inglese (`[Debug]`, `Dump`, `LogFiles`, `[Mods] Override`…). Il gioco rinomina le vecchie all'avvio.
- Il registro passa a `Logs\imperivm\imperivm-mod.log` (il gioco cancella all'avvio i file sciolti di `Logs\`). Le sue righe hanno un livello (`I`, `W`, `E`) e un'etichetta (`patch`, `language`, `civilizations`, `specials`, `script`, `event`) al posto di un prefisso di testo come `Patch:`, e vengono conservate le ultime cinque sessioni. `LogLevel` e `LogChannels` in `[Debug]` di `imperivm-mod.ini` chiedono più dettagli. Il cuore nel menu principale crea un rapporto con i registri da condividere.

## 1.0

Richiede Imperivm Resurrected 0.29.5 o successivo.

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
