// void, Obj This

// It waits a little, as the game's scripts do, so they don't all run at the same instant
Sleep(rand(500) + 100);

Mod_LogInt("SDK script API version", Mod_ApiVersion());

if (Mod_IsActive("example-scripts"))
	Mod_Log("The example-scripts mod is active");

Mod_Log("A Roman hastatus entered the map");

// A behavior script must not end
while(true) Sleep(50000);
