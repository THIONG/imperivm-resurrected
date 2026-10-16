# Plantillas de ciudad y aldea propias

Una civilización de mod empieza la partida con una ciudad y una aldea que el mapa aleatorio saca de una lista de plantillas del juego. Por
defecto usa las de su civilización base: una Dacia clonada de Galia nace en un «Gaul Town». Con plantillas propias cambias la distribución,
por ejemplo una ciudad más ancha, casi cuadrada, con otro reparto de edificios dentro del recinto.

`--recta` no funciona bien (ver más abajo).

## Qué se puede y qué no

- Puedes mover y repetir las piezas del juego: muros (`?WallsN/S/E/W/NE/NW/SE/SW`), torres, puertas y edificios.
- No puedes crear piezas de muro nuevas. Las esquinas y las puertas están dibujadas para el recinto octogonal del juego, así que la forma
  «cuadrada» es un octógono con los lados largos más largos.
- No hay terreno propio: el suelo es el del bioma (hierba o arena). La ciudad del juego trae un camino de tierra en el centro y la tuya no.
- Las plantillas solo salen para jugadores de esa civilización. Las aldeas y ciudades independientes del mapa no las usan.

## Cómo se hace

1. Consigue las plantillas del juego. En `imperivm-mod.ini` (carpeta del juego), sección `[Debug]`, pon
   `Dump=randommapsettlements/maps/1/map.obj.xml`. Abre el juego, empieza una escaramuza en un mapa aleatorio y sal. Aparece
   `mod-dump\randommapsettlements\maps\1\map.obj.xml`. Después quita esa línea.
2. Genera las plantillas de tu civilización:

   ```
   node herramientas\crear-plantilla.js "<carpeta del juego>" "<carpeta de tu mod>" Dacian --base Gaul --ancho 1
   ```

   - `Dacian` es el prefijo. Crea «Dacian Town 1», «Dacian Town 2» y «Dacian Village 1..5».
   - `--base` es la civilización de la que copia: Gaul, Republican Roman, Carthaginian, Iberian, Imperial Roman, Briton, Egyptian o German.
   - `--ancho N` (0 a 4) ensancha las ciudades N tramos de 220 unidades. Con Galia, `--ancho 1` da 1760 x 1760 y solo ensancha «Town 1»: «Town 2»
     no tiene la forma que hace falta, y el script lo avisa y la deja igual.
   - `--sin-aldeas` genera solo las ciudades.
   - `--recta` (experimental, con `--lado N`, de 6 a 9 tramos de 220 por lado, 8 por defecto) hace un recinto rectangular de verdad, sin las
     esquinas cortadas, con las puertas N, S y E en el centro de cada lado. La puerta sale torcida, porque el juego solo trae
     puertas dibujadas para las esquinas cortadas. No la uses salvo para experimentar.

   Escribe `patches\randommapsettlements\maps\1\map.obj.xml` en tu mod.
3. Díselo a la civilización: en `civs\<id>.json` añade `"template": "Dacian"` ([Civilizaciones](civilizaciones.md)).
4. Valida: `node herramientas\validar-mod.js <carpeta del mod>`. Avisa si falta el parche, si no hay ciudades o aldeas con ese prefijo y si alguna
   clase de las plantillas no existe.

Si la civilización no tiene plantilla de un tipo (por ejemplo, ninguna aldea), usa la de su base para ese tipo.

## Editar a mano

Cada `<settlement name="Dacian Town 1">` del XML es una lista de piezas con su clase y su posición (x, y). Reglas que conviene respetar:

- Mantén cada lado por debajo de unas 1900 unidades. La aldea satélite se coloca a unas 2000 del centro de la ciudad, y una ciudad mayor se
  solaparía con ella.
- Toda clase del XML tiene que existir, o el mapa aleatorio no se genera y el juego no avisa. Usa solo clases del juego o de tu mod. El validador
  lo comprueba.
- Las torres van cada 220 unidades, y cada trozo de muro en el punto medio entre dos torres. Una puerta ocupa dos huecos.
- Deja las plantillas en la zona libre del plano (y entre 24192 y 27904) y separadas al menos 3000 unidades entre sí.
- Los parches de `randommapsettlements/` cuentan como jugabilidad: en una sala online, todos tienen el mismo mod.
