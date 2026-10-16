// Patch di script: quello che sta sotto ogni direttiva (@) viene aggiunto al GetTacticScript.vs del gioco.
//
// Negli script le variabili si dichiarano TUTTE all'inizio; il codice (if, return...) viene dopo. Per questo qui non si usa @top, che
// metterebbe il codice prima delle dichiarazioni dello script del gioco: si cerca la prima riga di codice con @before.

@before nRace = set.GetCentralBuilding.race;
// MOD: Dacia. Con "ia": true l'IA vede la sua base (Gallia), quindi .race darebbe Gaul; Mod_TrueRace dà la civiltà vera.
// TS_GaulTactic2 è una tattica che c'è nel gioco e che nessuna IA usa; il gioco ne sceglie una solo all'inizio della partita.
if (Mod_TrueRace(set.GetCentralBuilding) == Dacia)
{
	if (GetTime<2000)
	{
		Mod_Log("GetTacticScript: la IA de Dacia usa TS_GaulTactic2");
		return TS_GaulTactic2;
	}
	return TS_NONE;
}
