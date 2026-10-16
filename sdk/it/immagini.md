# Immagini proprie

Una mod può portare i suoi pulsanti dei comandi e ritratti delle unità invece di riusare quelli del gioco. Disegni con il programma che vuoi, salvi un PNG e lo converti con uno strumento. Serve [Node.js](https://nodejs.org).

Esempio da copiare: [`esempi/esempio-abilita`](esempi/esempio-abilita) (il pulsante di «Respiro»).

## Convertire un PNG

```
node strumenti/converti-immagine.js <entrada.png> <salida.bmp> --tipo pulsante
node strumenti/converti-immagine.js <entrada.png> <salida.bmp> --tipo icona
```

- `boton`: il pulsante di un comando (`button="..."` di un `<cmd>`). Nel gioco misura 153 x 51: tre riquadri da 51 x 51 (normale, con il mouse sopra e disattivato). Tu dai un PNG da 51 x 51 e lo strumento crea gli altri due riquadri (uno più chiaro e uno grigio). Se hai già tutti e tre, un PNG da 153 x 51 si usa così com'è. Un'altra dimensione viene scalata senza deformarla.
- `icon`: il ritratto di un'unità (`icon="..."`) o quello della sua coda di addestramento (`queueicon="..."`). Misura 65 x 76. Va bene un PNG di qualsiasi dimensione: viene scalato senza deformarlo, e la parte trasparente diventa il verde puro che il gioco prende come trasparente.

Con `--sfondo #RRGGBB` scegli il colore con cui si riempie la parte trasparente di un pulsante (di default, un marrone scuro come quello dei pulsanti del gioco).

Il PNG deve essere a 8 bit per canale e non interlacciato, che è la norma quando salvi da qualsiasi programma. Altrimenti lo strumento te lo dice.

## Dove va il file

Con lo stesso percorso che usa il gioco, dentro la cartella della mod. La regola per i pulsanti è `gameres/cmdbar/<lo que ponga button=>`:

| Cosa | Nell'XML | File nella tua mod |
|---|---|---|
| Pulsante di un comando | `button="Actions/Respiro.bmp"` | `gameres/cmdbar/Actions/Respiro.bmp` |
| Pulsante di un potenziamento | `button="research/Mejora.bmp"` | `gameres/cmdbar/research/Mejora.bmp` |
| Ritratto di un'unità | `<properties icon="gameres/icons/MiUnidad.bmp"/>` | `gameres/icons/MiUnidad.bmp` |
| Ritratto nella coda | `queueicon="gameres/icons/MiUnidad.bmp"` | `gameres/icons/MiUnidad.bmp` |

I comandi del gioco usano anche le cartelle `upgrades`, `skills`, `states` e `specials` dentro `cmdbar`. Funzionano allo stesso modo.

Le maiuscole nei percorsi non contano. Se il gioco non trova un'immagine, il pulsante compare come un riquadro blu uniforme.

## Se non compare

1. Metti `LogFiles=1` in `[Debug]` di `imperivm-mod.ini`. Il log annota ogni file che il gioco apre e segna quelli che non esistono (`VFS: opens … -> does not exist`), così vedi il nome esatto che chiede.

## Cosa non si può fare

- Le immagini dell'aiuto (`help/...`), quelle della barra informativa di ogni civiltà e gli sfondi del menu non hanno un convertitore.
- I modelli e le animazioni di unità ed edifici (`.rle` e `.ent.xml`) sono un'altra cosa: vedi [Grafica propria](grafica.md).
