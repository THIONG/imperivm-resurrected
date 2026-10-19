//void, int player, str param

// Victory mode "Dominio": whoever controls N settlements wins (N = the limit chosen in the room; if it's 0, 3).
// Elimination still applies: whoever is left with no town halls and no troops loses.
// The game runs one thread of this script per player.
Query q;
int i, nGoal, nMine, nTheirs;
bool bLost;

q = ClassPlayerObjs("BaseTownhall", player);
nGoal = Str2Int(param);
if (nGoal < 2) nGoal = 3;

EnvWriteInt("modo", 1);
EnvWriteInt(player, "elimination", 0);
SetPlayerStatus(player, 0, Translatef("Domination (%s1)", nGoal), false);

while (1) {
	Sleep(1000);
	nMine = q.count;

	// The best rival's settlements
	nTheirs = 0;
	for (i=1; i<=8; i+=1) {
		if (i != player && !DiplAreAllied(i, player)) {
			if (ClassPlayerObjs("BaseTownhall", i).count > nTheirs) nTheirs = ClassPlayerObjs("BaseTownhall", i).count;
		}
	}

	if (nMine >= nGoal) {
		SetPlayerStatus(player, 1, Translate("You won!"), true);
		SetPlayerStatus(player, 0, true);
		EnvWriteInt(player, "elimination", 1);
		EndGame(player, false); //false = win
		return;
	}
	if (nTheirs >= nGoal) {
		SetPlayerStatus(player, 1, Translate("You lost"), true);
		SetPlayerStatus(player, 0, true);
		EnvWriteInt(player, "elimination", 1);
		EndGame(player, true); //true = lose
		return;
	}

	// Elimination: no settlements and no troops
	if (nMine == 0 && MilUnits(player) == 0) {
		SetPlayerStatus(player, 1, Translate("You were eliminated"), true);
		SetPlayerStatus(player, 0, true);
		EnvWriteInt(player, "elimination", 1);
		EndGame(player, true); //true = lose
		return;
	}
	SetPlayerStatus(player, 1, Translatef("Settlements: %s1 / %s2", nMine, nGoal), true);
	SetPlayerStatus(player, 0, true);
}
