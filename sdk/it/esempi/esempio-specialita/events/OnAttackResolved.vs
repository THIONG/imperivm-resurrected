// void, Obj attacker, Obj victim, int damage

// L'effetto della specialità "Critical" (specials\critical.json): il gioco salva solo il bit nell'unità; cosa faccia lo
// decide il mod. Qui, un colpo su quattro di un'unità con la specialità fa il triplo del danno.
if (!attacker.IsValid) return;
if (!Mod_HasSpecial(attacker.AsUnit(), Critical)) return;
if (rand(100) >= 25) return;

Mod_SetAttackDamage(damage * 3);

// Un avviso per i primi tre colpi critici (per verificare che funzioni)
if (Mod_AddInt("critical_golpes", 1) <= 3)
	Mod_LogInt("Golpe crítico: daño", damage * 3);
