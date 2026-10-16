// void, Obj This

// It waits a little, as the game's scripts do, so they don't all run at the same instant
Sleep(rand(500) + 100);

Mod_LogInt("Version de la API de scripts del SDK", Mod_ApiVersion());

if (Mod_IsActive("ejemplo-scripts"))
	Mod_Log("El mod ejemplo-scripts esta activo");

Mod_Log("Ha entrado en el mapa un hastatus romano");

// A behavior script must not end
while(true) Sleep(50000);
