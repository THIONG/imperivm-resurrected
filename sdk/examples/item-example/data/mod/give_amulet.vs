// void, Obj This

// It waits a little, as the game's scripts do, so they don't all run at the same instant
Sleep(rand(500) + 100);

// The hero is given the item if it doesn't have it yet (when a saved game loads the script starts over: this way it isn't duplicated)
if (!This.FindItem("Example Amulet").IsValid) {
	if (This.AddItem("Example Amulet")) Mod_Log("A hero received the Example Amulet");
	else Mod_Log("The hero could not receive the Example Amulet (no free slot)");
}

// A behavior script must not end
while(true) Sleep(50000);
