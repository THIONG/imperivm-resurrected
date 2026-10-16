# Idiomas y textos

Un mod puede cambiar o añadir textos del juego sin sustituir el archivo de textos entero, y puede añadir un idioma nuevo (francés, alemán, polaco…) que sale en **Opciones → Idioma**. También puede traer archivos que solo se usen con un idioma: ayuda, consejos, imágenes del menú, voces.

Ejemplos para copiar: [`ejemplos/textos-propios`](ejemplos/textos-propios) e [`ejemplos/idioma-frances`](ejemplos/idioma-frances).

## Cómo funciona el idioma del juego

El idioma se elige en Opciones → Idioma y se aplica al reiniciar. Se guarda en `Settings.ini`, `[Language] Default=`.

Los textos del juego están en `currentlang/game.loc.xml`, dentro de `local\<idioma>.pak`. Cada texto es una línea así:

```xml
<translationtableentry text="Single player@/Menu/mainmenu.ini:SinglePlayer:Text" justtext="Single player" result="Estrategia"/>
```

`text` es la clave: el texto original en inglés, a veces seguido de `@` y un contexto (dónde se usa). `result` es lo que se ve en ese idioma. Si un texto no tiene traducción, el juego enseña la clave, o sea, el inglés. Por eso el inglés casi no necesita archivo.

La interfaz del propio Imperivm Resurrected (gestor de contenido, sala, Controles, partidas guardadas, panel de rendimiento) viene en español, inglés e italiano. Con el juego en italiano sale en italiano; con cualquier otro idioma, en inglés. Los textos de tu mod (nombre, descripción, órdenes) sí se traducen con los archivos `loc\` de más abajo.

Las cinemáticas y la música son iguales en todos los idiomas. Cambian los textos, las voces de las unidades, la ayuda, los consejos y algunas imágenes del menú.

Para ver el archivo de textos, añade esto a `imperivm-mod.ini` y abre el juego:

```ini
[Debug]
Dump=currentlang/game.loc.xml
```

Se copia a `mod-dump\currentlang\game.loc.xml` con los textos de tus mods ya sumados. Para ver el original, usa el perfil Original.

## Cambiar o añadir textos

Pon los textos en `loc\<idioma>.loc.xml` dentro de tu mod. Si son muchos, repártelos en varios `*.loc.xml` dentro de `loc\<idioma>\`:

```
mods\mi-mod\
  mod.json
  loc\spanish.loc.xml
  loc\italian.loc.xml
  loc\english\unidades.loc.xml
```

`<idioma>` es el nombre del idioma en inglés y en minúsculas, como en `Settings.ini`: `spanish`, `english`, `italian`, `french`, `german`…

```xml
<translationtable>
  <!-- Cambia el texto en todos los sitios donde sale -->
  <translationtableentry text="Single player" result="Un jugador"/>

  <!-- Solo en un sitio: la clave completa, copiada del game.loc.xml del juego -->
  <translationtableentry text="Options@/Menu/endgamemenu.ini:Options:Text" result="Ajustes"/>

  <!-- Lo mismo escrito con context -->
  <translationtableentry text="Options" context="/Menu/endgamemenu.ini:Options:Text" result="Ajustes"/>

  <!-- Texto nuevo, p. ej. el nombre de una unidad de tu mod (display_name en su .sc.xml) -->
  <translationtableentry text="Gladiator" result="Gladiador"/>
</translationtable>
```

- Sin contexto (`text="Single player"`), el cambio vale en todos los sitios donde el juego usa ese texto. Con contexto (`text="…@…"` o el atributo `context`), solo en ese sitio.
- Salto de línea: `\n` dentro del texto, como en los archivos del juego.
- En un texto, `&` se escribe `&amp;`, `<` se escribe `&lt;` y las comillas dobles `&quot;` (o usa comillas simples para el valor).
- Guarda en UTF-8. Si está en ANSI, el mod lo convierte, pero UTF-8 es lo seguro.
- Una línea mal escrita se salta y se apunta en `Logs\imperivm\imperivm-mod.log` con su número de línea. El resto del archivo se carga.
- `<juego>\mod\loc\<idioma>.loc.xml` sirve para probar sin crear un mod, y gana a todos los mods.
- Si dos mods cambian el mismo texto, debería ganar el que va más arriba en el perfil.
- Los textos se leen al arrancar: reinicia después de cambiarlos.

Los nombres de unidades, edificios y órdenes de tu mod se escriben en inglés en sus archivos (`display_name`, `rollover`…). Esa es la clave. Para traducirlos, añade su traducción en `loc\spanish.loc.xml`, `loc\italian.loc.xml`, etc.

## Añadir un idioma nuevo

1. Declara el idioma en `mod.json`:

   ```json
   "language": {"code": "french", "name": "Français", "base": "spanish"}
   ```

   - `code`: el nombre del idioma en inglés y en minúsculas. Es el que se guarda en `Settings.ini`.
   - `name`: lo que se ve en el selector.
   - `base` (opcional, por defecto `english`): el idioma del que se toma lo que tu mod no traiga. Mira el aviso de más abajo antes de dejar el inglés.

2. Traduce los textos en `loc\french.loc.xml` (o en varios archivos dentro de `loc\french\`). Lo que no traduzcas sale en inglés.

3. Opcional: archivos del idioma en `Local\french\`, con las rutas que tienen dentro de `currentlang\`. Los que pide el juego son:

   ```
   Local\french\help.xml              ayuda (F1)
   Local\french\tips.xml              consejos de la pantalla de carga
   Local\french\credits.txt           créditos
   Local\french\splash.ini            pantallas de inicio
   Local\french\menu_16_9.bmp         fondo del menú (también menu_4_3.bmp, menubackground.bmp, logo.bmp)
   Local\french\voices\<unidad>\1.wav voces de las unidades (1.wav hasta 5.wav según la unidad)
   ```

   También puedes empaquetarlo todo en `Local\french.pak`.

4. Activa el mod en tu perfil, elige **Français** en Opciones → Idioma y reinicia.

**Qué trae el idioma base.** Lo que tu idioma no traiga se toma del idioma `base`. El inglés no trae ayuda (`help.xml`), consejos (`tips.xml`), `splash.ini` ni imágenes de menú. Un idioma nuevo con base inglés se queda sin esos archivos. El español y el italiano sí los traen: con `"base": "spanish"` o `"italian"` los tienes, o los traes tú en `Local\<idioma>\`.

Los mapas traen sus propios textos por idioma. Si un mapa no trae el tuyo, se usan los del idioma base.

**Letras.** Las fuentes del juego deberían cubrir latín (con los acentos europeos), griego y cirílico. Chino, japonés y coreano no se pueden mostrar sin fuentes nuevas.

En multijugador el idioma no importa: jugadores con idiomas distintos pueden jugar juntos.

## Archivos para un solo idioma

Cualquier archivo de `currentlang\` se puede cambiar solo para un idioma con la carpeta `Local\<idioma>\` de tu mod. `Local\spanish\help.xml` cambia la ayuda solo en español. Si pones el archivo en `currentlang\help.xml` (fuera de `Local\`), vale para todos los idiomas.

## Si algo no sale

Busca en `Logs\imperivm\imperivm-mod.log` las líneas con la etiqueta `language`. Las de aviso (`W`) señalan las líneas con errores. Para ver también qué archivos de textos se han leído y qué archivos de `Local\<idioma>\` se usan, pon `LogChannels=language:debug` en `[Debug]` de `imperivm-mod.ini`.
