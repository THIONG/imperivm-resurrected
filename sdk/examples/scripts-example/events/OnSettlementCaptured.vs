// void, Obj building, int oldPlayer, int newPlayer

// It runs when a settlement changes owner during the game. building is its central building; players go from 1 to 16
// (ownerless settlements belong to players 15 and 16).
Mod_LogInt("Settlement captured: it used to belong to player", oldPlayer);
Mod_LogInt("Settlement captured: it now belongs to player", newPlayer);

// A variable that travels with the game: it counts captures, and keeps counting if you save and load
Mod_LogInt("Captures in this game", Mod_AddInt("captures", 1));
