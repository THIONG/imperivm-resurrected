// void, Obj victim

// Si esegue quando muore un qualsiasi oggetto della mappa (unità, edifici, decorazioni...), subito prima che il gioco
// lo elabori. Come l'"ondie" delle classi, non può attendere: niente Sleep.

if (!victim.IsValid)
	return;

// Qui ci interessano solo i soldati
if (victim.IsMilitary)
	Mod_LogInt("OnObjectDie: ha muerto un soldado del jugador", victim.player);
