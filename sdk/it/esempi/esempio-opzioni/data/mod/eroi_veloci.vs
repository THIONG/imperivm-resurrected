// void, Obj This

int nSpeed;

Sleep(rand(500) + 100);

// Mod_GetOption("id") dice se l'host ha attivato un interruttore della sala; Mod_GetOptionValue("id") dà il numero dell'opzione scelta in un
// elenco (0 la prima). Entrambe danno lo stesso valore su tutti i PC
nSpeed = 100;
if (Mod_GetOption("heroes-veloces")) nSpeed = 150;
if (Mod_GetOptionValue("velocidad-heroes") == 1) nSpeed = 125;
if (Mod_GetOptionValue("velocidad-heroes") == 2) nSpeed = 150;

if (nSpeed != 100) {
	This.AsUnit().SetSpeedFactor(nSpeed);
	Mod_LogInt("Opcion de velocidad de heroes: factor aplicado a un heroe", nSpeed);
}

// Uno script di comportamento non deve terminare
while(true) Sleep(50000);
