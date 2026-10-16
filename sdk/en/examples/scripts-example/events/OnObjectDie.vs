// void, Obj victim

// It runs when any object on the map dies (units, buildings, decorations...), right before the game
// processes it. Like the "ondie" of the classes, it can't wait: no Sleep.

if (!victim.IsValid)
	return;

// Here we only care about soldiers
if (victim.IsMilitary)
	Mod_LogInt("OnObjectDie: ha muerto un soldado del jugador", victim.player);
