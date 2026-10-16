// void, Obj killer, Obj victim

// It runs when a hit kills an object: killer is who dealt the hit and victim who died.
// Like the "onkill" of the classes, it can't wait: no Sleep.

if (!killer.IsValid)
	return;

// To check that the event fires: one line per death (the player who kills and the player who is killed)
Mod_LogInt("OnObjectKill: mata el jugador", killer.player);
Mod_LogInt("OnObjectKill: muere un objeto del jugador", victim.player);

if (!killer.IsAlive)
	return;

// Example passive ability "bloodlust": a Roman legionary heals 25 health when it kills a soldier
if (!killer.IsHeirOf("RHastatus"))
	return;
if (!victim.IsMilitary)
	return;

killer.Heal(25);
Mod_Log("OnObjectKill: un legionario se cura 25 de vida al matar");
