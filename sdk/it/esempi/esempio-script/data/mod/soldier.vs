// void, Obj This

// Si esegue con ogni soldato di qualsiasi classe (eredita dalla classe base Military): è il modo di reagire al fatto che
// compaia un oggetto senza bisogno di un evento proprio
Sleep(rand(500) + 100);

// Si annotano solo i primi dieci: una mappa ha centinaia di soldati e riempirebbero il registro (l'SDK si ferma a 2000 righe per sessione).
// Mod_AddInt salva un contatore che viaggia con la partita
if (Mod_AddInt("ejemplo_soldados", 1) <= 10)
	Mod_LogInt("Un soldado ha entrado en el mapa. Jugador", This.player);

// Uno script di comportamento non deve terminare
while(true) Sleep(50000);
