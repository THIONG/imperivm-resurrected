# Esempio: abilità delle truppe

Una nuova abilità, Respiro, per tutti i soldati da mischia (le classi che discendono da `Melee`): tasto `Y`, costa 5 di energia (dei
10 che ha un soldato) e cura 60 di vita.

| File | Cos'è |
|---|---|
| `data\commands\Respiro.xml` | L'ordine: il pulsante e il suo tasto. |
| `patches\data\classes\Melee.sc.xml` | Aggiunge il metodo `respiro` alla classe `Melee` con una patch, senza sostituire il suo file. |
| `data\mod\respiro.vs` | Lo script che la esegue. |
| `loc\spanish.loc.xml` | Il nome e la descrizione dell'ordine, con `respiro` come contesto. |
| `arte\respiro.png` | Il disegno del pulsante (51 x 51). |
| `gameres\cmdbar\Actions\Respiro.bmp` | Il pulsante nel formato del gioco, ottenuto con `strumenti/converti-immagine.js` dal PNG. |

Per provarlo: copia questa cartella in `<gioco>\mods\`, attivala nel gestore dei contenuti e riavvia. Seleziona un legionario o un guerriero e premi `Y`
(o il suo pulsante). A ogni uso viene scritto `Respiro: un soldado se ha curado` in `Logs\imperivm\imperivm-mod.log`.

Cambia il gameplay: tutti i giocatori hanno la mod. Guida: [Abilità delle truppe](../../abilita.md).
