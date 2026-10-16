// void, Obj killer, Obj victim

// Si esegue quando un colpo uccide un oggetto: killer è chi ha dato il colpo e victim chi è morto.
// Come l'"onkill" delle classi, non può attendere: niente Sleep.

if (!killer.IsValid)
	return;

// Per verificare che l'evento scatti: una riga per ogni morte (il giocatore che uccide e quello che muore)
Mod_LogInt("OnObjectKill: mata el jugador", killer.player);
Mod_LogInt("OnObjectKill: muere un objeto del jugador", victim.player);

if (!killer.IsAlive)
	return;

// Abilità passiva di esempio "sete di sangue": un legionario romano si cura di 25 di vita quando uccide un soldato
if (!killer.IsHeirOf("RHastatus"))
	return;
if (!victim.IsMilitary)
	return;

killer.Heal(25);
Mod_Log("OnObjectKill: un legionario se cura 25 de vida al matar");
