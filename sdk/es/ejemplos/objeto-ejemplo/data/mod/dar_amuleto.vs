// void, Obj This

// Se espera un poco, como hacen los scripts del juego, para no ejecutarse todos en el mismo instante
Sleep(rand(500) + 100);

// Se le da el objeto al héroe si aún no lo tiene (al cargar una partida guardada el script vuelve a empezar: así no se duplica)
if (!This.FindItem("Amuleto de ejemplo").IsValid) {
	if (This.AddItem("Amuleto de ejemplo")) Mod_Log("Un heroe ha recibido el Amuleto de ejemplo");
	else Mod_Log("El heroe no ha podido recibir el Amuleto de ejemplo (sin hueco libre)");
}

// Un script de comportamiento no debe terminar
while(true) Sleep(50000);
