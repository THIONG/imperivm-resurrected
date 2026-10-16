// void, Obj attacker, Obj victim, int damage

// Se ejecuta con cada golpe que hace daño, justo antes de aplicarlo. damage es el daño que va a recibir la víctima (nunca negativo).
// Ejemplo: los hastatus romanos hacen el doble de daño. Mod_SetAttackDamage cambia el daño del golpe.
// Se ejecuta MUCHAS veces por segundo en una batalla: filtra pronto y no hagas trabajo caro.
if (!attacker.IsValid) return;
if (!attacker.IsHeirOf("RHastatus")) return;

// Un aviso para los tres primeros golpes (para comprobar que funciona)
if (Mod_AddInt("hastatus_golpes", 1) <= 3)
	Mod_LogInt("Golpe de un hastatus: daño original", damage);

Mod_SetAttackDamage(damage * 2);
