// void, Obj This

// Si aspetta un po', come fanno gli script del gioco, per non eseguirsi tutti nello stesso istante
Sleep(rand(500) + 100);

// All'eroe si dà l'oggetto se non ce l'ha ancora (caricando una partita salvata lo script riparte: così non si duplica)
if (!This.FindItem("Amuleto de ejemplo").IsValid) {
	if (This.AddItem("Amuleto de ejemplo")) Mod_Log("Un heroe ha recibido el Amuleto de ejemplo");
	else Mod_Log("El heroe no ha podido recibir el Amuleto de ejemplo (sin hueco libre)");
}

// Uno script di comportamento non deve terminare
while(true) Sleep(50000);
