# Esempio: un avamposto nuovo

Un nuovo avamposto neutrale, il Fortín de arqueros, di quelli sparsi per la mappa che si possono catturare. Compare nelle mappe casuali insieme a quelli
del gioco (tanti quanti l'avamposto di addestramento iberico), ha 15000 di vita e una guarnigione di 16 arcieri galli, con nome e descrizione in spagnolo.

È stato fatto con un comando. La cartella `DATA` si ottiene con `node estrai-dati.js "<cartella del gioco>" <cartella di output>`:

```
node strumenti/crea-avamposto.js <cartella di output>\DATA esempi/esempio-avamposto ArcherFort IOutpost "Archer fort"
```

| File | Cos'è |
|---|---|
| `data\classes\ArcherFort.sc.xml` | L'avamposto: copia dell'avamposto di addestramento iberico (`IOutpost`) con un altro id, nome, vita e guarnigione (modificati poi a mano). |
| `patches\data\randommap\placeobjects.xml` | Lo aggiunge all'elenco degli avamposti della mappa casuale, per le tre dimensioni di mappa, con le quantità della base. |
| `loc\spanish.loc.xml` | Il nome e la descrizione in spagnolo. |

Per provarlo: copia questa cartella in `<gioco>\mods\`, attivala nel gestore dei contenuti, riavvia e gioca una mappa casuale. Gli avamposti compaiono sparsi
per la mappa, qualunque civiltà giochi. Nell'editor compare in *Structures / Outposts / Archer fort*.

Cambia il gameplay: tutti i giocatori hanno la mod. Guida: [Avamposti ed edifici](../../avamposti.md).
