# Ejemplo: un fortín nuevo

Un fortín neutral nuevo, el Fortín de arqueros, de los que salen repartidos por el mapa y se capturan. Aparece en los mapas aleatorios junto a los
del juego (tantos como el Fortín de adiestramiento ibero), tiene 15000 de vida y una guarnición de 16 arqueros galos, con nombre y descripción en español.

Se hizo con un comando. La carpeta `DATA` sale de `node extraer-datos.js "<carpeta del juego>" <salida>`:

```
node herramientas/crear-fortin.js <salida>\DATA ejemplos/fortin-ejemplo ArcherFort IOutpost "Archer fort"
```

| Archivo | Qué es |
|---|---|
| `data\classes\ArcherFort.sc.xml` | El fortín: copia del Fortín de adiestramiento ibero (`IOutpost`) con otro id, nombre, vida y guarnición (editado a mano después). |
| `patches\data\randommap\placeobjects.xml` | Lo añade a la lista de puestos del mapa aleatorio, para los tres tamaños de mapa, con las cantidades de la base. |
| `loc\spanish.loc.xml` | El nombre y la descripción en español. |

Para probarlo: copia esta carpeta a `<juego>\mods\`, actívala en el gestor de contenido, reinicia y juega un mapa aleatorio. Los fortines salen dispersos
por el mapa, juegue la civilización que juegue. En el editor sale en *Structures / Outposts / Archer fort*.

Cambia la jugabilidad: todos los jugadores llevan el mod. Guía: [Fortines y edificios](../../fortines.md).
