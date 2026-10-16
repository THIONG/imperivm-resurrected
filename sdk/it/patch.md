# Patch

Una patch cambia una parte di un file del gioco senza sostituirlo per intero. Due mod possono patchare lo stesso file e le due modifiche si sommano. Se ognuna portasse la sua copia del file, la seconda sovrascriverebbe la prima.

Vanno bene file XML (comandi, oggetti, bonus, definizioni di unità), INI (costanti, interfaccia) e script `.vs` (IA, comportamenti). Il file deve esistere nel gioco. Esempio da copiare: [`esempi/esempio-patch`](esempi/esempio-patch).


## Dove va

Nella tua mod, dentro `patches`, con lo stesso percorso che ha il file nel gioco:

```
mods\mi-mod\
  mod.json
  patches\data\commands\RBarracks.xml     parchea data\commands\RBarracks.xml
  patches\data\const.ini                  parchea data\const.ini
```

- Ogni volta che il gioco apre quel file, gli vengono applicate le patch delle mod attive. Gli originali non si toccano mai.
- Con più mod, si applicano dall'ultima alla prima dell'[ordine di caricamento](riferimento.md#1-struttura-di-una-mod): la prima ha l'ultima parola. Quell'ordine lo calcola il gioco ed è lo stesso su tutti i computer con le stesse mod; il controllo del multiplayer non dipende da esso.
- Se la tua mod porta anche una sua copia del file (senza `patches`), le patch delle altre mod si applicano su quella.
- Per provare una patch senza creare una mod, mettila in `<gioco>\mod\patches\<percorso>`. Si applica per ultima, sopra tutte.
- Una patch cambia il gameplay se il file lo cambia (`data\...`, mappe, unità). Allora tutti i giocatori devono avere la stessa mod.
- Le patch si leggono quando il gioco apre ogni file. Se ne cambi una, riavvia il gioco.

## Vedere come risulta un file

Aggiungi questo a `imperivm-mod.ini` e apri il gioco:

```ini
[Debug]
Dump=data/commands/RBarracks.xml,data/const.ini
```

Ogni file viene copiato in `mod-dump\<percorso>` così come lo legge il gioco, con le patch già applicate. Per vedere l'originale, usa il profilo Originale. Scrivi il nome esatto di ogni file, separati da virgole; la riga intera non può superare i 1.023 caratteri. Un `*` da solo scarica tutti i file che il gioco apre: ne escono migliaia, usalo con cautela.

## Patch XML

Un `<patch>` con operazioni che si applicano in ordine:

```xml
<patch>
  <set select="/commands/cmd[@name='trainRHastatus']" costgold="60" execdelay="3000" unset="costpop"/>
  <add select="/commands"><cmd name="miOrden" method="train" .../></add>
  <remove select="//cmd[@name='otraOrden']"/>
  <replace select="/commands/cmd[@name='trainRArcher']/src"><src obj="RBarracks"/></replace>
</patch>
```

- `<set select="…" atributo="valor" …/>`: cambia o aggiunge quegli attributi negli elementi scelti. `unset="a b"` toglie attributi.
- `<add select="…">…</add>`: aggiunge ciò che contiene in fondo all'elemento scelto. Con `position="start"` lo mette all'inizio; con `"before"` o `"after"`, accanto all'elemento scelto invece che dentro.
- `<remove select="…"/>`: toglie gli elementi scelti.
- `<replace select="…">…</replace>`: sostituisce gli elementi scelti con ciò che contiene.

Se ciò che cerchi potrebbe non esistere e non vuoi un errore, metti `optional="yes"` nell'operazione.

### Il selettore

Un percorso come quelli delle cartelle, con condizioni tra parentesi quadre. Scrivilo sempre iniziando con `/` o `//`: altrimenti il validatore dà errore.

| Selettore | Sceglie |
|---|---|
| `/commands` | il `<commands>` radice |
| `/commands/cmd` | tutti i `<cmd>` dentro `<commands>` |
| `/commands/cmd[@name='trainRHastatus']` | il `<cmd>` con quel `name` |
| `//cmd[@name='trainRHastatus']` | quel `<cmd>`, ovunque sia |
| `/commands/cmd[@key]` | i `<cmd>` che hanno l'attributo `key` |
| `/commands/cmd[3]` | il terzo `<cmd>` |
| `/commands/cmd[@name='x']/src` | i `<src>` di quel `<cmd>` |
| `//*[@name='x']` | qualsiasi elemento con `name="x"` |

Esistono solo queste condizioni: `[@a='v']`, `[@a]` e `[n]`. Puoi metterne più di seguito (`[@a='v'][2]`). Non ci sono `and`, `contains`, `text()` né `..`. Le maiuscole non contano, e i valori vanno tra apici singoli o doppi.

### Regole

- La patch deve essere XML ben formato. Nei valori, `&` si scrive `&amp;` e `<` si scrive `&lt;`. Se è malformata non si applica per intero, e l'errore compare nel log e nella scheda della mod.
- Un'operazione che fallisce (un selettore che non trova nulla) viene saltata e segnalata. Le altre si applicano.
- La patch non cambia la codifica: usa quella del file del gioco (quasi sempre ASCII).
- Il risultato perde il rientro degli attributi (finiscono su una riga). Il contenuto è lo stesso.
- Se togli o rinomini qualcosa che usano altri file, si rompe. Le IA del gioco richiamano i comandi per nome (`trainRScout`): togli il comando e l'IA fallirà.

## Patch INI

Un INI con le sezioni e le chiavi che vuoi cambiare:

```ini
; Cambia costanti di gameplay
[GamePlay]
DamageAmplify = 250
-TeachingLevel2

; Negli elenchi di nomi singoli (senza =), + aggiunge una riga se non c'è
[GameOptions Objects]
+MiBotonNuevo
-RSText
```

- `Clave = valor` cambia o aggiunge la chiave in quella sezione.
- `-Clave` toglie la chiave (o la riga che è esattamente quel testo).
- `+Linea` aggiunge una riga singola alla sezione se non c'è già.
- Se la sezione non esiste, viene creata in fondo al file.
- Togliere una chiave che non esiste dà errore nel log.
- I commenti a fine riga del file originale (`Clave = 20 ;comentario`) si perdono nella riga che cambi.

**Non mettere commenti dopo una riga della patch.** Il motore non li toglie: `-Clave ; texto` cerca una chiave con quel nome e fallisce, `[Seccion] ; texto` non viene riconosciuta come sezione e `Clave = 250 ; texto` salva il commento dentro il valore. I commenti vanno su righe a parte che iniziano con `;`. Il validatore non lo segnala.

## Patch di script

La patch di un `.vs` (per esempio `patches\data\ai\GetTacticScript.vs`) è un testo con direttive che iniziano con `@` in prima colonna. Il codice che scrivi sotto ognuna viene aggiunto allo script del gioco:

```
@before nRace = set.GetCentralBuilding.race;
// MOD: mi civilización
if (Mod_TrueRace(set.GetCentralBuilding) == Dacia)
	return TS_GaulTactic2;

@after player = set.player;
Mod_Log("GetTacticScript: eligiendo táctica");
```

`Dacia` non è una costante del gioco: esiste se c'è una civiltà attiva con quel nome (l'`id` del suo `civs\<id>.json`). Vedi [Civiltà](civilta.md).

| Direttiva | Dove mette il codice |
|---|---|
| `@top` | Subito dopo la prima riga dello script (quella della firma). Vale solo per dichiarare variabili o commenti. |
| `@bottom` | In fondo allo script. |
| `@before TEXTO` | Prima della prima riga dello script che contiene `TEXTO`. |
| `@after TEXTO` | Dopo quella riga. |
| `@replace TEXTO` | Al posto di quella riga. |
| `@end` | Chiude il blocco precedente. È opzionale: ciò che segue fino alla direttiva successiva viene ignorato. |

- `TEXTO` è un pezzo di una riga dello script, così com'è scritta (non un'espressione regolare). Si aggancia alla prima riga che lo contiene, quindi scegli un testo che compaia una volta sola: altrimenti il tuo codice finisce dove non ti aspettavi. Se nessuna riga lo contiene, l'errore compare nel log e la direttiva viene saltata.
- Negli script le variabili si dichiarano tutte all'inizio e il codice viene dopo. Un `if` o un `return` con `@top` finisce prima delle dichiarazioni e non compila (`Unexpected starting token`). Metti il codice con `@before` e la prima riga di codice dello script.
- Le patch di più mod si sommano, quindi due mod possono cambiare lo stesso script (per esempio, due civiltà che scelgono la loro tattica in `GetTacticScript.vs`).
- Lo script risultante deve compilare. Altrimenti l'errore compare in `Logs\imperivm\imperivm-mod.log` come `script: compilation error`.
- Il validatore non controlla il codice di queste patch, né le funzioni `Mod_*` né la firma.

## Se qualcosa non va

Cerca in `Logs\imperivm\imperivm-mod.log` le righe con l'etichetta `patch`: dicono quale file è stato patchato e quale operazione è fallita. Gli errori compaiono anche nella scheda della mod. Quelli di compilazione di una patch di script compaiono come `script: compilation error`.
