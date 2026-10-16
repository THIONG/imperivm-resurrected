// void, Obj This

int nSpeed;

Sleep(rand(500) + 100);

// Mod_GetOption("id") says whether the host ticked a room switch; Mod_GetOptionValue("id") gives the number of the option chosen in a
// list (0 is the first). Both give the same on every PC
nSpeed = 100;
if (Mod_GetOption("heroes-veloces")) nSpeed = 150;
if (Mod_GetOptionValue("velocidad-heroes") == 1) nSpeed = 125;
if (Mod_GetOptionValue("velocidad-heroes") == 2) nSpeed = 150;

if (nSpeed != 100) {
	This.AsUnit().SetSpeedFactor(nSpeed);
	Mod_LogInt("Opcion de velocidad de heroes: factor aplicado a un heroe", nSpeed);
}

// A behavior script must not end
while(true) Sleep(50000);
