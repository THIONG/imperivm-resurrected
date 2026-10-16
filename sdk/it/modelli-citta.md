# Modelli di città e villaggio propri

Una civiltà di mod inizia la partita con una città e un villaggio che la mappa casuale prende da un elenco di modelli del gioco. Di
default usa quelli della sua civiltà base: una Dacia clonata da Gaul nasce in un «Gaul Town». Con modelli propri cambi la disposizione,
per esempio una città più larga, quasi quadrata, con un'altra distribuzione degli edifici dentro il recinto.

`--retta` non funziona bene (vedi più sotto).

## Cosa si può fare e cosa no

- Puoi spostare e ripetere i pezzi del gioco: mura (`?WallsN/S/E/W/NE/NW/SE/SW`), torri, porte ed edifici.
- Non puoi creare pezzi di muro nuovi. Gli angoli e le porte sono disegnati per il recinto ottagonale del gioco, quindi la forma
  «quadrata» è un ottagono con i lati lunghi più lunghi.
- Non c'è terreno proprio: il suolo è quello del bioma (erba o sabbia). La città del gioco ha una strada sterrata al centro, la tua no.
- I modelli escono solo per i giocatori di quella civiltà. I villaggi e le città indipendenti della mappa non li usano.

## Come si fa

1. Procurati i modelli del gioco. In `imperivm-mod.ini` (cartella del gioco), sezione `[Debug]`, scrivi
   `Dump=randommapsettlements/maps/1/map.obj.xml`. Apri il gioco, inizia una schermaglia su una mappa casuale ed esci. Compare
   `mod-dump\randommapsettlements\maps\1\map.obj.xml`. Poi togli quella riga.
2. Genera i modelli della tua civiltà:

   ```
   node strumenti\crea-modello.js "<cartella del gioco>" "<cartella del tuo mod>" Dacian --base Gaul --larghezza 1
   ```

   - `Dacian` è il prefisso. Crea «Dacian Town 1», «Dacian Town 2» e «Dacian Village 1..5».
   - `--base` è la civiltà da cui copia: Gaul, Republican Roman, Carthaginian, Iberian, Imperial Roman, Briton, Egyptian o German.
   - `--larghezza N` (da 0 a 4) allarga le città di N tratti da 220 unità. Con Gaul, `--larghezza 1` dà 1760 x 1760 e allarga solo «Town 1»: «Town 2»
     non ha la forma che serve, e lo script lo segnala e la lascia com'è.
   - `--senza-villaggi` genera solo le città.
   - `--retta` (sperimentale, con `--lato N`, da 6 a 9 tratti da 220 per lato, 8 di default) fa un recinto davvero rettangolare, senza gli
     angoli tagliati, con le porte N, S ed E al centro di ogni lato. La porta viene storta, perché il gioco ha solo
     porte disegnate per gli angoli tagliati. Non usarla se non per sperimentare.

   Scrive `patches\randommapsettlements\maps\1\map.obj.xml` nella tua mod.
3. Dillo alla civiltà: in `civs\<id>.json` aggiungi `"template": "Dacian"` ([Civiltà](civilta.md)).
4. Valida: `node strumenti\valida-mod.js <cartella del mod>`. Segnala se manca la patch, se non ci sono città o villaggi con quel prefisso e se qualche
   classe dei modelli non esiste.

Se la civiltà non ha un modello di un certo tipo (per esempio nessun villaggio), per quel tipo usa quello della sua base.

## Modificare a mano

Ogni `<settlement name="Dacian Town 1">` dell'XML è un elenco di pezzi con la loro classe e la loro posizione (x, y). Regole da rispettare:

- Tieni ogni lato sotto le 1900 unità circa. Il villaggio satellite si mette a circa 2000 dal centro della città, e una città più grande si
  sovrapporrebbe.
- Ogni classe dell'XML deve esistere, altrimenti la mappa casuale non viene generata e il gioco non avvisa. Usa solo classi del gioco o della tua mod. Il validatore
  lo controlla.
- Le torri vanno ogni 220 unità, e ogni pezzo di muro nel punto medio tra due torri. Una porta occupa due spazi.
- Lascia i modelli nella zona libera del piano (y tra 24192 e 27904) e distanziati di almeno 3000 unità l'uno dall'altro.
- Le patch di `randommapsettlements/` contano come gameplay: in una lobby online, tutti hanno la stessa mod.
