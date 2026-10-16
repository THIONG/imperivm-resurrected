# Pubblicare una mod su GameBanana

Le mod di Imperivm Resurrected si pubblicano su https://gamebanana.com/games/26069. La scheda **Sfoglia contenuti** del gestore dei contenuti legge quella pagina: cerca, filtra, mostra la scheda e installa con un clic. Perché la tua mod compaia lì con il suo tipo e i suoi filtri, segui questi passi.

## Prima di caricarla

1. Scrivi la categoria nel tuo `mod.json` (una della tabella più sotto). Se è una mappa, una campagna, una traduzione o un pacchetto audio, aggiungi anche i suoi tag:

   ```json
   {
     "id": "duelo-en-el-nilo",
     "name": "Duelo en el Nilo",
     "version": "1.0",
     "authors": ["Il tuo nome"],
     "category": "Maps",
     "tags": ["2 players"]
   }
   ```

2. Passala dal [validatore](validare.md). Ti avvisa se la categoria non è tra quelle che il gioco riconosce o ha un nome vecchio, se a una mappa mancano i giocatori e se a una campagna, traduzione o audio manca la lingua.

3. Crea lo `.zip` della cartella della mod, con il suo `mod.json` dentro. Non metterci eseguibili né script di sistema (`.exe .dll .asi .bat .cmd .com .scr .ps1 .vbs .js .msi .sys .cpl .lnk .reg .jar .hta .wsf .pif`): una mod che ne contiene uno non si carica.

## Su GameBanana

1. Vai su **Add → Mod** dalla pagina del gioco (`https://gamebanana.com/mods/add?_idGameRow=26069`). Sempre su **Mod**: quello che carichi come Sound, Tool o Script non compare nel gioco.
2. **Category**: la stessa del tuo `mod.json`.
3. **Tags**: gli stessi tag, scritti allo stesso modo.
4. **Files**: il tuo `.zip`, e nel campo **Version** la stessa versione del `mod.json`. Ogni nuova versione è un file nuovo: non sostituire il precedente, perché chi gioca online con la versione vecchia ne ha ancora bisogno.
5. **Requirements**: se la tua mod ne richiede un'altra, collega la sua pagina (`https://gamebanana.com/mods/...`). Il gioco la installa insieme.
6. Almeno uno screenshot. Il primo è l'immagine che si vede nel gioco.
7. Per le note di versione, pubblica un **Update** con la versione come titolo (per esempio `1.1`). Il gioco lo mostra accanto a quella versione.

L'avviso di nuova versione arriva solo a chi ha installato la mod da Sfoglia contenuti (questo scrive `sources.gamebanana` nel suo `mod.json`), o a chi ha una mod con lo stesso `id` della voce. Chi scarica lo zip a mano e lo decomprime non riceve l'avviso.

## Categorie

Una per mod: quella di ciò che la mod fa di più. Sono nove, nei tre tipi che il gioco distingue. Il tipo non lo decide la categoria ma i file della mod (vedi [Cosa conta come gameplay?](README.md#cosa-conta-come-gameplay)); la categoria serve a trovarla in Sfoglia contenuti.

| Categoria | Nel gioco | Cosa ci va | Dove vanno i suoi file | Esempio |
|---|---|---|---|---|
| **Cambiano il gameplay** | | *Online devono averle tutti* | | |
| **Gameplay & Balance** | Regole e bilanciamento | Regole, statistiche, costi, IA; anche le mod grosse che cambiano tutto il gioco | `data\`, patch | [esempio-patch](esempi/esempio-patch), [esempio-script](esempi/esempio-script), [ordini-propri](esempi/ordini-propri) |
| **Units & Heroes** | Unità ed eroi | Unità ed eroi nuovi o modificati | `data\classes\`, `specials\` | [esempio-abilita](esempi/esempio-abilita), [esempio-specialita](esempi/esempio-specialita) |
| **Civilizations** | Civiltà | Civiltà nuove o rifatte | `civs\`, `data\` | [esempio-civilta](esempi/esempio-civilta) |
| **Contenuti da giocare** | | *Servono solo a chi li gioca* | | |
| **Maps** | Mappe | Mappe per schermaglia e multigiocatore | `scenarios\MiMapa.bfhp` | [esempio-mappa-casuale](esempi/esempio-mappa-casuale) |
| **Campaigns** | Battaglie e campagne | Battaglie per giocatore singolo con storia e obiettivi, singole o concatenate | `adventures\` | |
| **Conquest** | Conquista | Mappe e partite di Conquista | `conquests\` | |
| **Estetiche** | | *Ogni giocatore ha le sue* | | |
| **Graphics** | Grafica e interfaccia | Texture, aspetto delle unità, terreno, menu, icone, font | i percorsi dei pacchetti del gioco, `menuini\` | |
| **Audio & Voices** | Audio e voci | Musica, suoni e voci | `sounds\`, `currentlang\voices\` | |
| **Translations** | Testi e traduzioni | Il gioco o una mod in un'altra lingua, testi cambiati | `loc\` | [lingua-francese](esempi/lingua-francese), [testi-propri](esempi/testi-propri) |

Per le categorie senza esempio (mappe, campagne, grafica, audio) basta copiare il file nella sua cartella, con lo stesso nome se sostituisce uno del gioco.

**Nomi vecchi.** Su GameBanana esiste ancora la categoria **Scenarios**: non usarla (il gioco la mostra come Mappe). Un `mod.json` con `Scenarios`, `Interface` o `Total Conversions` funziona ancora come Maps, Graphics e Gameplay & Balance, ma il validatore ti chiede il nome nuovo.

## Tag

Servono solo in due casi: sono i filtri di giocatori e lingua di Sfoglia contenuti.

| Categoria | Tag | Filtro nel gioco |
|---|---|---|
| Maps | `4 players`, o un intervallo come `2-4 players` (da 2 a 8) | Giocatori |
| Campaigns, Audio & Voices, Translations | `Spanish`, `English`, `Italian`, `German`, `French`, `Polish` o `Multilingual` (puoi metterne più di uno) | Lingua |

Scrivili così come sono, in inglese, nei `tags` del tuo `mod.json` e nel campo **Tags** di GameBanana: il gioco li mostra nella lingua di ogni giocatore. Capisce anche altre forme comuni (`2v2`, `4 giocatori`, `Italiano`), ma così vai sul sicuro. Una mappa senza tag dei giocatori non compare filtrando per giocatori, e una campagna, traduzione o audio senza lingua non compare filtrando per lingua.

I tag `Gameplay`, `Cosmetic` e `Online tested` delle versioni precedenti non servono più: se una mod cambia il gameplay, il gioco lo capisce dai suoi file.

## Mod fatte prima di Imperivm Resurrected

Le mod di prima contengono interi pacchetti `.pak`, e quasi tutto ciò che portano sono file del gioco non modificati. Convertile in modo che contengano solo ciò che cambiano o aggiungono:

```
node strumenti/riduci-mod.js <vecchio mod> <cartella del gioco> <nuova cartella> --id mi-mod --nome "Mi mod" --versione 1.0 --categoria "Gameplay & Balance"
```

Da ogni pacchetto estrae solo i file diversi da quelli del gioco, scarta le copie `.bak` e mette i testi e i suoni di ogni lingua in `Local\<idioma>\` (o in `currentlang\` se sono uguali in tutte). Con `--percorsi` al posto della cartella di uscita mostra soltanto quali cartelle ci sono nei pacchetti, senza scrivere nulla.

- La cartella di uscita deve essere vuota.
- Se la vecchia mod ha già un `mod.json`, lo strumento lo conserva, tranne `sources` (da dove si scarica la mod originale), così gli avvisi di versione non puntano alla pagina di qualcun altro.

Provala nel gioco prima di caricarla.
