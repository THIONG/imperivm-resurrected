# Lingue e testi

Una mod può cambiare o aggiungere testi del gioco senza sostituire l'intero file dei testi, e può aggiungere una lingua nuova (francese, tedesco, polacco…) che compare in **Opzioni → Lingua**. Può anche portare file che si usano solo con una lingua: aiuto, consigli, immagini del menu, voci.

Esempi da copiare: [`esempi/testi-propri`](esempi/testi-propri) e [`esempi/lingua-francese`](esempi/lingua-francese).

## Come funziona la lingua del gioco

La lingua si sceglie in Opzioni → Lingua e si applica al riavvio. Si salva in `Settings.ini`, `[Language] Default=`.

I testi del gioco stanno in `currentlang/game.loc.xml`, dentro `local\<idioma>.pak`. Ogni testo è una riga così:

```xml
<translationtableentry text="Single player@/Menu/mainmenu.ini:SinglePlayer:Text" justtext="Single player" result="Estrategia"/>
```

`text` è la chiave: il testo originale in inglese, a volte seguito da `@` e da un contesto (dove si usa). `result` è ciò che si vede in quella lingua. Se un testo non ha traduzione, il gioco mostra la chiave, cioè l'inglese. Per questo l'inglese non ha quasi bisogno di un file.

L'interfaccia di Imperivm Resurrected stesso (gestore dei contenuti, stanza, Controlli, partite salvate, pannello prestazioni) esiste in spagnolo, inglese e italiano. Con il gioco in italiano compare in italiano; con qualsiasi altra lingua, in inglese. I testi della tua mod (nome, descrizione, comandi) si traducono invece con i file `loc\` più sotto.

Filmati e musica sono uguali in tutte le lingue. Cambiano i testi, le voci delle unità, l'aiuto, i consigli e alcune immagini del menu.

Per vedere il file dei testi, aggiungi questo a `imperivm-mod.ini` e apri il gioco:

```ini
[Debug]
Dump=currentlang/game.loc.xml
```

Viene copiato in `mod-dump\currentlang\game.loc.xml` con i testi delle tue mod già sommati. Per vedere l'originale, usa il profilo Originale.

## Cambiare o aggiungere testi

Metti i testi in `loc\<idioma>.loc.xml` dentro la tua mod. Se sono molti, distribuiscili in più `*.loc.xml` dentro `loc\<idioma>\`:

```
mods\mi-mod\
  mod.json
  loc\spanish.loc.xml
  loc\italian.loc.xml
  loc\english\unidades.loc.xml
```

`<idioma>` è il nome della lingua in inglese e minuscolo, come in `Settings.ini`: `spanish`, `english`, `italian`, `french`, `german`…

```xml
<translationtable>
  <!-- Cambia il testo in tutti i punti dove compare -->
  <translationtableentry text="Single player" result="Un jugador"/>

  <!-- Solo in un punto: la chiave completa, copiata dal game.loc.xml del gioco -->
  <translationtableentry text="Options@/Menu/endgamemenu.ini:Options:Text" result="Ajustes"/>

  <!-- Lo stesso scritto con context -->
  <translationtableentry text="Options" context="/Menu/endgamemenu.ini:Options:Text" result="Ajustes"/>

  <!-- Testo nuovo, p. es. il nome di un'unità della tua mod (display_name nel suo .sc.xml) -->
  <translationtableentry text="Gladiator" result="Gladiador"/>
</translationtable>
```

- Senza contesto (`text="Single player"`), la modifica vale in tutti i punti dove il gioco usa quel testo. Con il contesto (`text="…@…"` o l'attributo `context`), solo in quel punto.
- A capo: `\n` dentro il testo, come nei file del gioco.
- In un testo, `&` si scrive `&amp;`, `<` si scrive `&lt;` e le virgolette doppie `&quot;` (oppure usa apici singoli per il valore).
- Salva in UTF-8. Se è in ANSI, la mod lo converte, ma UTF-8 è la scelta sicura.
- Una riga scritta male viene saltata e annotata in `Logs\imperivm\imperivm-mod.log` con il suo numero di riga. Il resto del file si carica.
- `<gioco>\mod\loc\<idioma>.loc.xml` serve per provare senza creare una mod, e vince su tutte le mod.
- Se due mod cambiano lo stesso testo, dovrebbe vincere quella che viene prima nell'[ordine di caricamento](riferimento.md#1-struttura-di-una-mod), come per i file.
- I testi si leggono all'avvio: riavvia dopo averli cambiati.

I nomi di unità, edifici e comandi della tua mod si scrivono in inglese nei loro file (`display_name`, `rollover`…). Quella è la chiave. Per tradurli, aggiungi la traduzione in `loc\spanish.loc.xml`, `loc\italian.loc.xml`, ecc.

## Aggiungere una lingua nuova

1. Dichiara la lingua in `mod.json`:

   ```json
   "language": {"code": "french", "name": "Français", "base": "spanish"}
   ```

   - `code`: il nome della lingua in inglese e minuscolo. È quello che si salva in `Settings.ini`.
   - `name`: ciò che si vede nel selettore.
   - `base` (opzionale, di default `english`): la lingua da cui si prende ciò che la tua mod non porta. Leggi l'avviso più sotto prima di lasciare l'inglese.

2. Traduci i testi in `loc\french.loc.xml` (o in più file dentro `loc\french\`). Ciò che non traduci compare in inglese.

3. Opzionale: file della lingua in `Local\french\`, con i percorsi che hanno dentro `currentlang\`. Quelli che chiede il gioco sono:

   ```
   Local\french\help.xml              ayuda (F1)
   Local\french\tips.xml              consejos de la pantalla de carga
   Local\french\credits.txt           créditos
   Local\french\splash.ini            pantallas de inicio
   Local\french\menu_16_9.bmp         fondo del menú (también menu_4_3.bmp, menubackground.bmp, logo.bmp)
   Local\french\voices\<unita>\1.wav voci delle unità (da 1.wav a 5.wav, a seconda dell'unità)
   ```

   Puoi anche impacchettare tutto in `Local\french.pak`.

4. Attiva la mod nel tuo profilo, scegli **Français** in Opzioni → Lingua e riavvia.

**Cosa porta la lingua base.** Ciò che la tua lingua non porta si prende dalla lingua `base`. L'inglese non ha aiuto (`help.xml`), consigli (`tips.xml`), `splash.ini` né immagini del menu. Una lingua nuova con base inglese resta senza questi file. Lo spagnolo e l'italiano li hanno: con `"base": "spanish"` o `"italian"` li ottieni, oppure li porti tu in `Local\<idioma>\`.

Le mappe portano i propri testi per lingua. Se una mappa non ha il tuo, si usano quelli della lingua base.

**Caratteri.** I font del gioco dovrebbero coprire latino (con gli accenti europei), greco e cirillico. Cinese, giapponese e coreano non si possono mostrare senza font nuovi.

Nel multiplayer la lingua non conta: giocatori con lingue diverse possono giocare insieme.

## File per una sola lingua

Qualsiasi file di `currentlang\` si può cambiare per una sola lingua con la cartella `Local\<idioma>\` della tua mod. `Local\spanish\help.xml` cambia l'aiuto solo in spagnolo. Se metti il file in `currentlang\help.xml` (fuori da `Local\`), vale per tutte le lingue.

## Se qualcosa non va

Cerca in `Logs\imperivm\imperivm-mod.log` le righe con l'etichetta `language`. Quelle di avviso (`W`) indicano le righe con errori. Per vedere anche quali file di testi sono stati letti e quali file di `Local\<idioma>\` si usano, aggiungi `LogChannels=language:debug` in `[Debug]` di `imperivm-mod.ini`.
