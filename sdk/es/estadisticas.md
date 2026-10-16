# Estadísticas de unidades y edificios

Lo que no lleva marca está en los datos del juego. *(deducido)* es una suposición sobre cómo usa el juego ese valor, y *(parcial)* la menos fiable.

Las estadísticas son atributos de `<properties>` en la clase (`data\classes\<Clase>.sc.xml`). Para cambiarlas sin copiar la clase entera usa un
[parche](parches.md); para crear una unidad, mira [Unidades nuevas](unidades.md).

```xml
<patch>
  <!-- Los legionarios corren más: velocidad 60 -> 75 -->
  <set select="/class/properties[@speed]" speed="75"/>
</patch>
```

Va en `patches\data\classes\RHastatus.sc.xml`. Un atributo que la clase no pone se hereda de su padre.

## Qué atributo toca cada cosa

| Quiero cambiar… | Atributo | Notas |
|---|---|---|
| Velocidad de movimiento | `speed` | La animación de andar se adapta sola (deducido) |
| Ritmo de ataque | no hay atributo | Duración de la animación de atacar, o `SetSpeedFactor` por script |
| Alcance de un arquero | `range` | |
| Alcance mínimo (catapultas) | `min_range` | |
| Tamaño real (hitbox) | `radius` | Colisión, caminos y alcance (deducido) |
| Zona de clic | `selection_radius` | Cosmético (deducido) |
| Daño | `damage` | |
| Armadura | `armor_slash` | `armor_pierce` no hace nada (parcial) |
| Vida | `maxhealth` | |
| Visión | `sight` | |
| Qué ataca antes | `target_factor` | |
| Proyectiles | `projectile_class` | Con límites, ver *Alcance* |

### Velocidad (`speed`)

- Son unidades de mapa por segundo. En el juego: infantería 60-80, arqueros 60-160, caballería 130-190, héroes 120, arietes 40, barcos 240.
- El ciclo de la animación de andar dura `step × 1000 / speed` milisegundos, con `step` sacado del `.ent.xml` (el hastatus: 42 × 1000 / 60 = 700 ms).
  Cambiar solo `speed` no desincroniza el dibujo: la animación se acelera o frena con el movimiento. *(deducido)*
- `speed="0"` en una unidad que anda cuelga el juego por una división entre cero. También una clase que no defina `speed` ni lo herede. *(deducido)*

### Ritmo de ataque

No hay un atributo de «velocidad de ataque» para unidades. *(deducido)*

- Un golpe dura lo que la animación de atacar, la suma de los `<frame duration>` del `.ent.xml`, y el daño se aplica en el momento `action_time` de
  esa animación. En las 118 animaciones del juego, los golpes van de 594 a 1584 ms, con una mediana de 990 ms (el hastatus: 994 ms, `action_time` 426).
- `attack_delay` no sirve para unidades: solo lo usan `Catapult` (500) y `FakeTower` (400), y es lo único que lo pone en los datos. *(deducido)*
- Para atacar más rápido hay dos vías. Una, acortar los fotogramas de la animación de atacar y reescalar `action_time` en la misma proporción.
  La otra, `Unit::SetSpeedFactor(150)` por script, que acelera a la vez el paso y el ataque un 50 %. `SetSpeedFactor(0)` cuelga el juego. *(deducido)*
- Para cambiar el `.ent.xml` desde un mod, un parche XML debería valer. Un `.ent.xml` suelto no sustituye al del juego ([Gráficos propios](graficos.md)).
- El ritmo y el momento del golpe son simulación: tienen que ser iguales para todos los jugadores.

### Alcance (`range`, `min_range`)

- Valores del juego: infantería 15-40, arqueros 300-500 (los barcos llegan a 700), centinelas 600, caballería 15-40, catapultas 700-800, torres
  600-800. `RCatapult` tiene `min_range=201`.
- Se mide de borde a borde: la distancia entre centros menos el `radius` de las dos unidades. Por eso `radius` también cuenta. *(deducido)*
- Una unidad cuerpo a cuerpo pasa a ser «de distancia» con `range` más un `projectile_class` (Arrow, Javelin, Axe, Slingstone) y una animación de
  ataque con `action_time`.
- Con `range` menor que `min_range` nunca ataca. Con `range="0"` en una unidad cuerpo a cuerpo, tampoco. *(deducido)*
- El proyectil tarda lo que dure su animación sea cual sea la distancia (flecha 495 ms, hacha y jabalina 561, piedra de honda 300). No hay atributo
  de velocidad del proyectil. Un atacante con proyectil hace la mitad de daño a los edificios. *(deducido)*
- Catapultas: la parábola sale de `shot_tan` y `shot_height`. No los cambies en un solo ordenador.

### Hitbox y tamaño

| Atributo | Para qué | Valores del juego |
|---|---|---|
| `radius` | Tamaño físico: colisión, caminos, separación y alcance (deducido). Simulación | Infantería 10, caballería 20, héroes 20-30, arietes 80, catapultas 50 |
| `selection_radius` | Aro de selección y zona de clic. Cosmético mientras la unidad tenga `formation_radius` (deducido) | |
| `formation_radius` | Separación entre huecos de una formación. Simulación | Infantería 26, caballería 35, héroes 45 |

En un edificio nuevo pon siempre `radius`. El heredado (`Building`: 600) es enorme y los atacantes «llegan» al edificio desde muy lejos. Los de
verdad llevan 160-250 los cuarteles, 47 las torres, 80-110 las murallas y 333-345 los fortines.

### Daño y armadura

Constantes de `data\const.ini`, sección `[GamePlay]`: `MinDamage=3`, `MinPercentOfAttackersDamage=20`, `DamageAmplify=200`
y `LevelDifferenceDamageTable`. Cómo se usan es *(deducido)*:

- Un golpe hace `damage − armor_slash` del defensor, con un mínimo del 20 % del daño del atacante y nunca menos de 3. Después se aplica la
  diferencia de nivel entre los dos: va de −90 % a +90 %, y si el defensor tiene más nivel, resta.
- `armor_pierce` no entra en el cálculo: se puede escribir y no hace nada *(parcial: puede que algún script lo use)*. Si quieres dar armadura, usa `armor_slash`.
- `damage_type` (`slash`, `pierce`, `siege`, `none`) no cambia el daño: decide a quién puede atacar la unidad. `slash` y `pierce` atacan unidades
  (y edificios si hay proyectil), `siege` también edificios, `none` no ataca.
- Los edificios no tienen armadura: el daño entra entero.
- `DamageAmplify` es el daño a la guarnición de un edificio. Estas constantes se retocan con [parches INI](parches.md).

### Otros atributos

| Atributo | Qué hace |
|---|---|
| `maxhealth` | Vida. Infantería 160-3000, héroes 1000-1300, fortines 10000. `0` cuelga un edificio o hace nacer la unidad muerta *(deducido)* |
| `maxstamina` | La energía que gastan las especialidades y las [habilidades](habilidades.md). Todos los soldados del juego tienen 10. Con menos que el coste, la especialidad no se activa *(deducido)* |
| `sight` | Visión: revela la niebla (deducido). Infantería 500, centinelas 600, civiles 150 |
| `target_factor` | Qué se ataca antes: más bajo, antes. Defecto 100, héroes 200, civiles 1000. Negativo = nunca lo elige solo (la fauna lleva -1) *(deducido)* |
| `unit_specials` | [Especialidades](especialidades.md), las del juego y las tuyas |
| `feeds`, `max_food` | `Unit` hereda `feeds=1` y `max_food=20`: todas las unidades normales comen. Con comida a 0 pierden vida *(deducido; cuándo, sin aclarar)*. `RamUnit`, `Sentry`, `GGhost` y los animales llevan `feeds=0`; pónselo a máquinas y criaturas mágicas |
| `does_not_regenerate` | `1`: sin comida ni regeneración. Útil para invocados y máquinas *(deducido)* |
| `stamina_dec_time` | En unidades invocadas: milisegundos entre pérdidas de 1 de energía. Viven ≈ `(maxstamina+1) × este valor`; con `0` o menos mueren al instante *(deducido)* |
| `water_unit` | Barcos y peces: caminos y objetivos de agua |
| `desync_hash` | `1`: el movimiento de este objeto entra en la comprobación de sincronía. Pon `0` en efectos visuales nuevos *(deducido)* |

## Lo que no hace nada

`armor_pierce` *(parcial)*, `speed_modify` y `taunt` (como propiedad es un nombre de sonido) se leen y no se usan. *(deducido)* `attack_delay` no
sirve en unidades. `target_priority` solo cuenta para torres y catapultas; para unidades, `target_factor`.

## Cambiar en plena partida (por script)

| Qué | Cómo |
|---|---|
| Vida, vida máxima, energía, energía máxima | `SetHealth`, `SetMaxHealth`, `SetStamina`, `SetMaxStamina`. El juego recalcula los máximos al equipar un objeto o subir de nivel y pisa el valor |
| Visión | `SetSight(n)`, de 100 a 2048 |
| Velocidad y ritmo de ataque | `Unit::SetSpeedFactor(n)`, en porcentaje (100 = normal). `0` cuelga |
| Nivel, experiencia, especialidades | `SetLevel`, `SetExperience`, `Mod_SetSpecial` |
| Daño, armadura, alcance o radio de una unidad concreta | No hay función. Crea otra clase con esos valores y convierte la unidad con `Unit::Mutate("OtraClase")`, que conserva vida, experiencia y órdenes |

Las funciones están en [funciones-del-juego.txt](funciones-del-juego.txt). Para cambiar el daño de
un golpe concreto, usa el evento `OnAttackResolved` con `Mod_SetAttackDamage` ([Especialidades](especialidades.md)).

## Lo que no se puede con XML

- La fórmula de combate (armadura plana, mínimos, tabla de niveles). Solo se ajustan las constantes de `const.ini`.
- La velocidad del proyectil y su alcance de vuelo: el golpe sale si la distancia es menor que `range + 150`. *(deducido)*
- Comportamientos de código nuevos: los `cpp_class` son solo los que trae el juego.
- La lógica de las formaciones, salvo lo que permite [Formaciones](formaciones.md).

## Multijugador

Casi todo lo de arriba es simulación. Todos los jugadores tienen que llevar el mismo mod, y el juego lo comprueba al entrar en una sala; una
diferencia entre dos jugadores desincroniza la partida y se aborta. Son cosméticos `selection_radius`, `healthbar_*`, `icon`,
`display_name`, `help` y los dibujos.

Los dibujos (`.rle`) no cuentan para la comprobación. La lista completa de lo que sí cuenta está en el [README](README.md).

## Lo que no se sabe

- La equivalencia exacta entre unidades de mapa y píxeles de pantalla.
- Si `sight` limita también a qué distancia busca objetivo una unidad. Se ve que revela la niebla y que los scripts la usan para buscar; el cálculo exacto, no.
- Qué `formation_priority` va delante en una formación.
- Qué pasa con un `action_time` mayor que la duración de la animación. Evítalo.
