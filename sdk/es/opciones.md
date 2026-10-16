# Opciones de partida propias

Un mod puede añadir **interruptores** (sí / no) y **listas de valores** (un desplegable) a la lista de REGLAS de la sala, junto a "Niebla de guerra" o "Bloquear diplomacia". El anfitrión las elige, todos las ven, y tus scripts leen lo elegido con `Mod_GetOption("id")` (interruptor) o `Mod_GetOptionValue("id")` (lista). Ambas son de la API 6 de scripts.

Ejemplo: [`ejemplos/opciones-ejemplo`](ejemplos/opciones-ejemplo) (interruptor "Héroes veloces" y lista "Velocidad de héroes").

## Camino rápido

`crear-opcion.js` escribe el JSON y, si quieres, el esqueleto del script. No necesita la carpeta `DATA`.

```
node herramientas/crear-opcion.js mods/mi-mod heroes-veloces --nombre "Héroes veloces" --nombre-en "Fast heroes" --descripcion "Todos los héroes se mueven un 50 % más rápido."
node herramientas/crear-opcion.js mods/mi-mod velocidad-heroes --nombre "Velocidad de héroes" --valores "Normal,Rápida,Muy rápida" --defecto Normal --script
```

La primera crea un interruptor; con `--valores` (de 2 a 16, separados por comas) crea una lista.

| Opción | Para qué |
|---|---|
| `--nombre` | Obligatorio. Lo que se ve en la sala |
| `--nombre-en`, `--nombre-it` | El nombre en inglés y en italiano |
| `--descripcion`, `--descripcion-en`, `--descripcion-it` | La ayuda de la sala |
| `--valores`, `--valores-en`, `--valores-it` | Los valores de una lista, en el mismo orden y el mismo número. El id de cada valor sale de su texto |
| `--defecto` | Interruptor: `true` o `false`. Lista: el número del valor (desde 0), su id o su texto. Sin él, `false` o el primero |
| `--script` | Crea también `data\mod\<id>.vs`, un comportamiento con el `if (Mod_GetOption("<id>"))` (o `Mod_GetOptionValue`) listo para completar. Engánchalo a una clase con un parche, como en el ejemplo |

Genera `options\<id>.json` y, con `--script`, `data\mod\<id>.vs`. Dice cuántos de los 8 bits gasta y cuántos lleva ya el mod. No pisa nada: con un id repetido en el mod, se para sin escribir nada.

## Cómo se hace a mano

1. Un archivo `options\<id>.json` en tu mod.

   Un interruptor:

   ```json
   {
     "id": "heroes-veloces",
     "name": { "spanish": "Héroes veloces", "english": "Fast heroes" },
     "description": { "spanish": "Todos los héroes se mueven un 50 % más rápido.", "english": "All heroes move 50% faster." },
     "default": false
   }
   ```

   Una lista (de 2 a 16 valores):

   ```json
   {
     "id": "velocidad-heroes",
     "name": { "spanish": "Velocidad de héroes", "english": "Hero speed" },
     "values": [
       { "id": "normal", "name": { "spanish": "Normal", "english": "Normal" } },
       { "id": "rapida", "name": { "spanish": "Rápida (+25 %)", "english": "Fast (+25%)" } },
       "Muy rápida"
     ],
     "default": "normal"
   }
   ```

   - `id`: letras, números, `_` y `-`, hasta 32. Es lo que usan los scripts y no distingue mayúsculas. Si falta, se usa el nombre del archivo.
   - `name` y `description`: un texto, o uno por idioma (el del juego y, si no está, el inglés).
   - `values`: cada uno es un texto o un objeto con `id` y `name`. El primero es el 0.
   - `default`: en un interruptor, `true` o `false`; en una lista, el número del valor (desde 0) o su `id`. Si pones un `id` que no existe, se toma el 0 sin avisar; `validar-mod.js` sí lo detecta.

2. Un script que la lea (un comportamiento de clase, un evento o un script de victoria):

   ```
   if (Mod_GetOption("heroes-veloces")) {
       This.AsUnit().SetSpeedFactor(150);
   }
   if (Mod_GetOptionValue("velocidad-heroes") == 1) {
       This.AsUnit().SetSpeedFactor(125);
   }
   ```

   `Mod_GetOption` da `true` si el valor no es el 0 (también con una lista). `Mod_GetOptionValue` da el número del valor elegido.

## Límites

- **Hay 8 bits para todas las opciones de todos los mods activos.** Un interruptor gasta 1; una lista de 2 valores gasta 1, de 3 o 4 gasta 2, de 5 a 8 gasta 3 y de 9 a 16 gasta 4. Entran por orden alfabético de `id`; lo que no cabe se ignora y el registro lo dice. Si dos opciones tienen el mismo `id`, vale la primera que encuentra el juego al montar los mods.
- **Solo funcionan con sala.** Las opciones se fijan al empezar una partida desde la sala (o al volver a una online). En aventuras, campañas y tutoriales no hay sala: `Mod_GetOption` da siempre 0 y la lista vale 0, aunque pongas `"default": true`.
- Los preajustes de la sala (Clásico, Competitivo) no tocan las opciones de los mods.

## Online

Es jugabilidad: todos los jugadores deben llevar el mismo mod. La opción viaja con las demás reglas de la sala y se guarda con la partida.
