# Objetos nuevos

Los objetos que llevan los héroes (amuletos, hierbas, gemas…) son datos. El juego trae 42 en `data\items.xml`, y un mod puede añadir los suyos
con un [parche](parches.md), sin compilar nada.

Ejemplo listo para copiar: [`ejemplos/objeto-ejemplo`](ejemplos/objeto-ejemplo).

## Camino rápido

El objeto del ejemplo, y que cada héroe lo reciba, sale con:

```
node herramientas/crear-objeto.js <carpeta del mod> "Amuleto de ejemplo" --dano 10 --armadura 5 --dar-heroes
```

Genera, dentro de la carpeta del mod:

- `patches\data\items.xml`: el objeto con todos sus atributos. Si el parche ya existe, lo añade sin tocar los que hay.
- Con `--dar-heroes`: `patches\data\classes\Hero.sc.xml` (un comportamiento más para la clase `Hero`) y `data\mod\dar_<id>.vs`, el script que da el objeto a cada héroe si aún no lo tiene. Si `Hero.sc.xml` ya existe, también lo añade.
- `mod.json`, solo si no había uno.

| Opción | Para qué |
|---|---|
| `--vida`, `--dano`, `--armadura`, `--nivel`, `--experiencia` | Lo que suma a quien lo lleva. `--armadura` pone `armor_slash` y `armor_pierce` |
| `--icono` | Un icono del juego, por ejemplo `"gameres/icons/items/Belt of might.bmp"`. Sin él, el de los dientes de oso |
| `--descripcion` | El texto de la ventana del objeto. Sin él, uno en inglés como `Adds 10 to damage and 5 to armor` |
| `--dar-heroes` | Que cada héroe reciba el objeto al aparecer |
| `--datos <DATA>` | Comprueba que ningún objeto del juego tenga ya ese id |

Repetir el comando con otro id añade otro objeto. Si el id ya existe en tu parche o en el juego, lo dice y no escribe nada.

No genera `use_script`, `equip_script` ni `kill_script`, ni `usecount` o `cursed`: añádelos a mano al `<item>` ([Un objeto con efecto propio](#un-objeto-con-efecto-propio)). Lo que genera es igual que el ejemplo.

## Lo que hace por dentro (cómo hacerlo a mano)

## Un objeto

Un parche de `data\items.xml`, en `patches\data\items.xml` dentro de tu mod:

```xml
<patch>
  <add select="/items">
    <item id="Amuleto de ejemplo"
          image="gameres/icons/items/Bear Teeth.bmp"
          sound="data/sound entities/ItemUse.xml"
          level="0"
          name="Amuleto de ejemplo"
          description="Adds 10 to damage and 5 to armor"
          important="no">
      <bonus health="0" damage="10" armor_slash="5" armor_pierce="5" level="0" experience="0"/>
    </item>
  </add>
</patch>
```

| Atributo | Para qué |
|---|---|
| `id` | El nombre con el que lo nombran los scripts (`AddItem("…")`). Único |
| `image` | El icono: uno del juego, o el tuyo dentro de `gameres/icons/items/` en tu mod (un BMP) |
| `name`, `description` | Lo que se lee en la ventana del objeto |
| `level` | Nivel mínimo del héroe para llevarlo (ningún objeto del juego lo usa distinto de 0) |
| `important` | `yes` o `no`. Si es un objeto especial (se enseña aparte) |
| `usecount` | Cuántas veces se usa: `1` para los de un solo uso, más para los de carga (`1000` de comida en el trigo) |
| `cursed` | `1` si no se puede quitar, `0` si sí. Es un número, no `yes`: el juego lo lee como número y cualquier texto vale 0, sin avisar. |

El `<bonus>` suma a quien lo lleva: `health`, `damage`, `armor_slash`, `armor_pierce`, `level` y `experience`. Con `health_percent`,
`damage_percent`, `armor_slash_percent` y `armor_pierce_percent` suma un porcentaje en vez de un número fijo.

`items.xml` acepta algunos atributos más (`customdata`, `location_script`, `object_script`, `attacheddie_script`), sin documentar; mira cómo los usan los objetos del juego.

## Un objeto con efecto propio

Un script que se ejecuta cuando el héroe lo usa:

| Atributo | Cuándo se ejecuta | Firma |
|---|---|---|
| `use_script="file://data/mod/mi.vs"` | Cuando el jugador usa el objeto | `// void, Obj owner` |
| `equip_script` | Al equiparlo | Mira los del juego |
| `kill_script` | Al matar a alguien con él | Mira los del juego |

Escribe la ruta con `file://`, como los objetos del juego (`file://data/ItemScripts/….vs`).

Los scripts del juego están en `data\ItemScripts`; para verlos, saca los datos con [`extraer-datos.js`](herramientas/extraer-datos.js). El de las
hierbas curativas:

```
//void, Obj owner

CreateFeedback("Heal", owner.AsUnit());
owner.Heal(owner.maxhealth);
ItemUsed(1);
```

`ItemUsed(n)` gasta `n` usos del objeto. Con `usecount="1"` desaparece al usarlo.

## Cómo conseguirlo en la partida

- Darlo con un script: `Obj::AddItem("id")` devuelve `true` si cabe en el inventario. El ejemplo lo hace en un comportamiento de la clase `Hero`
  (`patches\data\classes\Hero.sc.xml`), que corre con cada héroe que aparece. Comprueba antes que no lo tiene (`FindItem("id").IsValid`): al
  cargar una partida guardada el script vuelve a empezar.
- Soltarlo al matar: un evento [`OnObjectKill`](scripts.md) que llame a `AddItem` sobre el que mata, o a `DropItem` en el suelo.
- En ruinas y tesoros del mapa: mira `data\classes\BaseRuins.sc.xml`. Reparte objetos con líneas `itemtype` y `levelperitem`. En los datos del juego
  no lo usa ninguna otra clase, ni las tiendas de las civilizaciones.

Los objetos cambian la jugabilidad: en una partida online todos los jugadores llevan el mismo mod.
