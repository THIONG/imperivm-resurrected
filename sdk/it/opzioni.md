# Opzioni di partita personalizzate

Una mod può aggiungere **interruttori** (sì / no) e **liste di valori** (un menu a tendina) all'elenco delle REGOLE della lobby, accanto a "Nebbia di guerra" o "Blocca la diplomazia". L'host le sceglie, tutti le vedono, e i tuoi script leggono la scelta con `Mod_GetOption("id")` (interruttore) o `Mod_GetOptionValue("id")` (lista). Entrambe fanno parte dell'API 6 degli script.

Esempio: [`esempi/esempio-opzioni`](esempi/esempio-opzioni) (interruttore "Héroes veloces" e lista "Velocidad de héroes").

## Via rapida

`crea-opzione.js` scrive il JSON e, se vuoi, lo scheletro dello script. Non richiede la cartella `DATA`.

```
node strumenti/crea-opzione.js mods/mi-mod heroes-veloces --nome "Héroes veloces" --nome-en "Fast heroes" --descrizione "Todos los héroes se mueven un 50 % más rápido."
node strumenti/crea-opzione.js mods/mi-mod velocidad-heroes --nome "Velocidad de héroes" --valori "Normal,Rápida,Muy rápida" --predefinito Normal --script
```

Il primo comando crea un interruttore; con `--valori` (da 2 a 16, separati da virgole) crea una lista.

| Opzione | A cosa serve |
|---|---|
| `--nome` | Obbligatorio. Quello che si vede nella lobby |
| `--nome-en`, `--nome-it` | Il nome in inglese e in italiano |
| `--descrizione`, `--descrizione-en`, `--descrizione-it` | L'aiuto della lobby |
| `--valori`, `--valori-en`, `--valori-it` | I valori di una lista, nello stesso ordine e nello stesso numero. L'id di ogni valore deriva dal suo testo |
| `--predefinito` | Interruttore: `true` o `false`. Lista: il numero del valore (da 0), il suo id o il suo testo. Senza, `false` o il primo |
| `--script` | Crea anche `data\mod\<id>.vs`, un comportamento con l'`if (Mod_GetOption("<id>"))` (o `Mod_GetOptionValue`) pronto da completare. Aggancialo a una classe con una patch, come nell'esempio |

Genera `options\<id>.json` e, con `--script`, `data\mod\<id>.vs`. Dice quanti degli 8 bit consuma e quanti ne usa già la mod. Non sovrascrive niente: con un id ripetuto nella mod, si ferma senza scrivere nulla.

## Come si fa a mano

1. Un file `options\<id>.json` nella tua mod.

   Un interruttore:

   ```json
   {
     "id": "heroes-veloces",
     "name": { "spanish": "Héroes veloces", "english": "Fast heroes" },
     "description": { "spanish": "Todos los héroes se mueven un 50 % más rápido.", "english": "All heroes move 50% faster." },
     "default": false
   }
   ```

   Una lista (da 2 a 16 valori):

   ```json
   {
     "id": "velocidad-heroes",
     "name": { "spanish": "Velocidad de héroes", "english": "Hero speed" },
     "values": [
       { "id": "normal", "name": { "spanish": "Normal", "english": "Normal" } },
       { "id": "rapida", "name": { "spanish": "Rápida (+25 %)", "english": "Fast (+25%)" } },
       "Muy rápida"
     ],
     "default": "normal"
   }
   ```

   - `id`: lettere, numeri, `_` e `-`, fino a 32. È quello che usano gli script e non distingue maiuscole e minuscole. Se manca, si usa il nome del file.
   - `name` e `description`: un testo, oppure uno per lingua (quella del gioco e, se manca, l'inglese).
   - `values`: ognuno è un testo o un oggetto con `id` e `name`. Il primo è lo 0.
   - `default`: in un interruttore, `true` o `false`; in una lista, il numero del valore (da 0) o il suo `id`. Se metti un `id` che non esiste, si prende lo 0 senza avvisare; `valida-mod.js` invece lo rileva.

2. Uno script che la legga (un comportamento di classe, un evento o uno script di vittoria):

   ```
   if (Mod_GetOption("heroes-veloces")) {
       This.AsUnit().SetSpeedFactor(150);
   }
   if (Mod_GetOptionValue("velocidad-heroes") == 1) {
       This.AsUnit().SetSpeedFactor(125);
   }
   ```

   `Mod_GetOption` dà `true` se il valore non è 0 (anche con una lista). `Mod_GetOptionValue` dà il numero del valore scelto.

## Limiti

- **Ci sono 8 bit per tutte le opzioni di tutte le mod attive.** Un interruttore ne consuma 1; una lista di 2 valori ne consuma 1, di 3 o 4 ne consuma 2, di 5-8 ne consuma 3 e di 9-16 ne consuma 4. Entrano in ordine alfabetico di `id`; quello che non ci sta viene ignorato e il log lo segnala. Se due opzioni hanno lo stesso `id`, vale la prima che il gioco trova quando monta le mod.
- **Funzionano solo con la lobby.** Le opzioni si fissano all'inizio di una partita dalla lobby (o quando si rientra in una online). Nelle avventure, nelle campagne e nei tutorial non c'è lobby: `Mod_GetOption` dà sempre 0 e la lista vale 0, anche se metti `"default": true`.
- I preset della lobby (Classico, Competitivo) non toccano le opzioni delle mod.

## Online

È gameplay: tutti i giocatori devono avere la stessa mod. L'opzione viaggia con le altre regole della lobby e viene salvata con la partita.
