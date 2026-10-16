# Modello di mod

Una cartella minima da cui partire. Copiala in `<gioco>\mods\`, cambiale il nome e compila `mod.json`.

1. Cambia `id` (lettere minuscole, numeri e trattini: è ciò che ti identifica), `name`, `authors` e `description`.
2. Scegli `category` ed `tags` dall'elenco di [Pubblicare](../pubblicare.md).
3. Aggiungi le cartelle che ti servono. Con gli stessi percorsi che usa il gioco:

```
data\classes\       classi nuove o copie di quelle del gioco
data\commands\      ordini (pulsanti)
patches\data\...    patch sui file del gioco
loc\                testi
events\             script di eventi
civs\               civiltà nuove
options\            opzioni di partita
```

Cosa va in ogni cartella, e il dettaglio di ogni cosa, è nelle guide ([indice](../README.md)); l'elenco completo dei campi, nel [riferimento](../riferimento.md).

Prima di aprire il gioco: `node strumenti\valida-mod.js <la tua cartella> --dati <cartella DATA>`. La cartella DATA si ottiene con `estrai-dati.js` (vedi [Convalidare una mod](../validare.md)).
