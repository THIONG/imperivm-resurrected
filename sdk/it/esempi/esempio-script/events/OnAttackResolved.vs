// void, Obj attacker, Obj victim, int damage

// Si esegue a ogni colpo che fa danno, subito prima di applicarlo. damage è il danno che la vittima sta per subire (mai negativo).
// Esempio: gli hastati romani fanno il doppio del danno. Mod_SetAttackDamage cambia il danno del colpo.
// Si esegue MOLTE volte al secondo in una battaglia: filtra presto e non fare lavoro costoso.
if (!attacker.IsValid) return;
if (!attacker.IsHeirOf("RHastatus")) return;

// Un avviso per i primi tre colpi (per verificare che funzioni)
if (Mod_AddInt("hastatus_golpes", 1) <= 3)
	Mod_LogInt("Golpe de un hastatus: daño original", damage);

Mod_SetAttackDamage(damage * 2);
