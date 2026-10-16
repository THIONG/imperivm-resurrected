// void, Obj building, int oldPlayer, int newPlayer

// It runs when a settlement changes owner during the game. building is its central building; players go from 1 to 16
// (ownerless settlements belong to players 15 and 16).
Mod_LogInt("Asentamiento capturado: antes era del jugador", oldPlayer);
Mod_LogInt("Asentamiento capturado: ahora es del jugador", newPlayer);

// A variable that travels with the game: it counts captures, and keeps counting if you save and load
Mod_LogInt("Capturas de esta partida", Mod_AddInt("capturas", 1));
