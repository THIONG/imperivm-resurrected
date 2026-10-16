// void, Obj This

// Se ejecuta con cada soldado de cualquier clase (hereda de la clase base Military): es la manera de reaccionar a que
// aparezca un objeto sin necesitar un evento propio
Sleep(rand(500) + 100);

// Solo se apuntan los diez primeros: un mapa trae cientos de soldados y llenarían el registro (el SDK corta a 2000 líneas por sesión).
// Mod_AddInt guarda un contador que viaja con la partida
if (Mod_AddInt("ejemplo_soldados", 1) <= 10)
	Mod_LogInt("Un soldado ha entrado en el mapa. Jugador", This.player);

// Un script de comportamiento no debe terminar
while(true) Sleep(50000);
