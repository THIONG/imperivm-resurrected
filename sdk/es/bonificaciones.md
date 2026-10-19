# Bonificaciones iniciales

Una bonificación inicial es un script que el juego ejecuta para un jugador al empezar la partida. La sala las lista en la columna "Bonif." de cada jugador, y un mod puede añadir las suyas.

## El script

Va en `data\BonusScripts\` del mod, con el nombre `NNN NOMBRE.vs`:

```
data\BonusScripts\004 Tributo.vs
```

- El número ordena la lista y es el valor que la sala envía a los demás jugadores. En la sala solo aparecen los números del `000` al `099`; del `100` en adelante son scripts que arrancan otros scripts (las conquistas del juego los usan) y nunca se ofrecen.
- `000 None.vs` debe existir y estar vacío: es la opción "sin bonificación". El juego trae del `000` al `003`; una bonificación nueva toma el siguiente número libre.
- El nombre de la lista es el del archivo sin el número, con mayúscula solo al principio de cada palabra (`004 BOAR TEETH.vs` es "Boar Teeth").
- La firma es fija: `//void, Settlement set, int playerid`. El script se ejecuta una vez por cada jugador que la eligió, con su asentamiento principal.

Ejemplo, 3000 de oro al empezar:

```
//void, Settlement set, int playerid
//@name 3000 gold
//@name.es 3000 de oro
//@description 3000 gold at the start.
//@description.es 3000 de oro al empezar.

Sleep(100);
set.SetGold(set.gold + 3000);
set.GoldConverted(3000);
```

## Nombre y descripción en la sala

Las líneas que empiezan por `//@` al principio del script dan el texto del desplegable. Tienen que ir antes de la primera línea de código.

| Línea | Uso |
|---|---|
| `//@name Texto` | El nombre que se ve en el desplegable y en la fila del jugador (corto: la columna es estrecha). |
| `//@description Texto` | Lo que hace la bonificación. Se muestra junto al desplegable cuando el puntero está sobre ella. |
| `//@name.es`, `//@description.it`… | Lo mismo para un idioma (`en`, `es`, `it`). Si el idioma del jugador no tiene línea, se usa la línea sin idioma y después la inglesa. |

Sin estas líneas el nombre es el del archivo y no hay descripción. Las tres bonificaciones del juego tienen nombre y descripción propios mientras su script es el del juego; un mod que cambie el efecto de una de ellas recibe el nombre del archivo, así que debe añadir estas líneas. Si dos bonificaciones acaban con el mismo nombre, la sala añade el número del archivo para distinguirlas.

## En línea

Todos los jugadores necesitan los mismos archivos: el contenido de la sala se compara antes de la partida, y el valor de la bonificación es una posición de la lista.

Necesita Imperivm Resurrected 0.9.0 o posterior.
