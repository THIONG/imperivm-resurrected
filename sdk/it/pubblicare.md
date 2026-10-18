# Pubblicare una mod su GameBanana

Le mod di Imperivm Resurrected si pubblicano su https://gamebanana.com/games/26069. La scheda **Sfoglia contenuti** del gestore dei contenuti legge quella pagina: cerca, filtra, mostra la scheda e installa con un clic. Perché la tua mod compaia lì con il suo tipo e i suoi filtri, segui questi passi.

## Prima di caricarla

1. Scrivi la categoria nel tuo `mod.json` (una della tabella più sotto) e, se serve, i suoi [tag](#tag): altre categorie se la tua mod fa più cose, i giocatori se è uno scenario e la lingua se è un'avventura, una traduzione o un pacchetto audio:

   ```json
   {
     "id": "duelo-en-el-nilo",
     "name": "Duelo en el Nilo",
     "version": "1.0",
     "authors": ["Il tuo nome"],
     "category": "Scenarios",
     "tags": ["Players: 2"]
   }
   ```

2. Passala dal [validatore](validare.md). Ti avvisa se la categoria non è tra quelle che il gioco riconosce o ha un nome vecchio, se un tag è scritto male, se a uno scenario mancano i giocatori e se a un'avventura, traduzione o audio manca la lingua.

3. Crea lo `.zip` della cartella della mod, con il suo `mod.json` dentro. Non metterci eseguibili né script di sistema (`.exe .dll .asi .bat .cmd .com .scr .ps1 .vbs .js .msi .sys .cpl .lnk .reg .jar .hta .wsf .pif`): una mod che ne contiene uno non si carica.

## Su GameBanana

1. Vai su **Add → Mod** dalla pagina del gioco (`https://gamebanana.com/mods/add?_idGameRow=26069`). L'audio è l'eccezione: va su **Add → Sound** (`https://gamebanana.com/sounds/add?_idGameRow=26069`). Quello che carichi come Tool o Script non compare nel gioco.
2. **Category**: la stessa del tuo `mod.json`.
3. **Tags**: gli stessi tag. Ognuno ha un titolo e un valore: in `Type: Civilizations` il titolo è `Type` e il valore `Civilizations`. Non bisogna crearli prima: si scrivono in ogni mod.
4. **Files**: il tuo `.zip`, e nel campo **Version** la stessa versione del `mod.json`. Ogni nuova versione è un file nuovo: non sostituire il precedente, perché chi gioca online con la versione vecchia ne ha ancora bisogno.
5. **Requirements**: se la tua mod ne richiede un'altra, collega la sua pagina (`https://gamebanana.com/mods/...`, o `https://gamebanana.com/sounds/...` se è audio). Il gioco la installa insieme.
6. Almeno uno screenshot. Il primo è l'immagine che si vede nel gioco.
7. Per le note di versione, pubblica un **Update** con la versione come titolo (per esempio `1.1`). Il gioco lo mostra accanto a quella versione.

L'avviso di nuova versione arriva solo a chi ha installato la mod da Sfoglia contenuti (questo scrive `sources.gamebanana` nel suo `mod.json`, o `sources.gamebananaSound` se viene da Sounds), o a chi ha una mod con lo stesso `id` della voce. Chi scarica lo zip a mano e lo decomprime non riceve l'avviso.

## Categorie

**In breve:** su GameBanana scegli **una categoria**, quella che descrive meglio la mod nel complesso. Tutto il resto che porta va nei **tag** `Type`, `Players` e `Language`: metti tutti quelli che le corrispondono, e solo quelli.

GameBanana ne consente una sola per mod: scegli quella di ciò che la mod fa di più. Se la tua mod fa altre cose, per esempio una grande che porta civiltà e mappe, aggiungi le altre con tag `Type` (vedi [Tag](#tag)) e comparirà in tutte. Sono nove, nei tre tipi che il gioco distingue. Il tipo non lo decide la categoria ma i file della mod (vedi [Cosa conta come gameplay?](README.md#cosa-conta-come-gameplay)); la categoria serve a trovarla in Sfoglia contenuti.

| Categoria | Nel gioco | Cosa ci va | Dove vanno i suoi file | Esempio |
|---|---|---|---|---|
| **Cambiano il gameplay** | | *Online devono averle tutti* | | |
| **Gameplay & Balance** | Gameplay e bilanciamento | Regole, statistiche, costi, IA; anche le mod grosse che cambiano tutto il gioco | `data\`, patch | [esempio-patch](esempi/esempio-patch), [esempio-script](esempi/esempio-script), [ordini-propri](esempi/ordini-propri) |
| **Units & Heroes** | Unità ed eroi | Unità ed eroi nuovi o modificati | `data\classes\`, `specials\` | [esempio-abilita](esempi/esempio-abilita), [esempio-specialita](esempi/esempio-specialita) |
| **Civilizations** | Civiltà | Civiltà nuove o rifatte | `civs\`, `data\` | [esempio-civilta](esempi/esempio-civilta) |
| **Contenuti da giocare** | | *Servono solo a chi li gioca* | | |
| **Scenarios** | Scenari | Mappe per schermaglia e multigiocatore | `scenarios\MiMapa.bfhp` | [esempio-mappa-casuale](esempi/esempio-mappa-casuale) |
| **Adventures** | Avventure | Grandi battaglie, sfide e campagne per giocatore singolo, con storia e obiettivi | `adventures\` | |
| **Conquests** | Conquiste | Mappe e partite di Conquista | `conquests\` | |
| **Estetiche** | | *Ogni giocatore ha le sue* | | |
| **Graphics** | Grafica | Texture, aspetto delle unità, terreno, menu, icone, font | i percorsi dei pacchetti del gioco, `menuini\` | |
| **Audio & Voices** | Audio e voci | Musica, suoni e voci. Si pubblica nella sezione **Sounds** di GameBanana, non in Mods | `sounds\`, `currentlang\voices\` | |
| **Translations** | Traduzioni | Il gioco o una mod in un'altra lingua, testi cambiati | `loc\` | [lingua-francese](esempi/lingua-francese), [testi-propri](esempi/testi-propri) |

Per le categorie senza esempio (avventure, conquiste, grafica, audio) basta copiare il file nella sua cartella, con lo stesso nome se sostituisce uno del gioco.

**Mappe sciolte.** Se lo `.zip` contiene solo il `.bfhp`, senza cartelle, il gioco lo sistema da solo durante l'installazione secondo il tipo indicato dalla mappa stessa (quello scelto dal suo autore nell'editor): un'avventura va in `adventures\` (con la sua `.bmp` dello stesso nome), una conquista in `conquests\` e una mappa da schermaglia in `scenarios\`, oppure in `adventures\` se la categoria è Adventures (missioni per giocatore singolo fatte come mappa normale). In ogni caso è meglio caricarla già nella sua cartella; il [validatore](validare.md) avvisa se una mappa è in quella sbagliata, per esempio un'avventura in `scenarios\`, che la lista degli Scenari non mostra.

**Nomi vecchi.** Scenari, Avventure e Conquiste si chiamano come le cartelle del gioco (`scenarios\`, `adventures\`, `conquests\`). Prima erano **Maps**, **Campaigns** e **Conquest**: un `mod.json` con quei nomi, o con `Interface` o `Total Conversions`, funziona ancora, ma il validatore ti chiede il nome nuovo.

## Tag

Ogni tag ha un titolo e un valore. Il gioco ne usa tre:

| Tag | A cosa serve | Esempio |
|---|---|---|
| `Type: <categoria>` | Altre categorie, oltre a quella principale. Un tag per categoria | `Type: Civilizations`, `Type: Scenarios` |
| `Players: <giocatori>` | Giocatori di uno scenario, da 2 a 8. Puoi metterne più di uno | `Players: 4`, `Players: 2-4` |
| `Language: <lingua>` | Lingua di un'avventura, una traduzione o un pacchetto audio: `Spanish`, `English`, `Italian`, `German`, `French`, `Polish` o `Multilingual`. Puoi metterne più di uno | `Language: Italian` |

Scrivili così come sono, in inglese, nei `tags` del tuo `mod.json` (per esempio `"tags": ["Type: Scenarios", "Players: 2-4"]`) e nel campo **Tags** di GameBanana: il gioco li mostra nella lingua di ogni giocatore. Metti solo questi tre: il gioco non ne usa altri, e il validatore ti avvisa se ce n'è qualcuno in più.

Uno scenario senza `Players` non compare filtrando per giocatori, e un'avventura, traduzione o audio senza `Language` non compare filtrando per lingua. Lo stesso vale per le categorie di `Type`: una mod con `Type: Scenarios` ha bisogno di `Players` per comparire filtrando per giocatori.

Le forme senza titolo delle versioni precedenti (`4 players`, `Spanish`) funzionano ancora, ma il validatore chiede quella nuova. I tag `Gameplay`, `Cosmetic` e `Online tested` non servono più: se una mod cambia il gameplay, il gioco lo capisce dai suoi file.

## Mod fatte prima di Imperivm Resurrected

Le mod di prima si installavano copiandole sopra il gioco: contengono interi pacchetti `.pak` che sostituiscono quelli del gioco, e quasi tutto ciò che portano sono file del gioco non modificati. Convertile in modo che contengano solo ciò che cambiano o aggiungono:

```
node strumenti/riduci-mod.js <vecchio mod> <cartella del gioco> <nuova cartella> --id mi-mod --nome "Mi mod" --versione 1.0 --categoria "Gameplay & Balance"
```

Lo strumento calcola cosa vede il gioco con la mod copiata sopra e cosa vede senza, e tiene solo la differenza:

- Dai pacchetti, i file diversi da quelli del gioco, rispettando il loro ordine (in una cartella vince il pacchetto con il nome maggiore, come `zzMod.pak` su `Data.pak`).
- I testi e i suoni di ogni lingua in `Local\<lingua>\` (o in `currentlang\` se sono uguali in tutte), e il resto di un pacchetto di lingua in `Local\<lingua>.pak`.
- Ciò che la mod vecchia toglieva dal gioco sostituendo i suoi pacchetti, in `hidden.txt`, perché neanche il gioco lo veda.
- `data\const.ini` come patch e senza la sezione `[Resolutions]`: la risoluzione la sceglie il giocatore nelle opzioni.
- Se la mod porta un suo `rle.mmp`, solo ciò che cambia o aggiunge a quello del gioco, in `rle.mmp.tail`; quello del gioco non si tocca.
- Lascia fuori le copie `.bak`, `Config.ini` e `Settings.ini` (configurazione dell'installazione) e gli eseguibili, con i quali la mod non si caricherebbe.

Opzioni:

- `--escludi <schemi>` lascia fuori altri file della mod vecchia, come le guide d'installazione (percorsi con `*` e `?`, separati da virgole).
- `--nascondi <schemi>` nasconde file sciolti del gioco che il leggimi della mod vecchia diceva di cancellare (per esempio `currentlang/*,units/*`).
- `--percorsi`, al posto della cartella di output, mostra solo quali cartelle ci sono dentro i pacchetti, senza scrivere niente.

La cartella di output deve essere vuota. Se la mod vecchia ha già un `mod.json`, lo strumento lo conserva, tranne `sources` (da dove si scarica la mod originale), così gli avvisi di versione non puntano alla pagina di un altro. Alla fine dice cosa ha lasciato fuori e cosa nasconde.

Provala nel gioco prima di caricarla.
