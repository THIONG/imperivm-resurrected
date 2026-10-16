// void, Obj victim

// Se ejecuta cuando muere cualquier objeto del mapa (unidades, edificios, adornos...), justo antes de que el juego
// lo procese. Como el "ondie" de las clases, no puede esperar: nada de Sleep.

if (!victim.IsValid)
	return;

// Aqui solo nos interesan los soldados
if (victim.IsMilitary)
	Mod_LogInt("OnObjectDie: ha muerto un soldado del jugador", victim.player);
