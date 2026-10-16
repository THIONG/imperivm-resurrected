# Grafica personalizzata per unità ed edifici

L'SDK non disegna niente: usa Photoshop, Paint.NET, Aseprite, Blender… Il gioco legge solo il suo formato `.rle`, che nessun programma esporta, quindi l'SDK include un convertitore.

Una mod può portare i suoi sprite come file singoli, con gli stessi percorsi che usa il gioco. Un'unità o un edificio nuovo cambia il gameplay: tutti i giocatori della partita devono avere la mod.

Esempio: [`esempi/esempio-unita-grafica`](esempi/esempio-unita-grafica) (un soldato fatto di cubetti, 8 direzioni, colore del giocatore). Lo disegna `esempi\generatore-unita-grafica\disegna-soldato.js`, l'unica cosa che sostituiresti con i tuoi disegni.

Tutti i comandi di questa guida si lanciano dalla radice del repo. Gli strumenti sono Node.js e non richiedono altro.

## Cosa si può fare

- Un'unità nuova con disegni propri: sì, in una cartella nuova `units\MiUnidad\`.
- Cambiare il disegno di un'unità che esiste già: non con un file singolo, perché il gioco preferisce il suo. Crea un'unità nuova.
- Un edificio nuovo con disegni propri: sì, ma con più lavoro (fogli grandi, fasi di costruzione, impronta `.pass` e punti). Vedi Edifici.
- Icone e pulsanti dell'interfaccia: già risolto in [Immagini personalizzate](immagini.md).
- Suoni nuovi: `.wav` e `.ogg`.

## Gli strumenti

- `strumenti\converti-sprite.js`: dai tuoi PNG a `.rle`. Ritaglia, riduce a 256 colori, segna il colore del giocatore e crea l'ombra.
- `strumenti\crea-entita.js`: scrive il `.ent.xml` (immagini, livelli, stati, animazioni) di un'unità o di un edificio, e l'impronta `.pass` di un edificio.
- `strumenti\leggi-rle.js`: l'opposto, da `.rle` a PNG. Serve per guardare uno sprite del gioco e per partire da lì.
- `strumenti\valida-mod.js`: segnala immagini mancanti, `idx` ripetuti, righe e colonne che non tornano ([Convalida](validare.md)).

## Un'unità passo per passo

### 1. Disegna e salva i PNG

Un PNG per animazione, direzione e fotogramma, tutti della stessa dimensione (la tela, 150 x 150 per un'unità) e con sfondo trasparente:

```
Idle_d0_f00.png  Idle_d0_f01.png  ...  Idle_d7_f07.png      <Nome>_d<direzione>_f<fotogramma>.png, contando da 0
Walk_d0_f00.png  ...
Fight_..., Death_..., Transition_..., Taunt_...
```

- **Animazioni.** Obbligatorie: `Idle` (fermo), `Walk` (camminare), `Fight` (attaccare) e `Death` (morire). Opzionali: `Transition` (dal riposo al combattimento) e `Taunt` (provocare). Se mancano, si usa `Idle` al loro posto.
- **Direzioni.** 8 per i soldati, 12 per eroi e animali, 4 per le navi. Tutte le animazioni di un'unità hanno le stesse.
- **Ordine delle direzioni.** Nei fogli del gioco, la colonna 0 guarda la telecamera (in basso sullo schermo) e le altre seguono in senso orario: sud, ovest, nord, est con le diagonali in mezzo. Con uno sprite tuo, guarda prima un foglio del gioco con `leggi-rle.js`, poi disegna le otto.
- **Tela e piedi.** Stessa tela in tutti i fotogrammi e i piedi nello stesso punto. Nelle unità del gioco è il pixel (75, 100) di 150 x 150. Si cambia con `--tela` e `--piedi`.
- **Dimensione.** Il motore non ne impone nessuna: ogni fotogramma viene ritagliato a ciò che disegni. Come riferimento, un soldato del gioco misura circa 40 x 52 pixel e un eroe circa 40 x 88 (misure approssimative).
- **Fotogrammi.** Fino a 62 per animazione. Nell'hastatus: camminata 15, attacco 14, morte 20, riposo 8.
- **Trasparenza.** Tutto o niente: un alfa minore di 128 è trasparente. Il gioco non ha semitrasparenza.
- **Colori.** Fino a 256 per insieme; con il colore del giocatore, 192 (gli altri 64 sono la tinta). Se ne usi di più, il convertitore li riduce e avvisa.
- **Colore del giocatore.** Dipingi di magenta ciò che deve cambiare con la squadra. Il convertitore prende come tinta i magenta (rosso e blu simili, quasi niente verde). `(255, 0, 255)` è il colore del giocatore quasi puro; `(128, 0, 128)`, `(64, 0, 64)`… sono le sue ombreggiature (7 livelli utili). Evita viola e rosa nel resto della figura: potrebbero finire come tinta per sbaglio. Non ammorbidire i bordi: un bordo semitrasparente mescolato diventa rosa, non tinta.
- **Ombra.** Opzionale: un PNG per fotogramma con lo stesso nome più `_sombra` (`Walk_d3_f05_sombra.png`), nero o grigio scuro opaco su trasparente. Se non lo dai, il convertitore disegna un'ellisse sotto i piedi. Il gioco scurisce della metà il terreno sotto l'ombra.

Puoi anche dare un foglio già montato per animazione: `Walk.png` con le direzioni in colonne e i fotogrammi in righe, ogni cella della dimensione della tela (`--tela AnchoxAlto`). L'ombra va in `Walk_sombra.png` con la stessa disposizione.

### Partire da uno sprite del gioco

Gli sprite del gioco sono nei `.rle.mmp` dei `.pak`, e i loro pixel in `rle.mmp`, nella cartella del gioco. Prima estraili con `estrai-dati.js`:

```
node strumenti\estrai-dati.js "<cartella del gioco>" <cartella di output> Units.pak HASTATUS
```

E convertili in PNG con `--convenzione`, che scrive esattamente ciò che si aspetta `converti-sprite.js` (un PNG per direzione e fotogramma, a tela piena, la tinta del giocatore in magenta e l'ombra in `_sombra`):

```
node strumenti\leggi-rle.js <cartella di output>\UNITS\RHASTATUS\WALK.RLE.MMP --rle-mmp "<cartella del gioco>\rle.mmp" --convenzione --output <carpeta de PNG>
node strumenti\converti-sprite.js <carpeta de PNG> --giocatore --output <mod>\units\MiUnidad
```

Senza `--convenzione`, `leggi-rle.js` scrive un foglio con celle della dimensione di ogni ritaglio, e `converti-sprite.js` non lo accetta. `--tela AnchoxAlto` cambia la tela (150 x 150 di default). L'ombra del gioco è un altro file (`WALK_SHADOW.RLE.MMP`): se non la estrai, il convertitore disegna l'ellisse. Sono file tuoi del gioco, da studiare o da cui partire: non ridistribuirli.

### 2. Converti in `.rle`

```
node strumenti\converti-sprite.js <cartella con i PNG> --output <mod>\units\MiUnidad --giocatore
```

- `--giocatore`: usa il colore del giocatore (il magenta diventa la tinta della squadra). Senza, la palette è libera di 256 colori, senza colore del giocatore; è la più semplice e quella che usano lupi e cervi.
- `--ombra auto|file|nessuna`: di default `auto` (i tuoi `_sombra` se ci sono, altrimenti un'ellisse).
- `--tavolozza-per-animazione`: una palette per ogni `.rle`. Di default ce n'è una per tutta l'unità, con la perdita di qualità di ridurre a 192 o 256 colori se ne usi di più. Il gioco ne usa una per animazione.
- `--tela AnchoxAlto` (di default quella dei PNG), `--piedi X,Y` (solo per l'ombra automatica), `--soglia-alfa N`.

Scrive `Idle.rle`, `Idle_shadow.rle`, `Walk.rle`…

### 3. Crea l'entità (`.ent.xml`)

```
node strumenti\crea-entita.js unita <mod>\units\MiUnidad --nome MiUnidad
```

Legge i `.rle` della cartella e scrive `MiUnidad.ent.xml`: 2 livelli (unità `z=1000`, ombra `z=800`), 2 stati (riposo e combattimento) e le 7 animazioni del motore (camminata 1, attacco 5, morte 9, `Idle` 13, `ToIdle` 18, `ToAttack` 19, `Taunt` 20) con i tempi dell'hastatus (camminata 55 ms per fotogramma, attacco 71, morte 80 con l'ultima parte lenta, riposo 150…). Sono 6 fogli di animazione e 7 animazioni del motore, perché il foglio `Transition` vale per `ToAttack` e, al contrario, per `ToIdle`.

Limiti del motore: vedi *Riferimento del formato*. Per cambiare il ritmo, `--config animaciones.json`:

```json
{ "Walk": { "duracion": 55, "paso": 42 }, "Fight": { "duracion": 71, "golpe": 426 } }
```

`duracion` sono i millisecondi per fotogramma. `paso` (`step`) e `golpe` (`action_time`, i millisecondi fino all'uscita del colpo) fanno parte della simulazione e influiscono sul gameplay.

### 4. La classe e il resto

La classe (`data\classes\MiUnidad.sc.xml`) è una copia intera di una del gioco con `entity="Units/MiUnidad/MiUnidad.ent.xml"`. L'ordine di reclutamento, il pulsante e i testi sono in [Unità nuove](unita.md). Non fare la classe figlia di un'altra: il gioco non eredita `entity` dal padre e l'unità sparisce. Il nome della cartella deve essere nuovo: se ripeti quello di un'unità del gioco, il gioco usa il suo `.rle` e il tuo viene ignorato.

### 5. Convalida e prova

```
node strumenti\valida-mod.js <mod> --dati <DATA> --gioco "<cartella del gioco>"
```

Un'immagine che non esiste manda in crash il gioco al caricamento della classe (un `idx` ripetuto o un `.rle` corrotto, lo stesso), e il validatore lo rileva prima. Poi copia la mod in `<gioco>\mods\`, attivalo e riavvia. Se il gioco si chiude, `Logs\imperivm\imperivm-mod.log` indica l'immagine che ha dato errore.

## Edifici

Stessa forma di un'unità, con queste differenze:

- **Una direzione** (`_d0`) e più righe, che sono le fasi di costruzione (quelli del gioco ne usano 4). Un foglio per livello dell'edificio, `layer1`, `layer2`… (fino a 5 nel gioco), più `shadow`. PNG: `layer1_d0_f00.png` (livello 1, fase 0), `layer1_d0_f01.png`… e `shadow_d0_f00.png`… Un'animazione chiamata `shadow` viene presa direttamente come maschera d'ombra (conta qualsiasi pixel opaco).
- **Tele grandi**, di circa 700 x 700 nel gioco. Il punto in cui si pianta l'edificio (`--piedi`) lo scegli tu; nella caserma romana è (305, 330).

```
node strumenti\converti-sprite.js <PNG> --output <mod>\buildings\MiEdificio --giocatore --ombra nessuna
node strumenti\crea-entita.js edificio <mod>\buildings\MiEdificio --nome MiEdificio --piedi 305,330 --impronta 8x8 --punti-da <otro .ent.xml>
```

- **L'impronta (`.pass`)** è la griglia a 1 bit che dice dove non si può passare. Ogni cella è di 16 unità di mappa (poco meno di mezzo soldato di larghezza). `--impronta AnchoxAlto` crea un rettangolo centrato sul punto di ancoraggio. `--impronta-png máscara.png` usa un disegno (1 pixel = 1 cella, nord in alto; opaco e scuro = bloccato, trasparente o bianco = libero). Non superare circa 120 x 120 celle. Una casa del gioco misura circa 15 celle per lato e la caserma romana circa 29.
- **I punti (`<point>`)** sono le porte, i posti delle sentinelle, il fumo… Il convertitore non può inventarli. Con `--punti-da <.ent.xml de un edificio parecido del juego>` copia quelli di quell'edificio; le loro coordinate sono relative al punto di ancoraggio, quindi correggile se il tuo ha un'altra forma.
- La classe dell'edificio (`CVXBuilding`, `parent="Building"`, ordini, costi…) si fa come in [Unità nuove](unita.md): copia intera di una del gioco.
- Non ancora fatto: versioni invernali (`entity_winter`) e animazioni degli edifici (fumo, fuoco). Si possono aggiungere a mano.
- Il messaggio finale di `converti-sprite.js` suggerisce `crea-entita.js unita` anche se converti un edificio: usa `edificio`.

## Riferimento del formato

Se vuoi leggere o scrivere i formati per conto tuo, il codice è in `strumenti\lib\rle.js` (`.rle`) e `strumenti\lib\pass.js` (`.pass`). Gli strumenti scrivono `.rle` a 256 colori (con o senza colore del giocatore) e ombra; non c'è il true color.

Limiti del `.ent.xml`:

- `<image idx file rows columns drawmode>`: 32 immagini; `idx` da 1 a 32 senza ripetizioni; `file` senza estensione o con `.rle`.
- `<layer idx name image z …>`: 32. Unità `z=1000`, ombra `z=800`.
- `<state idx …>`: 8.
- `<anim idx name frames duration action_time step>`: `idx` da 1 a 28; `frames` = righe + 2 (due fotogrammi sentinella di durata 0); fino a 64 fotogrammi.
- `variations` di `<entity>`: pari alle colonne di tutte le immagini.

## Suoni

La classe indica i suoi suoni con `<sounds attack="…" die="…"/>`. Ogni nome è un file `data\sound entities\<Nome>.xml` che elenca i `.wav` o `.ogg`; puoi mettere i tuoi nella mod. Se non vuoi suoni nuovi, copia quelli di un'unità simile (`attack="UnitSwordFight" die="UnitDeath"`).
