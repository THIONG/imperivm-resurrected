// void, Obj This

// It waits a little, as the game's scripts do, so they don't all run at the same instant
Sleep(rand(500) + 100);

// The hero is given the item if it doesn't have it yet (when a saved game loads the script starts over: this way it isn't duplicated)
if (!This.FindItem("Amuleto de ejemplo").IsValid) {
	if (This.AddItem("Amuleto de ejemplo")) Mod_Log("Un heroe ha recibido el Amuleto de ejemplo");
	else Mod_Log("El heroe no ha podido recibir el Amuleto de ejemplo (sin hueco libre)");
}

// A behavior script must not end
while(true) Sleep(50000);
