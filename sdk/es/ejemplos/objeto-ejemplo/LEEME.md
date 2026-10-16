# Ejemplo: objeto nuevo

Añade un objeto, el Amuleto de ejemplo (+10 de daño y +5 de armadura), y hace que cada héroe lo reciba al aparecer en el mapa.

- `patches\data\items.xml`: el objeto, añadido a la lista del juego. Cada `<item>` tiene un `id` (el nombre con el que lo nombran los scripts), un
  icono (`image`), un texto (`description`) y sus bonos (`<bonus>`: vida, daño, armaduras, nivel, experiencia). Un objeto con efecto propio lleva
  además `use_script="file://data/mod/mi.vs"`; los del juego están en `data\ItemScripts` (sácalos con `extraer-datos.js`, ver
  [Parches](../../parches.md)).
- `patches\data\classes\Hero.sc.xml`: un comportamiento más para la clase `Hero`.
- `data\mod\dar_amuleto.vs`: lo que hace ese comportamiento, `Obj::AddItem("Amuleto de ejemplo")`.

Si el
juego no encuentra el icono o el script falla, lo dice `Logs\vxlog.log`.

Para probarlo: copia esta carpeta a `<juego>\mods\`, actívala en el gestor de contenido, reinicia y empieza una escaramuza con cualquier héroe. En
`Logs\imperivm\imperivm-mod.log` debe salir «Un heroe ha recibido el Amuleto de ejemplo», y el héroe tendrá el objeto en su inventario.

Otras formas de dar un objeto: `Obj::AddItem` desde cualquier script (por ejemplo un `OnObjectKill` que lo suelte al matar), o ponerlo en las
ruinas y tesoros del mapa (mira `data\classes\BaseRuins.sc.xml`). Guía: [Objetos nuevos](../../objetos.md).
