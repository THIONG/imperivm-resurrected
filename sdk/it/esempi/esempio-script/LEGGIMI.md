# Esempio: script

Mostra gli eventi e i comportamenti della guida [Script](../../script.md). Attenzione: cambia l'equilibrio. Gli hastatus di tutti i giocatori fanno il doppio del danno e si curano di 25 quando uccidono un soldato. Non attivarlo in una partita seria.

Cosa c'è:

- `data\mod\hello.vs`: comportamento della classe `RHastatus`. Scrive righe nel log con `Mod_Log`, `Mod_LogInt`, `Mod_IsActive` e `Mod_ApiVersion`.
- `patches\data\classes\Military.sc.xml` e `data\mod\soldier.vs`: un comportamento nella classe base dei soldati, che ereditano tutti. Annota i primi dieci soldati che entrano nella mappa. È il modo di reagire alla comparsa di un oggetto.
- `events\OnGameStart.vs`: una riga all'inizio della partita.
- `events\OnObjectDie.vs`: una riga ogni volta che muore un soldato.
- `events\OnObjectKill.vs`: una riga per uccisione, e gli hastatus si curano di 25 quando uccidono un soldato.
- `events\OnAttackResolved.vs`: raddoppia il danno degli hastatus con `Mod_SetAttackDamage`. Annota i primi tre colpi.
- `events\OnPlayerDefeated.vs`: una riga quando un giocatore viene sconfitto.
- `events\OnSettlementCaptured.vs`: una riga quando un insediamento cambia proprietario, e conta le catture con una variabile di mod.

Per provarlo, copia questa cartella in `<gioco>\mods\`, attivala nel gestore dei contenuti e riavvia. Inizia una partita, addestra un hastatus con Republican Rome, combatti con lui e guarda `Logs\imperivm\imperivm-mod.log`: le righe con l'etichetta `script` sono di questa mod.
