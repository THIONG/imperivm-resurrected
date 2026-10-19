// void, Obj me

Unit this;
int cost;

this = me.AsUnit();
if (!.IsAlive)
	return;

// The game doesn't charge stamina for orders: the script does it
cost = GetCmdStaminaCost("respiro");
if (.stamina < cost)
	return;

.SetStamina(.stamina - cost);
.Heal(60);
Mod_Log("Respiro: a soldier was healed");
