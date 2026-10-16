// void, Obj This

// Se espera un poco, como hacen los scripts del juego, para no ejecutarse todos en el mismo instante
Sleep(rand(500) + 100);

Mod_LogInt("Version de la API de scripts del SDK", Mod_ApiVersion());

if (Mod_IsActive("ejemplo-scripts"))
	Mod_Log("El mod ejemplo-scripts esta activo");

Mod_Log("Ha entrado en el mapa un hastatus romano");

// Un script de comportamiento no debe terminar
while(true) Sleep(50000);
