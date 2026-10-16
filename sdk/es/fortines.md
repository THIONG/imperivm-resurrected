# Fortines y edificios de defensa

«Fortín» se usa para dos cosas distintas:

- Los fortines neutrales del mapa: los puestos que salen repartidos y se capturan («Fortín de comercio», «Fortín de reclutamiento»…). Un mod puede
  añadir tipos nuevos con una herramienta y salen solos en los mapas aleatorios.
- Los edificios de defensa de las ciudades: torres, muros y puertas.

## Un fortín neutral nuevo

`herramientas/crear-fortin.js` copia un fortín del juego con otro id y nombre y lo mete en los mapas aleatorios. Necesita la carpeta `DATA`, que
sacas con [`extraer-datos.js`](herramientas/extraer-datos.js) (`node extraer-datos.js "<carpeta del juego>" <salida>`):

```
node crear-fortin.js <salida>\DATA <carpeta del mod> ArcherFort IOutpost "Archer fort"
```

Escribe la clase (`data\classes\ArcherFort.sc.xml`), el parche de `data\randommap\placeobjects.xml` que lo añade a los tres tamaños de mapa (con las
cantidades de la base) y el nombre en español. Con `GOutpost`, `TOutpost` o `ROutpost` como base escribe además `patches\data\commands\*.xml`, para que
el fortín nuevo tenga las órdenes del puesto (vender comida, interés).

Las bases posibles:

| Base | Tipo |
|---|---|
| `GOutpost`, `TOutpost` | Fortín de comercio |
| `ROutpost`, `EOutpost` | Fortín del oro (interés) |
| `IOutpost`, `BOutpost` | Fortín de adiestramiento |
| `COutpost` | Fortín de reclutamiento |

Después editas la clase como quieras: `maxhealth`, la guarnición (`defender_cls_1`, `defenders_max_1`, `start_level_1`…), su comportamiento
(`<behavior script=…>`), lo que cuesta capturarlo. Ejemplo: [`ejemplos/fortin-ejemplo`](ejemplos/fortin-ejemplo).

Para cambiar cuántos salen, edita `min` y `max` de las líneas del parche (cuántos pone el generador por mapa). `mindist` es la distancia mínima
entre fortines, en píxeles. Los demás números son los del juego.

El modelo (`entity`) y el icono son los de la base. Para un modelo con sprites propios mira [Gráficos propios](graficos.md): lleva una sección de
edificios.

## Torres, muros, puertas y puestos

Son clases de edificio como las demás. Lo que conviene saber antes de añadir las tuyas:

- Un asentamiento (ciudad, aldea, puesto) es un grupo de edificios con dueño, población, oro y comida. Su tipo sale de la clase del edificio
  central: `BaseTownhall` es una ciudad, `BaseVillage` una aldea, `Outpost` un puesto.
- El juego base no deja construir edificios. No hay ninguna orden de construir (solo barcos y máquinas de asedio). Torres, muros, puertas y puestos
  salen del mapa: la plantilla de ciudad, los puestos del mapa aleatorio, los escenarios y el editor.
- Las plantillas de ciudad nombran clases concretas (`GTownhall`, `GTower01`, `GWallsN`, `GGateNW`…). Una civilización nueva usa las suyas si las
  clona con la herramienta de [Civilizaciones](civilizaciones.md).

| Clase base | Qué es |
|---|---|
| `Tower` (`CVXBuilding`) | Torre. Sus órdenes (`Tower Attack`, reparar) cuelgan de `Tower`, y una hija las hereda |
| `Wall` (`CVXBuilding`) | Muro. `num_sentry_slots` y `sentry_class_name` dicen cuántos centinelas lo patrullan y de qué clase |
| `Gate` (`CVXGate`) | Puerta: abrir, cerrar, permanente, reparar |
| `Outpost` (`CVXTownHall`) | Base de los puestos avanzados. Los concretos (`GOutpost`, `GGuardPost`…) usan `CVXOutpost` y cuelgan de `Outpost`: son independientes hasta que se capturan y cada uno hace algo con un script (vender comida, interés, experiencia…) |
| `GGuardPost` | Puesto de guardia, solo de Galia: 12 centinelas por script. No sale en `placeobjects.xml`, y `crear-fortin.js` lo avisa |

Para una clase nueva: un `.sc.xml` en `data\classes\` con `parent=` una de las de arriba, un `cpp_class` que ya exista (`CVXBuilding`, `CVXGate`,
`CVXOutpost`, `CVXTownHall`) y su `entity`. Copia el `entity` de la clase de la base, porque el juego no lo hereda del padre. El comportamiento
propio va con `<behavior>` y `<method sig="…" vs="…">`, y las órdenes con `<cmd>`, igual que en [Habilidades](habilidades.md). Un tipo de edificio
con una `cpp_class` nueva no se puede.

## Dónde colocarlo

| Sitio | Cómo |
|---|---|
| Mapa aleatorio | Añadiéndolo a `placeobjects.xml`, que es lo que hace `crear-fortin.js` |
| Ciudad o aldea inicial de una civilización | Las clases clonadas salen solas en la ciudad inicial. Para cambiar la distribución, [Plantillas](plantillas.md) |
| El editor de mapas | Con `edittree_pos` en la clase. El editor tiene carpetas fijas por civilización: las de una civilización nueva salen mezcladas con las de su base |
| Un mapa a mano | Lo coloca el editor y el mapa nombra la clase (necesita el mod) |

## Multijugador y límites

Cambia la jugabilidad: todos los jugadores llevan el mismo mod.

- No hay carpetas ni conjuntos de muros propios en el editor.
