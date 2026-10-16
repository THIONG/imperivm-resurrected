# Tipi di mappa casuale personalizzati

Una mod può aggiungere un tipo di mappa casuale (come Coastal, Islands o Mountain passes) al menu a tendina "Tipo di mappa" della lobby. Il tipo è una forma disegnata: terra, mare, montagne e i punti in cui parte ogni giocatore. Il gioco la deforma, la ruota e la riempie di terreno, alberi e oggetti con le sue regole di sempre.

Esempio: [`esempi/esempio-mappa-casuale`](esempi/esempio-mappa-casuale) (un anello di terra con un mare al centro e montagne in mezzo). Il repo contiene solo il disegno: i file che legge il gioco si generano con il comando del passo 2.

## Come si fa

1. **Disegna la forma** in un PNG quadrato, con il programma che vuoi e quattro colori:

   | Colore | Significato |
   |---|---|
   | nero `(0, 0, 0)` | mare |
   | bianco `(255, 255, 255)` | terra |
   | magenta `(255, 0, 255)` | montagne: linee sottili, di circa 3 pixel (opzionale) |
   | rosso `(255, 0, 0)` | punti in cui il gioco può mettere i siti di partenza e le città neutrali (opzionale) |

   Un colore non esatto passa a quello più simile. Il trasparente conta come mare. Se il PNG non è quadrato, viene stirato a 512 x 512 senza avvisi. Poi il gioco lo stira alla dimensione della mappa, quindi usa tratti spessi: i dettagli fini si perdono.

   I disegni del gioco hanno centinaia di punti rossi sparsi sulla terra e il gioco sceglie tra quelli. Di solito conviene non disegnarli e lasciare che lo strumento li distribuisca. Disegna il rosso solo se vuoi che i siti compaiano in certe zone.

2. **Trasformalo in un tipo di mappa**, dalla radice del repo:

   ```
   node strumenti\crea-mappa-casuale.js <cartella della mod> Anillo dibujo.png --giocatori 8
   ```

   Per l'esempio:

   ```
   node strumenti\crea-mappa-casuale.js esempi\esempio-mappa-casuale Anillo esempi\esempio-mappa-casuale\disegni\anello.png --giocatori 8
   ```

   Lo strumento scrive nella tua mod `data\randommap\Anillo.rm.xml` e `randommap\Anillo\010.bmp … 120.bmp`. Il nome (`Anillo`) contiene solo lettere senza accento, numeri e spazi, inizia con una lettera e ha da 2 a 31 caratteri. Non usare quello di un tipo del gioco (`Coastal`…): lo strumento non avvisa e sovrascriveresti il suo.

   | Opzione | A cosa serve |
   |---|---|
   | `--giocatori N` | Massimo di giocatori in mappa grande (da 2 a 8). In quella media, fino a 6 |
   | `--piccolo N` | Massimo in mappa piccola (0 = non compare in piccola; 2 di default) |
   | `--distorsione X` | Quanto si deforma la forma nella generazione, da 0 a 0.5 (0.1 quasi niente, 0.25 molto; 0.2 di default) |
   | `--senza-rotazione` | Niente rotazione né riflesso casuale |

   Puoi passare più PNG (`dibujo1.png dibujo2.png …`) e il gioco ne sceglie uno a caso a ogni partita. Lo strumento riempie fino a 12 ripetendo i tuoi. Se 12 non è un multiplo del numero di PNG, la distribuzione risulta diseguale (con 5, tre PNG escono 3 volte e due escono 2). Con uno solo, esce sempre quella forma.

3. Attiva la mod e riavvia. La lobby offre "Casuale: Anillo".

## Cose da tenere presenti

- Cambia il gameplay: tutti i giocatori devono avere la stessa mod.
- "Cerca partita" usa un elenco fisso di cinque tipi del gioco: un tipo nuovo compare solo nelle partite che crei tu.
- Non si possono creare biomi nuovi: il gioco distingue quattro zone fisse. Puoi però cambiare quello che mette ciascuna con le [patch](patch.md) su `data\randommap\terrainmix.xml`, `placetrees.xml` e `placeobjects.xml`.
- Prova con il numero di giocatori che intendi consentire.
