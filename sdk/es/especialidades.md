# Especialidades de unidad propias

El juego trae 36 especialidades fijas (Parry, Penetration, Deflection…). No se pueden ampliar, pero un mod puede declarar
especialidades nuevas cuyo efecto programas tú con scripts.

Ejemplo listo para copiar: [`ejemplos/especialidad-ejemplo`](ejemplos/especialidad-ejemplo), «Golpe crítico»: un 25 % de los golpes de los
hastatus hacen el triple de daño.

## Cómo funciona

Cada unidad lleva una máscara de 64 bits, un bit por especialidad. Los bits 0 al 35 son las 36 del juego. Los 36 al 63 son tuyos y se guardan con la partida. Una especialidad de mod ocupa uno.

Son tres pasos.

**1. Declararla.** Un archivo `specials\<id>.json` en tu mod:

```json
{
  "id": "Critical",
  "bit": 36,
  "name": {"spanish": "Golpe crítico", "english": "Critical hit", "italian": "Colpo critico"},
  "description": {"spanish": "Uno de cada cuatro golpes hace el triple de daño."}
}
```

| Campo | Qué es |
|---|---|
| `id` | El nombre: letras, números y `_`, empieza por letra, hasta 24. Lo escribes en `unit_specials` y es la constante de script (`Critical`). No puede ser el nombre de una de las 36 del juego (`Triple strike`…). Evita también ids que, en minúsculas y con `_` por espacio, sean constantes del juego (`triple_strike`, `keen_sight`, `life_steal`…): el validador no lo comprueba y la constante chocaría con la del juego. |
| `bit` | Un número del 36 al 63. No lo cambies una vez publicado: las partidas guardadas lo recuerdan. Dos mods no pueden usar el mismo. |
| `name` | Cómo se llama en cada idioma, o un solo texto. |
| `description` | Se lee, pero no se muestra en ningún sitio todavía. Descríbela en el texto de ayuda de la unidad. |
| `icon` | (Opcional) Se lee, pero no se usa. |

**2. Asignarla a clases.** Con el atributo `unit_specials` de siempre. El juego ignora los nombres que no conoce y el mod pone el bit. Mejor con
un [parche](parches.md), para no sustituir el archivo de la clase:

```xml
<patch>
  <set select="/class/properties[@unit_specials='Deflection']" unit_specials="Deflection, Critical"/>
</patch>
```

La herencia es la del juego. Si una clase pone `unit_specials`, su lista sustituye a la del padre; si no lo pone, hereda la del padre entera. Al
añadir una a una clase que ya tiene, repite las que tenía (`Deflection, Critical`). En el juego los nombres distinguen mayúsculas; los de tu mod, no.

**3. Programar el efecto.** El juego solo guarda el bit. Lo que hace lo escribes en un [script](scripts.md):

| Función | Qué hace |
|---|---|
| `Mod_HasSpecial(Unit unidad, int bit)` : `bool` | ¿Tiene la unidad esa especialidad? Vale para cualquier bit de 0 a 63, también los del juego. Recibe una `Unit`; con un `Obj`, usa `objeto.AsUnit()` (si no es una unidad, da `false`). |
| `Mod_SetSpecial(Unit unidad, int bit, bool puesta)` | Pone o quita la especialidad a esa unidad, por ejemplo al subir de nivel. Se guarda con la partida. |

El bit es la constante con el `id`: `Mod_HasSpecial(attacker.AsUnit(), Critical)`.

El sitio natural para el efecto es el evento `OnAttackResolved`, que puede cambiar el daño de cada golpe (`events\OnAttackResolved.vs`). Vale
cualquier otro script: un `<behavior>` que revise unidades cada cierto tiempo, `OnObjectKill`, `OnObjectDie`…

```
// void, Obj attacker, Obj victim, int damage
if (!attacker.IsValid) return;
if (!Mod_HasSpecial(attacker.AsUnit(), Critical)) return;
if (rand(100) >= 25) return;
Mod_SetAttackDamage(damage * 3);
```

## Multijugador

`specials\`, `events\` y `civs\` cuentan como jugabilidad: todos los jugadores llevan el mismo mod.

## Errores

Salen en `Logs\imperivm\imperivm-mod.log` (líneas con la etiqueta `specials`) y en la ficha del mod en el gestor: un `id` que no vale, un `bit` que falta
o se repite, un nombre que es de una especialidad del juego.

Aviso: si añades una `[Sección]` desconocida a `UNIT_SPECIALS.INI` (con un parche, para ponerle icono y texto a una
especialidad), el juego puede cerrarse. No lo hagas. Las especialidades de mods no usan ese archivo.

## Lo que falta

- La barra de información de la unidad, arriba, donde el juego enseña hasta tres especialidades suyas con su icono, no enseña las de mods.
- El texto de estadísticas del botón de reclutar debería añadirlas debajo de las del juego, pero sin el título
  «Specials»: una unidad que solo tenga especialidades de mod las mostraría sin título.
- Iconos propios: no se muestran.
- No se pueden añadir a la tabla de las 36 del juego ni crear habilidades de héroe nativas nuevas. Las activas se
  hacen con [órdenes y scripts](habilidades.md).
