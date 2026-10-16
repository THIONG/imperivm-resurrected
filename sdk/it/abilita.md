# Abilità delle truppe

Qualsiasi unità, normale o eroe, può avere abilità nuove. Un'abilità è un comando (il pulsante) più un metodo della
classe (lo script che lo esegue), come quelle di druidi e sacerdoti. Sono tutti file della mod.

Esempio pronto da copiare: [`esempi/esempio-abilita`](esempi/esempio-abilita), «Respiro» (tasto `Y`) per i soldati corpo a corpo.

## Via rapida

`crea-abilita.js` scrive i tre pezzi. Ti serve la cartella `DATA` del gioco ([Dati del gioco](README.md#dati-del-gioco)):

```
node strumenti/crea-abilita.js <DATA> mods/mi-mod respiro --classe Melee --tasto y --costo-energia 5 --nome Respiro --nome-en "Second wind"
```

| Opzione | A cosa serve |
|---|---|
| `--classe` | La classe che riceve il pulsante. Va bene una base (`Melee`, `Military`, `Unit`) e le figlie la ereditano, oppure una specifica (`RHastatus`, `Hero`). Deve esistere nel gioco |
| `--tasto` | Una lettera |
| `--costo-energia` | Energia da 1 a 9. Se la metti, lo script include già l'addebito |
| `--nome`, `--nome-en` | Il nome in spagnolo e in inglese. Senza `--nome-en`, il pulsante mostra l'id, o il nome se non ha accenti |
| `--descrizione`, `--descrizione-en` | Il testo d'aiuto, uguale |
| `--pulsante` | Un pulsante del gioco (`Actions/Mass Healing.bmp`, quello di default) o il tuo ([Immagini proprie](immagini.md)) |

Genera:

```
data\commands\Respiro.xml              il comando
patches\data\classes\Melee.sc.xml      il metodo nella classe (se c'è già una patch, si somma)
data\mod\respiro.vs                    lo script, con la firma e l'addebito dell'energia; il corpo lo scrivi tu
loc\spanish.loc.xml                    i testi in spagnolo (si sommano a quelli che ci sono)
mod.json                               solo se la cartella non ne ha uno
```

Ti avvisa se il tasto è già usato da un altro comando di quella classe, di una classe padre o di una figlia, e ti dice quali tasti restano liberi. Genera comunque. Non sovrascrive file: se l'id esiste già nella tua mod o nel gioco, si ferma senza scrivere niente.

Poi scrivi il corpo di `data\mod\respiro.vs` e passa il [validatore](validare.md).

## Cosa genera (come farlo a mano)

### I tre pezzi

**1. Il comando.** Un `<cmd>` in un nuovo file di `data\commands` (il gioco carica tutti i `.xml` di quella cartella):

```xml
<cmd name="respiro" priority="10" offset="1"
	button="Actions/Mass Healing.bmp" key="y"
	rollover="Second wind" description="..."
	groupverifier="" groupdispatch="" voidgroupdispatch="1"
	coststamina="5">
	<src obj="Melee"/>
</cmd>
```

- `name` è il nome del comando e anche il `sig` del metodo.
- `button` è l'immagine del pulsante: una del gioco, o una tua ([Immagini proprie](immagini.md)).
- `<src obj="…"/>` sono le classi che hanno il pulsante. Va bene una classe base, e le figlie lo ereditano: `Melee` (corpo a corpo), `Military`,
  `Unit` (tutte) o una specifica (`RHastatus`). Attenzione a `Military`: include gli eroi, e il tasto `Y` è già usato dall'abilità `skill_battle_cry`
  dell'eroe. Con `Melee` non c'è conflitto.
- `coststamina` è l'energia che costa. La barra `stamina` di un soldato del gioco vale 10, quindi il 5 dell'esempio ne consuma metà. Il gioco
  non la addebita da solo: la addebita il tuo script. Usa un valore da 1 a 9. Il tooltip mostra il costo come energia se è minore di 10 e come lealtà se è 10 o più.
- `key` è il tasto. Compare in Controlli, scheda Contenuti, e lì si cambia ([Tasti delle mod](tasti.md)).

**2. Il metodo.** Nella classe, con una [patch](patch.md) per non sostituire l'intero file. Va in `patches\data\classes\Melee.sc.xml`:

```xml
<patch>
  <add select="/class"><method sig="respiro" vs="data/mod/respiro.vs"/></add>
</patch>
```

Le classi figlie ereditano i metodi.

**3. Lo script.** `data\mod\respiro.vs`. Un comando immediato riceve `// void, Obj me`, l'unità che lo esegue:

```
// void, Obj me

Unit this;
int cost;

this = me.AsUnit();
if (!.IsAlive)
	return;

cost = GetCmdStaminaCost("respiro");
if (.stamina < cost)
	return;
.SetStamina(.stamina - cost);
.Heal(60);
```

I comandi con bersaglio (un clic su un'altra unità) ricevono `// void, Obj This, Obj target`. Guarda come fanno quelli del gioco in
`data\subai\druid_*.vs` e `data\commands\DRUID.XML`. Tutte le funzioni sono in [funciones-del-juego.txt](funzioni-del-gioco.txt).

### Nome e descrizione

Il comando si traduce con il suo nome come contesto. Un `loc\<idioma>.loc.xml` ([Lingue e testi](lingue.md)):

```xml
<translationtableentry text="Second wind" context="respiro" result="Respiro"/>
<translationtableentry text="The soldier recovers 60 health (costs stamina)" context="respiro" result="El soldado recupera 60 de vida (gasta energía)"/>
```

Il `text` è quello di `rollover` e `description` del comando. In [Unità nuove](unita.md) c'è l'altra forma (`text="…@contexto"`); `lingue.md` dice
che valgono entrambe.

## Abilità passive

Un'abilità senza pulsante (curarsi quando si uccide, danno extra, un'aura…) non ha bisogno di un comando. Usa un [evento](script.md) (`OnObjectKill`, `OnObjectDie`) o
un `<behavior>` nella classe.

## Multigiocatore

Segue le regole di [Script](script.md#multiplayer): non leggere niente di locale. Tutti i giocatori devono avere la stessa mod.

## Cosa non si può fare

- Creare specialità (`unit_specials`) o abilità di eroe (`HeroSkills`) native nuove con un effetto proprio: il loro effetto lo porta il gioco. Puoi usare e combinare le 25 che esistono. I comandi con script qui sopra coprono quasi tutto il resto, e le specialità proprie
  seguono un'altra strada ([Specialità](specialita.md)).
- Effetti visivi e suoni nuovi: riusa quelli del gioco.
