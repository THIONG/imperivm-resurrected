# Ejemplo: formación nueva

Añade una formación para los héroes, la **Cuña** (+3 de daño y +1 de armadura; la caballería delante). Se elige como las demás, en el botón de formación del héroe.

- `patches\data\formations.xml`: la formación (tamaño, bonos y dónde se coloca cada clase de unidad).
- `patches\data\commands\Hero.xml`: sus dos botones, `form-wedge-menu` (el que se ve cuando es la actual) y `form-wedge` (el que la elige, tecla `i`). Usan el dibujo de la formación Front.
- `patches\data\subai\hero_formation_verify.vs`: parche del script del juego, que solo deja elegir formación con sus cuatro menús; añade el nuevo.

Los bonos valen solo con las tropas paradas ("stand ground"). El texto del botón lo pone el juego; el `Description` del XML no se muestra.

Para probarlo, copia esta carpeta a `<juego>\mods\`, actívala en el gestor de contenido, reinicia, empieza una escaramuza, pon tropas a las órdenes de un héroe y abre su menú de formaciones. Guía: [Formaciones](../../formaciones.md).
