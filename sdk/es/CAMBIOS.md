# Cambios

Versión del SDK y qué mods se pueden hacer con cada una. La versión de la API de scripts (`Mod_ApiVersion()`) va aparte: ahora es la 7.

## 1.6

Necesita Imperivm Resurrected 0.9.0 o posterior.

- Una variante que sustituye a su base (`"replacesBase": true`) puede cambiar con `letter` y `template` la letra y las plantillas de
  ciudad y aldea de esa base durante toda la partida, también las de los poblados neutrales del mapa aleatorio. Así se sustituye una
  civilización entera del juego ([Civilizaciones](civilizaciones.md#variantes)). El validador busca las plantillas también en el
  `packs\randommapsettlements.bfhp` del mod.
- `modes\<nombre>.json` fija los ajustes de la sala y las civilizaciones de un modo de victoria ([Modos de victoria](victoria.md)).
- `rules\game.json` fija reglas del motor para todas las partidas del mod: el ejército no se separa del héroe en las murallas, los
  centinelas no se pueden seleccionar y hay clases que no se pueden atacar ([Opciones](opciones.md)).
- Un script de bonificación numerado del `004` al `099` en `data\BonusScripts` aparece en la sala, con el nombre y la descripción de sus líneas `//@` ([Bonificaciones iniciales](bonificaciones.md)).
- El validador conoce `Mod_PlayerCiv` y `Mod_CanExecCmd`.

## 1.5

Necesita Imperivm Resurrected 0.9.0 o posterior.

- `reducir-mod.js` convierte también los mods cuyo `rle.mmp` cambia gráficos del juego, no solo los que añaden gráficos al
  final. El `rle.mmp` del juego se queda entero: lo que el mod cambia o añade va detrás, en `rle.mmp.tail`, y sus imágenes se
  ajustan para encontrarlo en su sitio nuevo. Las imágenes del juego que el mod no sustituye se siguen viendo como en el juego.
- `reducir-mod.js` respeta mejor lo que enseñaba el juego con el mod antiguo: un archivo suelto de `Currentlang\` gana al del
  paquete de idioma, y un archivo suelto igual al de los paquetes del juego se conserva si algún idioma enseñaba otro.
- El validador ya no da error por dos órdenes con el mismo nombre para edificios distintos, porque el juego une cada una a los
  edificios de su `<src>`; solo avisa si las dos son para el mismo edificio. Tampoco pide la firma en la primera línea de los
  modos de victoria (`data\GameScripts`) ni de los scripts de objetos: el juego se la da.

## 1.4

Necesita Imperivm Resurrected 0.9.0 o posterior.

- Un mod puede salir en varias categorías de Explorar contenido: la principal es la de GameBanana y las demás se añaden con
  etiquetas `Type`, por ejemplo `Type: Civilizations`.
- Las etiquetas llevan título, como en GameBanana: `Players: 2-4` y `Language: Spanish`. Las formas sin título siguen
  funcionando y el validador pide la nueva.
- En el juego, las categorías se llaman igual que en GameBanana: **Jugabilidad y balance** (antes Reglas y balance),
  **Gráficos** (antes Gráficos e interfaz) y **Traducciones** (antes Textos y traducciones).

## 1.3

Necesita Imperivm Resurrected 0.9.0 o posterior.

- Las categorías de mapas se llaman como las carpetas del juego: **Scenarios** (Escenarios), **Adventures** (Aventuras) y **Conquests**
  (Conquistas), en lugar de Maps, Campaigns y Conquest. Los nombres anteriores siguen funcionando y el validador pide el nuevo.
- Un `.bfhp` suelto en la raíz del `.zip` se coloca al instalarlo en `scenarios\`, `adventures\` o `conquests\` según el tipo que dice el
  propio mapa (`game_type`), y la categoría solo decide si un mapa normal es una misión de Adventures. El validador avisa de los mapas que
  están en una carpeta que no les toca.
- El audio se publica en la sección **Sounds** de GameBanana, como pide GameBanana, y Explorar contenido lo enseña en Audio y voces. Lo
  que se instala desde ahí guarda su número en `sources.gamebananaSound`.

## 1.2

Necesita Imperivm Resurrected 0.9.0 o posterior.

- `Mod_CanExecCmd(objeto, orden)` (API 7): la misma comprobación que hace el botón de una orden (la tiene, no está deshabilitada y su
  verificador la acepta). El menú de trucos lista las tropas de las clases cargadas, también las de los mods (`edittree_pos` bajo
  `Units/`), e investiga lo que permiten los botones, en el orden de sus requisitos y sin escoger dos opciones de una elección.
- Civilizaciones: `"variant": true` declara una variante de una civilización del juego, con sus mismas clases y otro nombre en la sala;
  con `"replacesBase": true` ocupa el sitio de su base en la sala y en el sorteo aleatorio.
  `GetPlayerRace` da la base y `Mod_PlayerCiv(jugador)` (API 7) la elegida ([Civilizaciones](civilizaciones.md#variantes)). La interfaz
  de una civilización de mod (barras, esquinas, iconos rápidos, pantalla del final) es la de su base, no la de Galia. La constante de los
  scripts respeta las mayúsculas del `id` (`RomeImperial`).
- Un `.bfhp` de `packs\` que trae un mod (por ejemplo `RandomMapSettlements.bfhp`, las ciudades iniciales del mapa aleatorio) se usa
  de verdad: el juego lo abre para escritura y antes se quedaba con el suyo.
- Los archivos sueltos de `Local\<idioma>\` (textos, ayuda, consejos) se usan siempre: antes se perdían si el juego los leía antes de
  montar los mods.
- Mods antiguos: `reducir-mod.js` respeta el orden de sus paquetes, pone en `hidden.txt` lo que quitaban del juego, escribe `rle.mmp.tail` si amplían el `rle.mmp` y `Local\<idioma>.pak` para lo de un idioma fuera de `currentlang\`, y pasa `data\const.ini` a parche sin `[Resolutions]`. Opciones nuevas `--fuera` y `--ocultar`. Las cinemáticas se buscan también en los paquetes.
- Nueve categorías de GameBanana en tres tipos: Gameplay & Balance, Units & Heroes, Civilizations · Maps, Campaigns, Conquest · Graphics, Audio & Voices, Translations. `Scenarios`, `Interface` y `Total Conversions` siguen valiendo como Maps, Graphics y Gameplay & Balance, pero el validador pide el nombre nuevo.
- Las etiquetas solo sirven para los filtros de jugadores (`4 players`, `2-4 players`, en mapas) e idioma (`Spanish`…, en campañas, traducciones y audio). `Gameplay`, `Cosmetic` y `Online tested` ya no hacen falta. El validador avisa de un mapa sin jugadores y de una campaña, traducción o audio sin idioma, y una etiqueta desconocida ya es un aviso, no un error.
- El gestor ya no tiene botón Publicar: el zip se hace a mano (ver [Publicar](publicar.md)).
- El orden de los mods lo calcula el juego ([Referencia](referencia.md#1-estructura-de-un-mod)): ya no se ordena a mano en el perfil, y la comprobación de multijugador no depende del orden. La clave de `imperivm-mod.ini` es `[Mods] Profile`.

## 1.1

Necesita Imperivm Resurrected 0.9.0 o posterior.

- Los nombres de los campos de `mod.json`, `civs\*.json`, `specials\*.json` y `options\*.json` pasan a inglés (`name`, `authors`, `description`, `tags`, `category`, `index`, `biome`, `template`, `letter`, `values`, `default`…). Los nombres en español siguen valiendo.
- Las directivas de los parches de scripts (`.vs`) pasan a `@top`, `@bottom`, `@before`, `@after`, `@replace` y `@end`. Las antiguas siguen valiendo.
- Las claves de `imperivm-mod.ini` pasan a inglés (`[Debug]`, `Dump`, `LogFiles`, `[Mods] Override`…). El juego renombra las antiguas al arrancar.
- El registro pasa a `Logs\imperivm\imperivm-mod.log` (el juego borra al arrancar los archivos sueltos de `Logs\`). Sus líneas llevan un nivel (`I`, `W`, `E`) y una etiqueta (`patch`, `language`, `civilizations`, `specials`, `script`, `event`) en lugar de un prefijo de texto como `Patch:`, y se guardan las cinco últimas sesiones. `LogLevel` y `LogChannels` en `[Debug]` de `imperivm-mod.ini` piden más detalle. El corazón del menú principal crea un informe con los registros para compartir.

## 1.0

Necesita Imperivm Resurrected 0.9.0 o posterior.

Un mod puede, solo con archivos y sin compilar nada:

- Cambiar cualquier archivo XML, INI o script del juego con parches, y sumar los cambios de varios mods.
- Añadir unidades, con sus estadísticas, y dibujos propios convertidos desde PNG.
- Añadir habilidades de tropas y de héroes, y especialidades.
- Añadir civilizaciones, con su ciudad y su aldea iniciales, su IA y sus héroes.
- Añadir fortines neutrales.
- Añadir objetos.
- Añadir modos de victoria, con sus límites.
- Añadir opciones de partida a la sala: interruptores y listas de valores.
- Añadir tipos de mapa aleatorio.
- Añadir formaciones de héroe.
- Añadir idiomas y textos, órdenes y botones propios.
- Reaccionar a lo que pasa en la partida con eventos de script y funciones `Mod_*`.

Herramientas: validar un mod, extraer los datos del juego, crear una unidad, un objeto, una habilidad, una formación o una opción de partida, clonar una civilización, crear un fortín, plantillas de ciudad, tipos de mapa, convertir imágenes y sprites, crear la entidad de una unidad o edificio, adaptar un mod antiguo.
