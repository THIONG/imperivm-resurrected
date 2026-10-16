// Parche de script: lo que va debajo de cada directiva (@) se añade al GetTacticScript.vs del juego.
//
// En los scripts las variables se declaran TODAS al principio; el código (if, return...) va después. Por eso aquí no se usa @top, que
// pondría el código antes de las declaraciones del script del juego: se busca la primera línea de código con @before.

@before nRace = set.GetCentralBuilding.race;
// MOD: Dacia. Con "ia": true la IA ve su base (Galia), así que .race daría Gaul; Mod_TrueRace da la civilización de verdad.
// TS_GaulTactic2 es una táctica que trae el juego y que ninguna IA usa; el juego solo elige al empezar la partida.
if (Mod_TrueRace(set.GetCentralBuilding) == Dacia)
{
	if (GetTime<2000)
	{
		Mod_Log("GetTacticScript: la IA de Dacia usa TS_GaulTactic2");
		return TS_GaulTactic2;
	}
	return TS_NONE;
}
