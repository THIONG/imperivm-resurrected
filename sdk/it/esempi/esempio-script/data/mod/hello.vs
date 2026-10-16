// void, Obj This

// Si aspetta un po', come fanno gli script del gioco, per non eseguirsi tutti nello stesso istante
Sleep(rand(500) + 100);

Mod_LogInt("Version de la API de scripts del SDK", Mod_ApiVersion());

if (Mod_IsActive("ejemplo-scripts"))
	Mod_Log("El mod ejemplo-scripts esta activo");

Mod_Log("Ha entrado en el mapa un hastatus romano");

// Uno script di comportamento non deve terminare
while(true) Sleep(50000);
