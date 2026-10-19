// void, Obj killer, Obj victim

// It runs when a hit kills an object: killer is who dealt the hit and victim who died.
// Like the "onkill" of the classes, it can't wait: no Sleep.

if (!killer.IsValid)
	return;

// To check that the event fires: one line per death (the player who kills and the player who is killed)
Mod_LogInt("OnObjectKill: the killer is player", killer.player);
Mod_LogInt("OnObjectKill: the victim belongs to player", victim.player);

if (!killer.IsAlive)
	return;

// Example passive ability "bloodlust": a Roman legionary heals 25 health when it kills a soldier
if (!killer.IsHeirOf("RHastatus"))
	return;
if (!victim.IsMilitary)
	return;

killer.Heal(25);
Mod_Log("OnObjectKill: a legionary heals 25 health on a kill");
