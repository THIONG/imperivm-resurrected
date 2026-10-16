# Pubblicare una mod su GameBanana

Le mod di Imperivm Resurrected si pubblicano su https://gamebanana.com/games/26069. La scheda **Sfoglia contenuti** del gestore dei contenuti legge quella pagina: cerca, filtra, mostra la scheda e installa con un clic. Perché la tua mod compaia lì con il suo tipo e i suoi filtri, segui questi passi.

## Prima di caricarla

1. Scrivi la categoria e i tag nel tuo `mod.json` (quelli delle tabelle più sotto):

   ```json
   {
     "id": "duelo-en-el-nilo",
     "name": "Duelo en el Nilo",
     "version": "1.0",
     "authors": ["Tu nombre"],
     "category": "Maps",
     "tags": ["Cosmetic", "2 players", "Spanish"]
   }
   ```

2. Passala dal [validatore](validare.md). Se la categoria o un tag non sono tra quelli che il gioco riconosce, te lo dice.

3. Crea lo `.zip` della cartella della mod, con il suo `mod.json` dentro. Il pulsante **Pubblica** del gestore lo prepara in `mods\_publish\<id>-<versione>.zip` e apre la pagina di caricamento. Nello zip finisce tutto ciò che c'è nella cartella della mod (anche un `arte\` con i PNG sorgente, se l'hai lasciato). Il gestore si rifiuta di crearlo se ci sono estensioni vietate o se il `mod.json` è rotto. Quelle vietate sono eseguibili e script di sistema (`.exe .dll .asi .bat .cmd .com .scr .ps1 .vbs .js .msi .sys .cpl .lnk .reg .jar .hta .wsf .pif`): una mod che ne contiene una non si carica e non si può pubblicare.

## Su GameBanana

1. Vai su **Add → Mod** dalla pagina del gioco. Sempre su **Mod**: quello che carichi come Sound, Tool o Script non compare nel gioco. Il pulsante Pubblica del gestore apre `https://gamebanana.com/mods/add?_idGameRow=26069`.
2. **Category**: la stessa del tuo `mod.json`.
3. **Tags**: gli stessi tag.
4. **Files**: il tuo `.zip`, e nel campo **Version** la stessa versione del `mod.json`. Ogni nuova versione è un file nuovo: non sostituire il precedente, perché chi gioca online con la versione vecchia ne ha ancora bisogno.
5. **Requirements**: se la tua mod ne richiede un'altra, collega la sua pagina (`https://gamebanana.com/mods/...`). Il gioco la installa insieme.
6. Almeno uno screenshot. Il primo è l'immagine che si vede nel gioco.
7. Per le note di versione, pubblica un **Update** con la versione come titolo (per esempio `1.1`). Il gioco lo mostra accanto a quella versione.

L'avviso di nuova versione arriva solo a chi ha installato la mod da Sfoglia contenuti (questo scrive `fuentes.gamebanana` nel suo `mod.json`), o a chi ha una mod con lo stesso `id` della voce. Chi scarica lo zip a mano e lo decomprime non riceve l'avviso.

## Categorie

Una per mod: quella di ciò che la mod fa di più.

| Categoria | Cosa ci va | Dove vanno i suoi file | Esempio |
|---|---|---|---|
| **Maps** | Mappe per schermaglia e multigiocatore | `scenarios\MiMapa.bfhp` | |
| **Scenarios** | Scenari per giocatore singolo con storia e obiettivi | `scenarios\` (con i suoi script) | |
| **Campaigns** | Più scenari concatenati | `adventures\` | |
| **Conquest** | Mappe e partite di Conquista | `conquests\` | |
| **Gameplay & Balance** | Regole, statistiche, costi, IA | `data\`, patch | [esempio-patch](esempi/esempio-patch), [esempio-script](esempi/esempio-script), [ordini-propri](esempi/ordini-propri) |
| **Units & Heroes** | Unità ed eroi nuovi o modificati | `data\classes\`, `specials\` | [esempio-abilita](esempi/esempio-abilita), [esempio-specialita](esempi/esempio-specialita) |
| **Civilizations** | Civiltà nuove o rifatte | `civs\`, `data\` | [esempio-civilta](esempi/esempio-civilta) |
| **Graphics** | Texture, aspetto delle unità, terreno | i percorsi dei pacchetti del gioco | |
| **Audio & Voices** | Musica, suoni e voci | `sounds\`, `currentlang\voices\` | |
| **Interface** | Menu, icone, font, testi dell'interfaccia | `menuini\`, `loc\` | [testi-propri](esempi/testi-propri) |
| **Translations** | Il gioco o una mod in un'altra lingua | `loc\` | [lingua-francese](esempi/lingua-francese) |
| **Total Conversions** | Mod grosse che cambiano tutto il gioco | di tutto | |

Per le categorie di contenuto senza esempio (mappe, scenari, grafica, audio) basta copiare il file nella sua cartella, con lo stesso nome se sostituisce uno del gioco.

## Tag

Sono opzionali, ma sono i filtri del gioco: senza, la tua mod non compare quando si filtra.

| Filtro nel gioco | Tag |
|---|---|
| Cambia la giocabilità | `Gameplay` |
| Solo estetico | `Cosmetic` |
| Giocatori (mappe) | `2 players` … `8 players` |
| Lingua | `Spanish`, `English`, `Italian`, `German`, `French`, `Polish`, `Multilingual` |
| Verificato | `Online tested` (solo se l'hai giocata online con un'altra persona) |

Metti `Gameplay` o `Cosmetic`, uno dei due. Una mod che cambia la partita la devono avere tutti online; una estetica no (vedi [Cosa conta come gameplay?](README.md#cosa-conta-come-gameplay)).

Scrivili così come sono, in inglese: il gioco li mostra nella lingua di ogni giocatore. Il filtro cerca per sottostringa e le categorie contano anche come tag. Una mod di «Gameplay & Balance» compare filtrando «Cambia la giocabilità» anche se ha `Cosmetic`.

## Mod fatte prima di Imperivm Resurrected

Le mod di prima contengono interi pacchetti `.pak`, e quasi tutto ciò che portano sono file del gioco non modificati. Convertile in modo che contengano solo ciò che cambiano o aggiungono:

```
node strumenti/riduci-mod.js <vecchio mod> <cartella del gioco> <nuova cartella> --id mi-mod --nome "Mi mod" --versione 1.0 --categoria "Gameplay & Balance" --tag Gameplay
```

Da ogni pacchetto estrae solo i file diversi da quelli del gioco, scarta le copie `.bak` e mette i testi e i suoni di ogni lingua in `Local\<idioma>\` (o in `currentlang\` se sono uguali in tutte). Con `--percorsi` al posto della cartella di uscita mostra soltanto quali cartelle ci sono nei pacchetti, senza scrivere nulla.

- La cartella di uscita deve essere vuota.
- Se la vecchia mod ha già un `mod.json`, lo strumento lo conserva, tranne `sources` (da dove si scarica la mod originale), così gli avvisi di versione non puntano alla pagina di qualcun altro.

Provala nel gioco prima di caricarla.
