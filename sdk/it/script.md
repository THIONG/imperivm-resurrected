# Script

Gli script (`.vs`) sono il linguaggio con cui il gioco programma l'IA, le abilità, le condizioni di vittoria e il comportamento di edifici e unità. Una mod può portare i suoi e usare le funzioni nuove (`Mod_...`) di Imperivm Resurrected.

Esempio da copiare: [`esempi/esempio-script`](esempi/esempio-script). Attenzione: cambia il bilanciamento (gli hastatus fanno il doppio del danno e si curano quando uccidono), quindi non attivarlo in una partita seria.

## Com'è fatto uno script

Un file di testo `.vs`. La prima riga è un commento con la firma: cosa restituisce e i parametri che riceve. Ogni punto dove si usa uno script si aspetta una firma precisa. Ci sono tre eccezioni, che il gioco carica sempre con la loro firma, con la riga o senza: i comportamenti di una classe (`<behavior script="...">`), come `void, Obj This`; le modalità di vittoria (`data/GameScripts`), come `void, int player, str param`; e gli script degli oggetti (vedi [Oggetti](oggetti.md)), con la firma del loro attributo.

```
// void, Obj This

Sleep(rand(500) + 100);
Mod_Log("Ha entrado en el mapa un hastatus romano");
while(true) Sleep(50000);
```

Per vedere come scrive i suoi il gioco, scaricali con `Dump=` (vedi [Patch](patch.md); va indicato ogni file per nome, senza caratteri jolly) oppure guardali nella cartella `DATA` che estrae `estrai-dati.js`: `data/subai/*.vs` (comportamenti e abilità), `data/ai/*.vs` (IA) e `data/GameScripts/*.vs` (vittoria). Tutte le funzioni del linguaggio, con la loro firma, sono in [funciones-del-juego.txt](funzioni-del-gioco.txt).

## Dove si aggancia uno script

| Dove | Come | Firma |
|---|---|---|
| Comportamento di una classe | `<behavior script="data/mod/mi.vs"/>` nel `.sc.xml` della classe (con una [patch](patch.md) se è del gioco). Gira con ogni oggetto della classe finché esiste. | `// void, Obj This` |
| Comando o abilità | `<method sig="miOrden" vs="data/mod/mi.vs"/>` nella classe, e un `<cmd name="miOrden">` in `data\commands` | dipende dal comando: guarda quelli del gioco |
| Alla morte di un'unità | `<method sig="ondie" vs="data/mod/mi.vs"/>` nella classe (e `sig="onkill"` per quando uccide) | guarda `data/subai/unit_on_kill.vs` |
| Condizione di vittoria | un `.vs` in `data/GameScripts` (vedi [Modalità di vittoria](modalita-vittoria.md)) | `// void, int player, str param` |
| Oggetti di `items.xml` | attributi come `use_script` o `equip_script` (vedi [Oggetti](oggetti.md)) | dipende dall'attributo |

`ondie` e `onkill` sono metodi (`<method sig=...>`), non attributi. Se scrivi `ondie="..."` nella classe non succede nulla, e il gioco non avvisa.

Uno script di comportamento non deve terminare: chiudilo con `while(true) Sleep(50000);`.

Una classe ammette solo 8 `<behavior>`.

## Eventi

Metti uno script in `events\<Evento>.vs` dentro la tua mod e il gioco lo esegue quando avviene l'evento, su tutti i PC insieme. Se più mod hanno lo stesso evento, si eseguono tutti. Per provare senza creare una mod, mettilo in `<gioco>\mod\events\<Evento>.vs`.

| Evento | Quando | Firma |
|---|---|---|
| `OnGameStart` | All'inizio di una partita di schermaglia, scenario o multiplayer, subito dopo l'avvio degli script di vittoria. Può usare `Sleep` e si salva con la partita. | `// void` |
| `OnObjectDie` | Quando muore qualsiasi oggetto della mappa (unità, eroi, edifici, decorazioni…), subito prima che il gioco elabori la sua morte: l'oggetto esiste ancora. È lo stesso momento dell'`ondie` della sua classe. | `// void, Obj victim` |
| `OnObjectKill` | Quando un colpo uccide un oggetto. `killer` ha dato il colpo, `victim` è morto. Stesso momento dell'`onkill` della classe dell'attaccante. | `// void, Obj killer, Obj victim` |
| `OnSettlementCaptured` | Quando un insediamento cambia proprietario durante la partita (cattura, o uno script che lo dà a un altro). Non scatta mentre si prepara la mappa. `building` è il suo edificio centrale. Giocatori da 1 a 16: gli insediamenti senza proprietario iniziano in mano ai giocatori 15 e 16; lo 0 compare solo se non c'è proprietario. | `// void, Obj building, int oldPlayer, int newPlayer` |
| `OnAttackResolved` | A ogni colpo che fa danno, subito prima di applicarlo: contano già armatura, specialità e bonus. `damage` è ciò che subirà la vittima; `Mod_SetAttackDamage(n)` lo cambia (0 lo annulla). Non scatta se il colpo non fa danno (deviazioni, parry). | `// void, Obj attacker, Obj victim, int damage` |
| `OnPlayerDefeated` | Quando il gioco dichiara sconfitto un giocatore (da 1 a 8), subito dopo averlo segnato. Una volta per giocatore. | `// void, int player` |

Tranne `OnGameStart`, gli eventi si eseguono all'istante e non possono aspettare: niente `Sleep`. `OnAttackResolved` e `OnObjectDie` scattano molte volte al secondo in una battaglia. Filtra presto ciò che non ti interessa (`if (!victim.IsValid) return;`, `victim.IsMilitary`, `victim.player`, `victim.IsHeirOf("RHastatus")`…) e non fare lavoro costoso.

In `OnSettlementCaptured`, `building` è un `Obj`. Per arrivare al suo insediamento passa da `AsBuilding`: `building.AsBuilding.settlement`.

Con `OnObjectKill` si fanno abilità passive. Un hastatus che si cura quando uccide:

```
// void, Obj killer, Obj victim
if (!killer.IsValid) return;
if (!killer.IsHeirOf("RHastatus")) return;
if (!victim.IsMilitary) return;
killer.Heal(25);
```

Con più mod su `OnAttackResolved`, `damage` è sempre il danno originale: si calcola una volta sola. Due mod che fanno `Mod_SetAttackDamage(damage * 2)` non si accumulano. Se vuoi sommarti a ciò che ha già cambiato un'altra mod, parti da `Mod_AttackDamage()`.

### Reagire alla comparsa di un oggetto

Non c'è un evento per questo (gli oggetti vengono creati in decine di posti, anche caricando una mappa o una partita). Si fa con un comportamento: un `<behavior>` aggiunto con una patch a una classe base (`Military` per tutti i soldati, `Unit` per tutte le unità) lo ereditano tutte le classi che ne discendono, e gira una volta con ogni oggetto quando compare. Guarda `patches\data\classes\Military.sc.xml` e `data\mod\soldier.vs` nell'esempio.

## Funzioni `Mod_`

La versione attuale dell'API è la 7. `Mod_ApiVersion()` te la dà, e una mod può richiederne una minima.

| Funzione | Cosa fa |
|---|---|
| `Mod_Log(str texto)` | Scrive `script: texto` in `Logs\imperivm\imperivm-mod.log` (livello `I`). Fino a 2.000 righe per sessione. |
| `Mod_LogInt(str texto, int valor)` | Scrive `script: texto: valor`. |
| `Mod_IsActive(str id)` : `bool` | La mod con quell'`id` (quello del suo `mod.json`) è attiva nel profilo? |
| `Mod_ApiVersion()` : `int` | La versione dell'API. |
| `Mod_SetInt(str nombre, int valor)` | Salva una variabile (API 2). |
| `Mod_GetInt(str nombre)` : `int` | Legge una variabile; se non esiste vale 0 (API 2). |
| `Mod_AddInt(str nombre, int suma)` : `int` | Somma a una variabile e restituisce il valore nuovo (API 2). |
| `Mod_HasKey(str nombre)` : `bool` | La variabile esiste? (API 2). |
| `Mod_AttackDamage()` : `int` | Solo in `OnAttackResolved`: il danno del colpo che si sta risolvendo. Fuori da lì dà 0 (API 3). |
| `Mod_SetAttackDamage(int daño)` | Solo in `OnAttackResolved`: cambia il danno prima di applicarlo. Fuori da lì non fa nulla (API 3). |
| `Mod_HasSpecial(Unit unidad, int bit)` : `bool` | L'unità ha quella specialità (bit da 0 a 63)? (API 4). Vedi [Specialità](specialita.md). |
| `Mod_SetSpecial(Unit unidad, int bit, bool puesta)` | Imposta o toglie una specialità (API 4). |
| `Mod_TrueRace(Obj objeto)` : `int` | La vera civiltà di un oggetto (10 o più se è di una mod), anche se per l'IA la sua razza è quella della sua base (API 5). Vedi [Civiltà](civilta.md). |
| `Mod_GetOption(str id)` : `bool` | L'opzione `id` (quella di `options\<id>.json`) è selezionata nella stanza? Dà lo stesso valore su tutti i PC (API 6). Vedi [Opzioni di partita](opzioni.md). |
| `Mod_GetOptionValue(str id)` : `int` | Il valore scelto dell'opzione `id`: 0 o 1 in un interruttore, e il numero della voce scelta (da 0) in un elenco (API 6). |
| `Mod_PlayerCiv(int jugador)` : `int` | La civiltà che il giocatore ha scelto nella lobby (10 o più se è di una mod), anche se è una variante e `GetPlayerRace` dà la sua base. -1 se quel giocatore non esiste (API 7). Vedi [Civiltà](civilta.md). |
| `Mod_CanExecCmd(Obj objeto, str orden)` : `bool` | Il giocatore potrebbe premere ora il pulsante di quel comando su quell'oggetto? È il controllo del gioco: l'oggetto lo ha, non è disattivato (`CmdDisable`) e il suo verificatore (`groupverifier` o quello del giocatore) lo accetta. Non guarda il costo (API 7). |

Le funzioni nuove si aggiungono in fondo e quelle esistenti non cambiano. Se la tua mod ne usa qualcuna, richiede una versione di Imperivm Resurrected che la includa. La versione dell'API entra nel controllo del multiplayer quando ci sono mod di gameplay: tutti devono avere la stessa.

### Variabili delle mod

Una tabella nome → intero perché la tua mod si ricordi delle cose (contatori, segnali, tempi):

```
Mod_AddInt("capturas", 1);
if (Mod_GetInt("capturas") >= 3) ...
```

- Si salvano con la partita (anche con i salvataggi automatici) e tornano quando la carichi. Una partita nuova parte senza variabili.
- Sono uguali su tutti i PC perché gli script di partita girano uguali su tutti. Cambiale solo da script di partita, mai in base a qualcosa di locale.
- Nomi fino a 64 caratteri, fino a 1.024 variabili, interi a 32 bit.
- Sono dell'intera partita, non di un giocatore. Per una per giocatore, aggiungi il giocatore al nome (`"oro1"`, `"oro2"`).
- Sono condivise tra tutte le mod attive: usa un prefisso tuo (`"mimod_capturas"`).

## Multiplayer

Gli script fanno parte della simulazione: si eseguono su tutti i PC, nello stesso momento, e devono fare la stessa cosa su tutti.

- Leggi solo cose uguali per tutti: gli oggetti della mappa, il tempo di gioco, il `rand()` del gioco.
- Non leggere nulla di locale: la camera, il giocatore che guarda, la musica, il mouse, l'ora di sistema. Se lo script cambia la partita in base a qualcosa di locale, la partita si desincronizza.
- Se qualcosa nasce da un tasto o dall'interfaccia, deve passare come comando del gioco, che viaggia in rete. Vedi [Tasti delle mod](tasti.md).

## Partite salvate

Una partita salvata contiene il codice degli script che erano in esecuzione. Per questo le funzioni dell'SDK non cambiano numero e si aggiungono solo in fondo: ciò che è salvato continua a caricarsi. Non riusare nomi di funzioni del gioco (`Run`, `Write`…) come se fossero tuoi.

## Errori

- Uno script che non compila lascia l'errore (file, riga e colonna) in `Logs\imperivm\imperivm-mod.log`, come `script: compilation error: …` (livello `E`), e anche in `Logs\comp_out.txt` e `Logs\vxlog.log`. Il gioco va avanti, ma quello script non viene eseguito.
- Gli errori durante l'esecuzione il gioco quasi mai li registra. Negli eventi sì: il log annota `Event X (…): the script ended with an error`, fino a 5 volte per script. Per il resto, usa `Mod_Log` e guarda il log.
