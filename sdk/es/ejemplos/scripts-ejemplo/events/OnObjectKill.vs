// void, Obj killer, Obj victim

// Se ejecuta cuando un golpe mata a un objeto: killer es quien dio el golpe y victim quien murio.
// Como el "onkill" de las clases, no puede esperar: nada de Sleep.

if (!killer.IsValid)
	return;

// Para comprobar que el evento se dispara: una linea por cada muerte (jugador que mata y jugador al que mata)
Mod_LogInt("OnObjectKill: mata el jugador", killer.player);
Mod_LogInt("OnObjectKill: muere un objeto del jugador", victim.player);

if (!killer.IsAlive)
	return;

// Habilidad pasiva de ejemplo "sed de sangre": un legionario romano se cura 25 de vida al matar a un soldado
if (!killer.IsHeirOf("RHastatus"))
	return;
if (!victim.IsMilitary)
	return;

killer.Heal(25);
Mod_Log("OnObjectKill: un legionario se cura 25 de vida al matar");
