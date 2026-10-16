// void, Obj This

int nSpeed;

Sleep(rand(500) + 100);

// Mod_GetOption("id") dice si el anfitrión marcó un interruptor de la sala; Mod_GetOptionValue("id") da el número de la opción elegida en una
// lista (0 la primera). Los dos dan lo mismo en todos los PCs
nSpeed = 100;
if (Mod_GetOption("heroes-veloces")) nSpeed = 150;
if (Mod_GetOptionValue("velocidad-heroes") == 1) nSpeed = 125;
if (Mod_GetOptionValue("velocidad-heroes") == 2) nSpeed = 150;

if (nSpeed != 100) {
	This.AsUnit().SetSpeedFactor(nSpeed);
	Mod_LogInt("Opcion de velocidad de heroes: factor aplicado a un heroe", nSpeed);
}

// Un script de comportamiento no debe terminar
while(true) Sleep(50000);
