# Ejemplo: parches

Cambia el cuartel romano y una constante sin sustituir ningún archivo del juego. Guía: [Parches](../../parches.md).

Qué hace:

- El hastatus pasa de 100 a 60 de oro, entrena en 3 segundos en vez de 6 y ya no gasta población.
- El cuartel tiene una orden nueva, `trainRArcherEjemplo`, que entrena arqueros (tecla `z`).
- `DamageAmplify` pasa de 200 a 250 en `CONST.INI`.

Cambia la jugabilidad: en multijugador todos tienen que llevar el mod.

Para probarlo, copia esta carpeta a `<juego>\mods\`, actívala en el gestor de contenido y reinicia. Mira `Logs\imperivm\imperivm-mod.log`: las líneas con la etiqueta `patch` dicen qué se ha aplicado y qué ha fallado.
