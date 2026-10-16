// void, Obj building, int oldPlayer, int newPlayer

// Si esegue quando un insediamento cambia proprietario durante la partita. building è il suo edificio centrale; i giocatori vanno da 1 a 16
// (gli insediamenti senza proprietario appartengono ai giocatori 15 e 16).
Mod_LogInt("Asentamiento capturado: antes era del jugador", oldPlayer);
Mod_LogInt("Asentamiento capturado: ahora es del jugador", newPlayer);

// Una variabile che viaggia con la partita: conta le conquiste, e continua a contare se salvi e carichi
Mod_LogInt("Capturas de esta partida", Mod_AddInt("capturas", 1));
