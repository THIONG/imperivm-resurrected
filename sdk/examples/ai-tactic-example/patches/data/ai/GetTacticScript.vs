// Script patch: what goes under each directive (@) is added to the game's GetTacticScript.vs.
//
// In scripts ALL variables are declared at the start; the code (if, return...) comes after. That's why @top isn't used here: it
// would put the code before the game script's declarations: the first line of code is found with @before.

@before nRace = set.GetCentralBuilding.race;
// MOD: Dacia. With "ia": true the AI sees its base (Gaul), so .race would give Gaul; Mod_TrueRace gives the real civilization.
// TS_GaulTactic2 is a tactic that ships with the game and that no AI uses; the game only picks one when the match starts.
if (Mod_TrueRace(set.GetCentralBuilding) == Dacia)
{
	if (GetTime<2000)
	{
		Mod_Log("GetTacticScript: the Dacia AI uses TS_GaulTactic2");
		return TS_GaulTactic2;
	}
	return TS_NONE;
}
