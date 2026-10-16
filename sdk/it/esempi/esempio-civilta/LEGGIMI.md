# Esempio: civiltà

Dacia: una civiltà nuova (indice 10, lettera `D`) clonata da Gaul con [`strumenti/clona-civilta.js`](../../strumenti/clona-civilta.js).

```
node estrai-dati.js "<cartella del gioco>" <cartella di output>
node clona-civilta.js <cartella di output>\DATA <cartella della mod> Dacia D Gaul 10
```

`estrai-dati.js` estrae `data.pak` in `<cartella di output>\DATA`, la cartella richiesta dallo strumento.

| File | Cos'è |
|---|---|
| `civs\dacia.json` | La dichiarazione della civiltà. Contiene `"template": "Dacian"`, aggiunto a mano. |
| `data\classes\D*.sc.xml` | Una copia di ogni classe di Gaul (`DSwordsman`, `DBarracks`, `DVillage`…) con un altro id e `race="Dacia"`. Sono copie intere, non figlie: il gioco non eredita dal padre l'entità (il modello) né i suoni. |
| `data\commands\Dacia.xml` | Gli ordini di Gaul che nominano le sue classi (`trainGSwordsman Dacia`, `trainpeasantd`, `hireheroD`…), indirizzati alle classi nuove. |
| `patches\data\commands\*.xml` | Aggiungono le classi nuove agli ordini condivisi (riparare, potenziamenti della caserma, fucina…). |
| `patches\randommapsettlements\maps\1\map.obj.xml` | I modelli «Dacian Town» e «Dacian Village» (città allargata). Li crea `crea-modello.js`; vedi [Modelli di città](../../modelli-citta.md). |

Dacia compare nei menu a tendina della lobby multigiocatore. Cambia il gameplay: tutti i giocatori hanno la mod. Nel log `Logs\imperivm\imperivm-mod.log`
(righe con l'etichetta `civilizations`) si vede che viene registrata e, con `LogChannels=civilizations:debug` in `[Debug]` di `imperivm-mod.ini`, quale giocatore gioca con lei.

Vedi [Civiltà](../../civilta.md).
