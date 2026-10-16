# Ejemplo: civilización

Dacia: una civilización nueva (índice 10, letra `D`) clonada de Galia con [`herramientas/clonar-civilizacion.js`](../../herramientas/clonar-civilizacion.js).

```
node extraer-datos.js "<carpeta del juego>" <salida>
node clonar-civilizacion.js <salida>\DATA <carpeta del mod> Dacia D Gaul 10
```

`extraer-datos.js` saca `data.pak` a `<salida>\DATA`, que es la carpeta que pide la herramienta.

| Archivo | Qué es |
|---|---|
| `civs\dacia.json` | La declaración de la civilización. Lleva `"template": "Dacian"`, que se añade a mano. |
| `data\classes\D*.sc.xml` | Una copia de cada clase de Galia (`DSwordsman`, `DBarracks`, `DVillage`…) con otro id y `race="Dacia"`. Son copias enteras, no hijas: el juego no hereda del padre la entidad (el modelo) ni los sonidos. |
| `data\commands\Dacia.xml` | Las órdenes de Galia que nombran sus clases (`trainGSwordsman Dacia`, `trainpeasantd`, `hireheroD`…), apuntando a las clases nuevas. |
| `patches\data\commands\*.xml` | Añaden las clases nuevas a las órdenes compartidas (reparar, mejoras del cuartel, herrería…). |
| `patches\randommapsettlements\maps\1\map.obj.xml` | Las plantillas «Dacian Town» y «Dacian Village» (ciudad ensanchada). Las hace `crear-plantilla.js`; ver [Plantillas](../../plantillas.md). |

Dacia sale en los desplegables de la sala de multijugador. Cambia la jugabilidad: todos los jugadores llevan el mod. En `Logs\imperivm\imperivm-mod.log`
(líneas con la etiqueta `civilizations`) se ve que se registra y, con `LogChannels=civilizations:debug` en `[Debug]` de `imperivm-mod.ini`, qué jugador juega con ella.

Ver [Civilizaciones](../../civilizaciones.md).
