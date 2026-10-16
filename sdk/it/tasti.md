# Tasti delle mod

Il giocatore cambia i tasti del gioco in **Opzioni → Controlli**. I comandi della tua mod compaiono lì da soli, purché abbiano un nome: non devi registrare niente.

Esempio da copiare: [`esempi/ordini-propri`](esempi/ordini-propri).

## Come compaiono i tuoi comandi

Un comando è un `<cmd>` di un file di `data\commands`. Il suo tasto di fabbrica è l'attributo `key` (`key="j"`; con `ctrl="1"` serve Ctrl). Il giocatore può cambiarlo, e la modifica si salva in `controls.ini`, non nella tua mod.

Il comando ha bisogno di `rollover`: è il suo nome nell'elenco. Senza `rollover` non compare in Controlli.

- Se il comando sta in un file del gioco che la tua mod modifica con una patch (per esempio `RBarracks.xml`), compare nella scheda di quell'edificio o unità, insieme agli altri. Eccezione: `Stonehenge.xml` non è nella tabella dei file noti, quindi un comando aggiunto lì compare sotto «Edifici» con il nome del file.
- Se la tua mod porta un nuovo file di comandi (per esempio `data\commands\MiaCaserma.xml`), compare nella scheda **Contenuti**, in una sezione con il nome della tua mod. Con più file di comandi, ognuno ha la sua sezione.

La scheda **Contenuti** compare solo se c'è almeno una mod attiva con comandi nuovi.

## Dare un nome alla sezione

Di default la sezione si chiama come la tua mod (più il nome del file, se ne hai più d'uno). Per dare nomi diversi per lingua, usa `commands` nel `mod.json`, con il nome del file senza estensione:

```json
"commands": {
  "MiaCaserma": {
    "name": {"spanish": "Cuartel romano extra", "english": "Extra Roman barracks", "italian": "Caserma romana extra"}
  }
}
```

Compare il nome nella lingua del gioco; se manca, l'inglese; se manca anche quello, il primo. Va bene anche un testo solo (`"name": "Cuartel extra"`). Sullo schermo: `Nome della mod · Caserma romana extra`.

Il nome di ogni comando è il suo `rollover`, e si traduce come qualsiasi altro testo: vedi [Lingue e testi](lingue.md).

## Quali tasti si possono usare

Una lettera (A–Z) o un simbolo (punto e virgola, uguale, virgola, trattino, punto, barra, barra rovesciata, parentesi quadre, apostrofo e accento grave), con Ctrl se vuoi. I numeri non vanno: i comandi accettano solo lettere e simboli, quindi un `key="1"` non diventa un tasto in Controlli. Non vanno nemmeno Maiusc e Alt, solo Ctrl.

Il giocatore può usare anche la modalità **Per posizione nella barra** (Q il primo pulsante, W il secondo…). I tuoi comandi ci entrano come tutti gli altri.

## Cosa non si può fare

- Un tasto che faccia qualcosa che non sia un comando del gioco (cambiare un'opzione della mod, lanciare uno script isolato) non esiste. Ogni azione con un tasto deve essere un comando.
- Un file di comandi fuori da `data\commands` non viene caricato.
