// void, Obj me

Unit this;
int cost;

this = me.AsUnit();
if (!.IsAlive)
	return;

// El juego no cobra el aguante de las órdenes: lo hace el script
cost = GetCmdStaminaCost("respiro");
if (.stamina < cost)
	return;

.SetStamina(.stamina - cost);
.Heal(60);
Mod_Log("Respiro: un soldado se ha curado");
