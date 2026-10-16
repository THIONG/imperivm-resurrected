# Formaciones nuevas

Las formaciones de los héroes (Línea, Frente, Bloque, Caballería) son datos: un `formations.xml` con bonos y colocación, y dos órdenes por formación en `data\commands\Hero.xml`. Un mod añade las suyas con [parches](parches.md), sin compilar nada.

Ejemplo: [`ejemplos/formacion-ejemplo`](ejemplos/formacion-ejemplo) (la Cuña).

## Camino rápido

`crear-formacion.js` escribe las tres piezas. Necesitas la carpeta `DATA` del juego ([Datos del juego](README.md#datos-del-juego)):

```
node herramientas/crear-formacion.js <DATA> mods/mi-mod Wedge --dano 3 --armadura 1 --como "Cavalry core" --tecla i
```

| Opción | Para qué |
|---|---|
| `<Nombre>` | El `Name` de la formación: letras y números, sin espacios. Tiene que ser nuevo |
| `--dano`, `--armadura`, `--defensa-pierce`, `--defensa-slash`, `--alcance`, `--nivel` | Los bonos (de -1000 a 1000). Sin ninguno, la formación solo cambia la colocación |
| `--como` | La formación del juego de la que copia la lista de clases, el tamaño, la separación y el dibujo del botón: `Front` (por defecto), `Line`, `Block`, `Cavalry core`, `Horse Wings`, `Ranged Wings` |
| `--clases "Unit=flancos,Horse=frente"` | Tu propia colocación, en vez de la de `--como`. Los sitios son `frente`, `flancos` y `centro` |
| `--ancho`, `--alto`, `--frente`, `--flanco-x`, `--flanco-y` | `Width`, `Height` y las tres separaciones (`Offset…`) |
| `--tecla` | La letra que elige la formación. Sin ella, busca una libre |
| `--descripcion`, `--boton` | El `Description` (en inglés) y un botón distinto (`States/…`) |

Genera:

```
patches\data\formations.xml                     la formación
patches\data\commands\Hero.xml                  sus dos botones, form-<nombre>-menu y form-<nombre>
patches\data\subai\hero_formation_verify.vs     el parche del script
mod.json                                        solo si la carpeta no tiene
```

Si el mod ya trae otra formación, la nueva se añade a esos mismos archivos sin pisar la anterior. Sirve para el parche del script: cada formación suma su `@replace`.

Comprueba que la tecla esté libre para el héroe, que hereda las órdenes de `Unit` y `Military`. Si choca, avisa y la genera igual. En el juego solo queda libre la `i`, así que la segunda formación de un mod tendrá que compartir tecla: elige una letra y comprueba que el choque no te importa.

Los botones usan el dibujo de la formación base. Para uno propio, ver [Imágenes propias](imagenes.md) y pasa `--boton`.

## Lo que hace por dentro (cómo hacerlo a mano)

### Las tres piezas

**1. La formación** (`patches\data\formations.xml`):

```xml
<patch>
  <add select="/Formations">
    <FormationClass Name="Wedge"
        Description="The warriors form a wedge with the cavalry at the point."
        Width="3" Height="1"
        BonusDamage="3" BonusArmor="1"
        OffsetFrontLineByY="40" OffsetWingsByX="30" OffsetWingsByY="0">
      <Class Name="Unit" Wings="1"/>
      <Class Name="Horse" FrontLine="1"/>
      <Class Name="BaseMage" CentralBlock="1"/>
      <Class Name="Peasant" CentralBlock="1"/>
    </FormationClass>
  </add>
</patch>
```

| Atributo | Para qué |
|---|---|
| `Name` | Nombre de la formación; es lo que usan las órdenes |
| `Description` | No se muestra en el botón: el juego pone un texto fijo ("Army bonus during stand ground") con los iconos de los bonos |
| `Width`, `Height` | Tamaño de la rejilla del grupo de primera línea (`FrontLine`). En la Cuña, donde las `Unit` van a los flancos, `Width=3` solo afecta a los caballos |
| `BonusDamage`, `BonusArmor`, `BonusRange`, `BonusLevel` | Bonos del ejército, y solo mientras la tropa está parada ("stand ground"). Se acotan entre -1000 y 1000 |
| `BonusDefencePierce`, `BonusDefenceSlash` | Defensa contra perforante y contra corte por separado. `BonusArmor` pone las dos a la vez |
| `OffsetFrontLineByY`, `OffsetWingsByX`, `OffsetWingsByY` | Cuánto se separan la primera línea y los flancos del héroe |
| `<Class Name="…" …/>` | Dónde van las unidades de esa clase: `FrontLine="1"` delante, `Wings="1"` a los lados, `CentralBlock="1"` en el centro, alrededor del héroe |

`BonusAttack` aparece en alguna formación del juego, pero no hace nada: no cuentes con él.

**2. Los botones** (`patches\data\commands\Hero.xml`). Dos órdenes: `form-<nombre>-menu` (el botón que se ve cuando esa es la formación actual) y `form-<nombre>` (el que la elige). Las dos llevan `param="<Name>"` y `menu="1"`. Copia `form-front-menu` y `form-front` y cambia `name`, `param`, `key` y `button` (el dibujo; ver [Imágenes](imagenes.md)).

La tecla no debe chocar con otra orden del héroe, que también hereda las de `Unit`. En `data\commands` (con `extraer-datos.js`, ver [Parches](parches.md)) busca `key="x"` en `HERO.XML` y `UNIT.XML`. El ejemplo usa `i`, que no está en ninguno de los dos; `q`, por ejemplo, es de `reveal`.

**3. Un parche del script que da paso a los botones** (`patches\data\subai\hero_formation_verify.vs`). El script del juego solo deja elegir formación cuando está abierto uno de sus cuatro menús, nombrados en una lista. Añade el tuyo:

```
@replace if (cmd_name != "form-front-menu")
if (cmd_name != "form-front-menu")
	if (cmd_name != "form-wedge-menu")
@end
```

Si otro mod parchea la misma línea para añadir otra formación, los parches se suman: cada `@replace` vuelve a escribir la línea del juego y deja la tuya detrás.

## Online

Los bonos cambian la jugabilidad: todos los jugadores deben llevar el mismo mod.
