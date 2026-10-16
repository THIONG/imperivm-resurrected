# Esempio: tipo di mappa casuale

Un nuovo tipo di mappa casuale, **Anillo**: un anello di terra con un mare al centro e montagne in mezzo.

Questa cartella contiene solo il disegno, `disegni\anello.png` (nero mare, bianco terra, magenta montagne; i punti dei siti li distribuisce lo strumento). Copiata così com'è in `mods\` non aggiunge nessuna mappa: prima vanno generati i file che legge il gioco. Dalla radice del repo:

```
node strumenti\crea-mappa-casuale.js esempi\esempio-mappa-casuale Anillo esempi\esempio-mappa-casuale\disegni\anello.png --giocatori 8
```

Questo crea dentro la mod `data\randommap\Anillo.rm.xml` e `randommap\Anillo\010.bmp … 120.bmp`. Poi copia la cartella in `<gioco>\mods\`, attivala e riavvia. In una schermaglia, "Tipo di mappa" offre "Casuale: Anillo".

Guida: [Mappe casuali](../../mappe-casuali.md).
