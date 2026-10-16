# Cambios

Versión del SDK y qué mods se pueden hacer con cada una. La versión de la API de scripts (`Mod_ApiVersion()`) va aparte: ahora es la 6.

## 1.2

Necesita Imperivm Resurrected 0.32.2 o posterior.

- Nueve categorías de GameBanana en tres tipos: Gameplay & Balance, Units & Heroes, Civilizations · Maps, Campaigns, Conquest · Graphics, Audio & Voices, Translations. `Scenarios`, `Interface` y `Total Conversions` siguen valiendo como Maps, Graphics y Gameplay & Balance, pero el validador pide el nombre nuevo.
- Las etiquetas solo sirven para los filtros de jugadores (`4 players`, `2-4 players`, en mapas) e idioma (`Spanish`…, en campañas, traducciones y audio). `Gameplay`, `Cosmetic` y `Online tested` ya no hacen falta. El validador avisa de un mapa sin jugadores y de una campaña, traducción o audio sin idioma, y una etiqueta desconocida ya es un aviso, no un error.
- El gestor ya no tiene botón Publicar: el zip se hace a mano (ver [Publicar](publicar.md)).
- El orden de los mods lo calcula el juego ([Referencia](referencia.md#1-estructura-de-un-mod)): ya no se ordena a mano en el perfil, y la comprobación de multijugador no depende del orden. La clave de `imperivm-mod.ini` es `[Mods] Profile`.

## 1.1

Necesita Imperivm Resurrected 0.31.58 o posterior.

- Los nombres de los campos de `mod.json`, `civs\*.json`, `specials\*.json` y `options\*.json` pasan a inglés (`name`, `authors`, `description`, `tags`, `category`, `index`, `biome`, `template`, `letter`, `values`, `default`…). Los nombres en español siguen valiendo.
- Las directivas de los parches de scripts (`.vs`) pasan a `@top`, `@bottom`, `@before`, `@after`, `@replace` y `@end`. Las antiguas siguen valiendo.
- Las claves de `imperivm-mod.ini` pasan a inglés (`[Debug]`, `Dump`, `LogFiles`, `[Mods] Override`…). El juego renombra las antiguas al arrancar.
- El registro pasa a `Logs\imperivm\imperivm-mod.log` (el juego borra al arrancar los archivos sueltos de `Logs\`). Sus líneas llevan un nivel (`I`, `W`, `E`) y una etiqueta (`patch`, `language`, `civilizations`, `specials`, `script`, `event`) en lugar de un prefijo de texto como `Patch:`, y se guardan las cinco últimas sesiones. `LogLevel` y `LogChannels` en `[Debug]` de `imperivm-mod.ini` piden más detalle. El corazón del menú principal crea un informe con los registros para compartir.

## 1.0

Necesita Imperivm Resurrected 0.29.5 o posterior.

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
