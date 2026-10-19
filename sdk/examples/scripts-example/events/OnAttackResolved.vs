// void, Obj attacker, Obj victim, int damage

// It runs for every hit that does damage, right before applying it. damage is the damage the victim is going to take (never negative).
// Example: Roman hastati do double damage. Mod_SetAttackDamage changes the hit's damage.
// It runs MANY times per second in a battle: filter early and don't do expensive work.
if (!attacker.IsValid) return;
if (!attacker.IsHeirOf("RHastatus")) return;

// A notice for the first three hits (to check that it works)
if (Mod_AddInt("hastatus_golpes", 1) <= 3)
	Mod_LogInt("Hastatus hit: original damage", damage);

Mod_SetAttackDamage(damage * 2);
