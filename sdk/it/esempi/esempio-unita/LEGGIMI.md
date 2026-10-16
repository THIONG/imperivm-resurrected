# Esempio: unità nuova

Il Veterano romano: un'unità nuova che si recluta nella caserma di Republican Rome (tasto `V`). È più forte dell'hastatus (260 di vita,
attacco 20, armatura 14) e usa il suo modello. Ha ritratto e pulsante propri.

| File | Cos'è |
|---|---|
| `data\classes\RVeteran.sc.xml` | La classe: copia intera di quella dell'hastatus con altre statistiche. |
| `data\commands\Veterano.xml` | L'ordine di reclutarlo nella caserma (180 di oro). |
| `gameres\icons\RVeteran.bmp` | Il ritratto (65 x 76), ottenuto con `strumenti/converti-immagine.js` da `arte\veterano.png`. |
| `gameres\cmdbar\Actions\train RVeteran.bmp` | Il pulsante (153 x 51), da `arte\pulsante.png`. |
| `loc\spanish.loc.xml` | Il nome dell'unità e il testo del pulsante, in spagnolo. |

Per provarlo: copia questa cartella in `<gioco>\mods\`, attivala nel gestore dei contenuti e riavvia. Gioca con Republican Rome e cerca il nuovo
pulsante (tasto `V`) nella caserma. Cambia il gameplay: tutti i giocatori hanno la mod.

Per un'unità con sprite propri, guarda [Grafica personalizzata](../../grafica.md). Guida a questa: [Unità nuove](../../unita.md).
