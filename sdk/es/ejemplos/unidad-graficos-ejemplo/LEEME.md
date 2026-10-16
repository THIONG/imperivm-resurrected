# Ejemplo: unidad con gráficos propios

El **Soldado simple**: una unidad nueva con sus propios sprites, sin reutilizar los del hastatus. Es un soldado de cubitos en 8 direcciones, con 6 hojas de animación (reposo, andar, atacar, morir, transición reposo/combate y provocar), color de jugador (túnica y penacho cambian con el equipo) y sombra. Se recluta en el cuartel de Roma republicana (tecla `X`, 90 de oro) y es más débil que el hastatus, para distinguirlo.

| Archivo | Qué es |
|---|---|
| `units\RSimpleSoldier\*.rle` | Los sprites en el formato del juego (12 archivos, unos 500 KB): una hoja por animación y su sombra. Generados, no se editan a mano |
| `units\RSimpleSoldier\RSimpleSoldier.ent.xml` | La entidad: qué imagen se pinta en cada capa y animación. La genera `crear-entidad.js` |
| `data\classes\RSimpleSoldier.sc.xml` | La clase (estadísticas), con `entity="Units/RSimpleSoldier/RSimpleSoldier.ent.xml"` |
| `data\commands\SimpleSoldier.xml` | La orden de reclutarlo en el cuartel. Usa el retrato y el botón del hastatus |
| `loc\spanish.loc.xml` | Nombre y texto del botón en español |

Los sprites los dibuja `..\unidad-graficos-generador\dibujar-soldado.js`, que no forma parte del mod (un mod no admite `.js`). Dibuja los PNG, llama a `convertir-sprite.js --jugador` y luego a `crear-entidad.js unidad`. Con tus propios dibujos te saltas el primer paso: [Gráficos propios](../../graficos.md).

Ojo: el script escribe siempre dentro de `units\RSimpleSoldier\` de este ejemplo, y sobrescribe lo que haya. Si solo quieres ver los PNG, `--png <carpeta>` los deja ahí; pero los `.rle` y el `.ent.xml` se regeneran igual. Para no tocar el ejemplo, trabaja con una copia del SDK:

```
node ejemplos\unidad-graficos-generador\dibujar-soldado.js
```

## Cómo probarlo

1. Valida (debe decir OK): `node herramientas\validar-mod.js ejemplos\unidad-graficos-ejemplo --datos <carpeta DATA sacada con extraer-datos.js> --juego "<carpeta del juego>"`.
2. Copia esta carpeta a `<juego>\mods\`, actívala en el gestor de contenido (o en un perfil) y reinicia.
3. Juega con Roma republicana en una escaramuza, construye un cuartel y busca el botón nuevo ("Equipar soldado simple"), o pulsa `X` con el cuartel seleccionado.
4. Comprueba:
   - Si el juego se cierra al cargar la partida: es el fallo típico de una imagen mal cargada. `Logs\imperivm\imperivm-mod.log` apunta la imagen que falló.
   - Si la túnica y el penacho salen del color de tu jugador y no magenta.
   - Si mira hacia donde camina. Si va de espaldas o gira al revés, el orden de las 8 direcciones del juego es otro (ver "Orden de las direcciones" en [Gráficos propios](../../graficos.md)).
   - Si los pies están en el punto de la unidad y tiene sombra.
   - Si anda, ataca, muere y se queda tumbada, y si vuelve a reposo tras el combate.
5. Si algo falla, quita la carpeta de `mods\` y vuelve al perfil Original.

Cambia la jugabilidad: en multijugador todos los jugadores tienen que llevar el mod.
