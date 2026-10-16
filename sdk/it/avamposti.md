# Avamposti ed edifici di difesa

«Avamposto» indica due cose diverse:

- Gli avamposti neutrali della mappa: i posti sparsi che si possono conquistare («Avamposto commerciale», «Avamposto di reclutamento»…). Una mod può
  aggiungere tipi nuovi con uno strumento, e compaiono da soli nelle mappe casuali.
- Gli edifici di difesa delle città: torri, mura e porte.

## Un nuovo avamposto neutrale

`strumenti/crea-avamposto.js` copia un avamposto del gioco con un altro id e nome e lo inserisce nelle mappe casuali. Richiede la cartella `DATA`, che
ottieni con [`estrai-dati.js`](strumenti/estrai-dati.js) (`node estrai-dati.js "<cartella del gioco>" <cartella di output>`):

```
node crea-avamposto.js <cartella di output>\DATA <cartella del mod> ArcherFort IOutpost "Archer fort"
```

Scrive la classe (`data\classes\ArcherFort.sc.xml`), la patch di `data\randommap\placeobjects.xml` che lo aggiunge ai tre formati di mappa (con le
quantità della base) e il nome in spagnolo. Con `GOutpost`, `TOutpost` o `ROutpost` come base scrive anche `patches\data\commands\*.xml`, perché
il nuovo avamposto abbia i comandi dell'avamposto (vendere cibo, interesse).

Le basi possibili:

| Base | Tipo |
|---|---|
| `GOutpost`, `TOutpost` | Avamposto commerciale |
| `ROutpost`, `EOutpost` | Avamposto dell'oro (interesse) |
| `IOutpost`, `BOutpost` | Avamposto di addestramento |
| `COutpost` | Avamposto di reclutamento |

Poi modifichi la classe come vuoi: `maxhealth`, la guarnigione (`defender_cls_1`, `defenders_max_1`, `start_level_1`…), il suo comportamento
(`<behavior script=…>`), quanto costa conquistarlo. Esempio: [`esempi/esempio-avamposto`](esempi/esempio-avamposto).

Per cambiare quanti ne compaiono, modifica `min` e `max` delle righe della patch (quanti ne mette il generatore per mappa). `mindist` è la distanza minima
tra avamposti, in pixel. Gli altri numeri sono quelli del gioco.

Il modello (`entity`) e l'icona sono quelli della base. Per un modello con sprite propri vedi [Grafica propria](grafica.md): ha una sezione sugli
edifici.

## Torri, mura, porte e avamposti

Sono classi di edificio come le altre. Cosa conviene sapere prima di aggiungere le tue:

- Un insediamento (città, villaggio, avamposto) è un gruppo di edifici con proprietario, popolazione, oro e cibo. Il suo tipo dipende dalla classe dell'edificio
  centrale: `BaseTownhall` è una città, `BaseVillage` un villaggio, `Outpost` un avamposto.
- Il gioco base non permette di costruire edifici. Non c'è nessun comando di costruzione (solo navi e macchine d'assedio). Torri, mura, porte e avamposti
  vengono dalla mappa: il modello di città, gli avamposti della mappa casuale, gli scenari e l'editor.
- I modelli di città nominano classi specifiche (`GTownhall`, `GTower01`, `GWallsN`, `GGateNW`…). Una civiltà nuova usa le sue se le
  clona con lo strumento di [Civiltà](civilta.md).

| Classe base | Cos'è |
|---|---|
| `Tower` (`CVXBuilding`) | Torre. I suoi comandi (`Tower Attack`, riparare) dipendono da `Tower`, e una figlia li eredita |
| `Wall` (`CVXBuilding`) | Muro. `num_sentry_slots` e `sentry_class_name` dicono quante sentinelle lo pattugliano e di che classe |
| `Gate` (`CVXGate`) | Porta: aprire, chiudere, permanente, riparare |
| `Outpost` (`CVXTownHall`) | Base degli avamposti. Quelli specifici (`GOutpost`, `GGuardPost`…) usano `CVXOutpost` e discendono da `Outpost`: sono indipendenti finché non vengono conquistati e ognuno fa qualcosa con uno script (vendere cibo, interesse, esperienza…) |
| `GGuardPost` | Posto di guardia, solo dei Gaul: 12 sentinelle da script. Non compare in `placeobjects.xml`, e `crea-avamposto.js` lo segnala |

Per una classe nuova: un `.sc.xml` in `data\classes\` con `parent=` una di quelle qui sopra, una `cpp_class` che esista già (`CVXBuilding`, `CVXGate`,
`CVXOutpost`, `CVXTownHall`) e il suo `entity`. Copia l'`entity` dalla classe della base, perché il gioco non lo eredita dal padre. Il comportamento
proprio va con `<behavior>` e `<method sig="…" vs="…">`, e i comandi con `<cmd>`, come in [Abilità](abilita.md). Un tipo di edificio
con una `cpp_class` nuova non si può fare.

## Dove metterlo

| Posto | Come |
|---|---|
| Mappa casuale | Aggiungendolo a `placeobjects.xml`, che è quello che fa `crea-avamposto.js` |
| Città o villaggio iniziale di una civiltà | Le classi clonate compaiono da sole nella città iniziale. Per cambiare la disposizione, [Modelli di città](modelli-citta.md) |
| L'editor di mappe | Con `edittree_pos` nella classe. L'editor ha cartelle fisse per civiltà: quelle di una civiltà nuova compaiono mescolate con quelle della sua base |
| Una mappa a mano | Lo mette l'editor e la mappa nomina la classe (serve la mod) |

## Multigiocatore e limiti

Cambia il gameplay: tutti i giocatori devono avere la stessa mod.

- Nell'editor non ci sono cartelle né set di mura propri.
