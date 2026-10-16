// void, Obj attacker, Obj victim, int damage

// El efecto de la especialidad "Critical" (specials\critical.json): el juego solo guarda el bit en la unidad, lo que hace lo
// programa el mod. Aquí, uno de cada cuatro golpes de una unidad con la especialidad hace el triple de daño.
if (!attacker.IsValid) return;
if (!Mod_HasSpecial(attacker.AsUnit(), Critical)) return;
if (rand(100) >= 25) return;

Mod_SetAttackDamage(damage * 3);

// Un aviso para los tres primeros críticos (para comprobar que funciona)
if (Mod_AddInt("critical_golpes", 1) <= 3)
	Mod_LogInt("Golpe crítico: daño", damage * 3);
