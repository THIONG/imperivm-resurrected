// void, Obj attacker, Obj victim, int damage

// The effect of the "Critical" specialty (specials\critical.json): the game only stores the bit on the unit; what it does is
// up to the mod. Here, one hit in four from a unit with the specialty does triple damage.
if (!attacker.IsValid) return;
if (!Mod_HasSpecial(attacker.AsUnit(), Critical)) return;
if (rand(100) >= 25) return;

Mod_SetAttackDamage(damage * 3);

// A notice for the first three criticals (to check that it works)
if (Mod_AddInt("critical_golpes", 1) <= 3)
	Mod_LogInt("Critical hit: damage", damage * 3);
