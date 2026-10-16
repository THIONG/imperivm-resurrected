// void, Obj me

Unit this;
int cost;

this = me.AsUnit();
if (!.IsAlive)
	return;

// Il gioco non scala la resistenza per gli ordini: lo fa lo script
cost = GetCmdStaminaCost("respiro");
if (.stamina < cost)
	return;

.SetStamina(.stamina - cost);
.Heal(60);
Mod_Log("Respiro: un soldado se ha curado");
