# Esempio: unità con grafica propria

Il **Soldado simple**: un'unità nuova con i suoi sprite, senza riutilizzare quelli dell'hastatus. È un soldato di cubetti in 8 direzioni, con 6 fogli di animazione (riposo, camminata, attacco, morte, transizione riposo/combattimento e provocazione), colore del giocatore (tunica e cimiero cambiano con la squadra) e ombra. Si recluta nella caserma di Republican Rome (tasto `X`, 90 di oro) ed è più debole dell'hastatus, per distinguerlo.

| File | Cos'è |
|---|---|
| `units\RSimpleSoldier\*.rle` | Gli sprite nel formato del gioco (12 file, circa 500 KB): un foglio per animazione e la sua ombra. Generati, non si modificano a mano |
| `units\RSimpleSoldier\RSimpleSoldier.ent.xml` | L'entità: quale immagine si disegna in ogni livello e animazione. La genera `crea-entita.js` |
| `data\classes\RSimpleSoldier.sc.xml` | La classe (statistiche), con `entity="Units/RSimpleSoldier/RSimpleSoldier.ent.xml"` |
| `data\commands\SimpleSoldier.xml` | L'ordine di reclutarlo nella caserma. Usa il ritratto e il pulsante dell'hastatus |
| `loc\spanish.loc.xml` | Nome e testo del pulsante in spagnolo |

Gli sprite li disegna `..\generatore-unita-grafica\disegna-soldato.js`, che non fa parte della mod (una mod non ammette `.js`). Disegna i PNG, chiama `converti-sprite.js --giocatore` e poi `crea-entita.js unita`. Con i tuoi disegni salti il primo passo: [Grafica personalizzata](../../grafica.md).

Attenzione: lo script scrive sempre dentro `units\RSimpleSoldier\` di questo esempio e sovrascrive quello che c'è. Se vuoi solo vedere i PNG, `--png <cartella>` li lascia lì; ma i `.rle` e il `.ent.xml` vengono rigenerati comunque. Per non toccare l'esempio, lavora con una copia dell'SDK:

```
node esempi\generatore-unita-grafica\disegna-soldato.js
```

## Come provarlo

1. Convalida (deve dire OK): `node strumenti\valida-mod.js esempi\esempio-unita-grafica --dati <carpeta DATA sacada con estrai-dati.js> --gioco "<cartella del gioco>"`.
2. Copia questa cartella in `<gioco>\mods\`, attivala nel gestore dei contenuti (o in un profilo) e riavvia.
3. Gioca con Republican Rome in una schermaglia, costruisci una caserma e cerca il nuovo pulsante ("Equipar soldado simple"), oppure premi `X` con la caserma selezionata.
4. Controlla:
   - Se il gioco si chiude al caricamento della partita: è l'errore tipico di un'immagine caricata male. `Logs\imperivm\imperivm-mod.log` indica l'immagine che ha dato errore.
   - Se tunica e cimiero hanno il colore del tuo giocatore e non il magenta.
   - Se guarda dove cammina. Se va di spalle o gira al contrario, l'ordine delle 8 direzioni del gioco è un altro (vedi "Ordine delle direzioni" in [Grafica personalizzata](../../grafica.md)).
   - Se i piedi sono nel punto dell'unità e se ha l'ombra.
   - Se cammina, attacca, muore e resta a terra, e se torna a riposo dopo il combattimento.
5. Se qualcosa non va, togli la cartella da `mods\` e torna al profilo Originale.

Cambia il gameplay: in multigiocatore tutti i giocatori devono avere la mod.
