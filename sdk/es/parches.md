# Parches

Un parche cambia una parte de un archivo del juego sin sustituirlo entero. Dos mods pueden parchear el mismo archivo y los dos cambios se suman. Si cada uno trajera su copia del archivo, el segundo pisaría al primero.

Valen archivos XML (órdenes, objetos, bonos, definiciones de unidades), INI (constantes, interfaz) y scripts `.vs` (IA, comportamientos). El archivo tiene que existir en el juego. Ejemplo para copiar: [`ejemplos/parches-ejemplo`](ejemplos/parches-ejemplo).


## Dónde va

En tu mod, dentro de `patches`, con la misma ruta que tiene el archivo en el juego:

```
mods\mi-mod\
  mod.json
  patches\data\commands\RBarracks.xml     parchea data\commands\RBarracks.xml
  patches\data\const.ini                  parchea data\const.ini
```

- Cada vez que el juego abre ese archivo, se le aplican los parches de los mods activos. Los originales no se tocan nunca.
- Con varios mods, se aplican del último al primero del [orden de carga](referencia.md#1-estructura-de-un-mod): el primero tiene la última palabra. Ese orden lo calcula el juego y es el mismo en todos los ordenadores con los mismos mods; la comprobación de multijugador no depende de él.
- Si tu mod trae además su propia copia del archivo (sin `patches`), los parches de los demás mods se aplican sobre ella.
- Para probar un parche sin crear un mod, ponlo en `<juego>\mod\patches\<ruta>`. Se aplica el último, por encima de todos.
- Un parche cambia la jugabilidad si el archivo la cambia (`data\...`, mapas, unidades). Entonces todos los jugadores tienen que llevar el mismo mod.
- Los parches se leen cuando el juego abre cada archivo. Si cambias uno, reinicia el juego.

## Ver cómo queda un archivo

Añade esto a `imperivm-mod.ini` y abre el juego:

```ini
[Debug]
Dump=data/commands/RBarracks.xml,data/const.ini
```

Cada archivo se copia a `mod-dump\<ruta>` tal como lo lee el juego, con los parches ya aplicados. Para ver el original, usa el perfil Original. Da el nombre exacto de cada archivo, separados por comas, y la línea entera no puede pasar de 1.023 caracteres. Un `*` solo vuelca todos los archivos que abre el juego: salen miles, úsalo con cuidado.

## Parches XML

Un `<patch>` con operaciones que se aplican en orden:

```xml
<patch>
  <set select="/commands/cmd[@name='trainRHastatus']" costgold="60" execdelay="3000" unset="costpop"/>
  <add select="/commands"><cmd name="miOrden" method="train" .../></add>
  <remove select="//cmd[@name='otraOrden']"/>
  <replace select="/commands/cmd[@name='trainRArcher']/src"><src obj="RBarracks"/></replace>
</patch>
```

- `<set select="…" atributo="valor" …/>`: cambia o añade esos atributos en los elementos elegidos. `unset="a b"` quita atributos.
- `<add select="…">…</add>`: añade lo de dentro al final del elemento elegido. Con `position="start"` lo pone al principio; con `"before"` o `"after"`, junto al elemento elegido en vez de dentro.
- `<remove select="…"/>`: quita los elementos elegidos.
- `<replace select="…">…</replace>`: cambia los elementos elegidos por lo de dentro.

Si lo que buscas puede no existir y no quieres error, pon `optional="yes"` en la operación.

### El selector

Una ruta como las de las carpetas, con condiciones entre corchetes. Escríbela siempre empezando por `/` o `//`: el validador da error si no.

| Selector | Elige |
|---|---|
| `/commands` | el `<commands>` raíz |
| `/commands/cmd` | todos los `<cmd>` dentro de `<commands>` |
| `/commands/cmd[@name='trainRHastatus']` | el `<cmd>` con ese `name` |
| `//cmd[@name='trainRHastatus']` | ese `<cmd>`, esté donde esté |
| `/commands/cmd[@key]` | los `<cmd>` que tienen atributo `key` |
| `/commands/cmd[3]` | el tercer `<cmd>` |
| `/commands/cmd[@name='x']/src` | los `<src>` de ese `<cmd>` |
| `//*[@name='x']` | cualquier elemento con `name="x"` |

Solo existen estas condiciones: `[@a='v']`, `[@a]` y `[n]`. Puedes poner varias seguidas (`[@a='v'][2]`). No hay `and`, `contains`, `text()` ni `..`. Las mayúsculas no cuentan, y los valores van entre comillas simples o dobles.

### Reglas

- El parche tiene que ser XML bien formado. En los valores, `&` se escribe `&amp;` y `<` se escribe `&lt;`. Si está mal formado no se aplica entero, y el error sale en el registro y en la ficha del mod.
- Una operación que falla (un selector que no encuentra nada) se salta y se avisa. Las demás se aplican.
- El parche no cambia la codificación: usa la del archivo del juego (casi siempre ASCII).
- El resultado pierde el sangrado de los atributos (quedan en una línea). El contenido es el mismo.
- Si quitas o renombras algo que usan otros archivos, se rompe. Las IA del juego llaman a las órdenes por su nombre (`trainRScout`): quita la orden y la IA fallará.

## Parches INI

Un INI con las secciones y claves que quieres cambiar:

```ini
; Cambia constantes de jugabilidad
[GamePlay]
DamageAmplify = 250
-TeachingLevel2

; En las listas de nombres sueltos (sin =), + añade una línea si no está
[GameOptions Objects]
+MiBotonNuevo
-RSText
```

- `Clave = valor` cambia o añade la clave en esa sección.
- `-Clave` quita la clave (o la línea que sea exactamente ese texto).
- `+Linea` añade una línea suelta a la sección si no está ya.
- Si la sección no existe, se crea al final del archivo.
- Quitar una clave que no existe da error en el registro.
- Los comentarios del final de línea del archivo original (`Clave = 20 ;comentario`) se pierden en la línea que cambias.

**No pongas comentarios después de una línea del parche.** El motor no los quita: `-Clave ; texto` busca una clave llamada así y falla, `[Seccion] ; texto` no se reconoce como sección y `Clave = 250 ; texto` guarda el comentario dentro del valor. Los comentarios van en líneas aparte que empiecen por `;`. El validador no avisa de esto.

## Parches de scripts

Un parche de un `.vs` (por ejemplo `patches\data\ai\GetTacticScript.vs`) es un texto con directivas que empiezan por `@` en la primera columna. El código que escribes debajo de cada una se añade al script del juego:

```
@before nRace = set.GetCentralBuilding.race;
// MOD: mi civilización
if (Mod_TrueRace(set.GetCentralBuilding) == Dacia)
	return TS_GaulTactic2;

@after player = set.player;
Mod_Log("GetTacticScript: eligiendo táctica");
```

`Dacia` no es una constante del juego: existe si hay una civilización activa con ese nombre (la `id` de su `civs\<id>.json`). Ver [Civilizaciones](civilizaciones.md).

| Directiva | Dónde pone el código |
|---|---|
| `@top` | Justo después de la primera línea del script (la de la firma). Solo vale para declarar variables o comentarios. |
| `@bottom` | Al final del script. |
| `@before TEXTO` | Antes de la primera línea del script que contiene `TEXTO`. |
| `@after TEXTO` | Después de esa línea. |
| `@replace TEXTO` | En lugar de esa línea. |
| `@end` | Cierra el bloque anterior. Es opcional: lo que siga hasta la próxima directiva se ignora. |

- `TEXTO` es un trozo de una línea del script, tal cual está escrito (no una expresión regular). Se ancla a la primera línea que lo contiene, así que elige un texto que aparezca una sola vez: si no, tu código acaba en un sitio que no esperabas. Si ninguna línea lo contiene, el error sale en el registro y la directiva se salta.
- En los scripts, las variables se declaran todas al principio y el código va después. Un `if` o un `return` con `@top` queda antes de las declaraciones y no compila (`Unexpected starting token`). Pon el código con `@before` y la primera línea de código del script.
- Los parches de varios mods se suman, así que dos mods pueden cambiar el mismo script (por ejemplo, dos civilizaciones que eligen su táctica en `GetTacticScript.vs`).
- El script resultante tiene que compilar. Si no, el error sale en `Logs\imperivm\imperivm-mod.log` como `script: compilation error`.
- El validador no revisa el código de estos parches, ni las funciones `Mod_*` ni la firma.

## Si algo no sale

Busca en `Logs\imperivm\imperivm-mod.log` las líneas con la etiqueta `patch`: dicen qué archivo se ha parcheado y qué operación falló. Los errores salen también en la ficha del mod. Los de compilación de un parche de script salen como `script: compilation error`.
