// void, Obj building, int oldPlayer, int newPlayer

// Se ejecuta al cambiar de dueño un asentamiento durante la partida. building es su edificio central; los jugadores van de 1 a 16
// (los asentamientos sin dueño son de los jugadores 15 y 16).
Mod_LogInt("Asentamiento capturado: antes era del jugador", oldPlayer);
Mod_LogInt("Asentamiento capturado: ahora es del jugador", newPlayer);

// Una variable que viaja con la partida: cuenta las capturas, y sigue contando si guardas y cargas
Mod_LogInt("Capturas de esta partida", Mod_AddInt("capturas", 1));
