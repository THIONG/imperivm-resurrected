// void, Obj This

// It runs for every soldier of any class (it inherits from the base class Military): it's the way to react to
// an object appearing without needing an event of its own
Sleep(rand(500) + 100);

// Only the first ten are logged: a map has hundreds of soldiers and they would fill the log (the SDK cuts off at 2000 lines per session).
// Mod_AddInt stores a counter that travels with the game
if (Mod_AddInt("example_soldiers", 1) <= 10)
	Mod_LogInt("A soldier entered the map. Player", This.player);

// A behavior script must not end
while(true) Sleep(50000);
