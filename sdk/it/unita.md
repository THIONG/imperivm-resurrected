# Unità nuove

Un'unità nuova sono tre cose: una classe (le statistiche), un comando per reclutarla in un edificio, e le sue immagini e i suoi testi. Sono tutti
file della mod; non devi programmare niente.

Di default l'unità riusa il modello e le animazioni di una del gioco. Se vuoi disegni tuoi, vedi [Grafica propria](grafica.md):
converti i tuoi PNG in `.rle` e genera il `.ent.xml`.

Esempio pronto da copiare: [`esempi/esempio-unita`](esempi/esempio-unita), il Veterano romano: più forte dell'hastatus, si recluta
nella caserma di Republican Rome. Con sprite propri: `esempi/esempio-unita-grafica` (vedi [Grafica propria](grafica.md)).

## Via rapida

Ti serve la cartella `DATA` del gioco ([`estrai-dati.js`](strumenti/estrai-dati.js)). Il Veterano dell'esempio si ottiene con:

```
node strumenti/crea-unita.js <DATA> <cartella del mod> RVeteran --base RHastatus --vita 260 --danno 20 --armatura 14 --edificio RBarracks --costo-oro 180 --tempo 9000 --tasto v --nome Veterano --nome-en Veteran
```

Genera, dentro della cartella della mod:

- `data\classes\RVeteran.sc.xml`: copia integrale della classe base, con il tuo id e le tue statistiche.
- `data\commands\RVeteran.xml`: il comando per reclutarla, copia di quello della base.
- `loc\spanish.loc.xml` (e `english.loc.xml` con `--nome-en`, `italian.loc.xml` con `--nome-it`): nome, plurale e testo del pulsante. Se il file esiste già, aggiunge le voci.
- `mod.json`, solo se non ce n'era uno.

| Opzione | A cosa serve |
|---|---|
| `--base` | L'unità del gioco da copiare (obbligatoria). Va bene il suo id o il suo `altid` |
| `--vita`, `--danno`, `--armatura`, `--velocita` | Statistiche. `--armatura` imposta `armor_slash` e `armor_pierce`. Senza, quelle della base |
| `--edificio` | Dove si recluta. Senza, dove si recluta la base (se sono più d'uno, quello della sua stessa civiltà) |
| `--costo-oro`, `--costo-cibo`, `--tempo` | Prezzo e tempo di reclutamento (in millisecondi) |
| `--tasto` | Il tasto del pulsante. Senza, una lettera del nome libera in quell'edificio |
| `--nome`, `--plurale` | Nome in spagnolo. Il plurale viene indovinato; correggilo con `--plurale` se non va bene |
| `--nome-en`, `--plurale-en`, `--nome-it` | Nome in inglese e in italiano. `display_name` è la chiave del testo, quindi con `--nome-en` la chiave è il nome in inglese |

Se qualcosa non va (id ripetuto, base che non è un'unità, edificio inesistente) dice perché e non scrive niente. Non sovrascrive nemmeno i file esistenti.

Limiti:

- Copia solo unità che il gioco recluta con un comando normale (`method="train"`). Quelle che si assoldano in gruppo (gladiatori, vichinghi…) no.
- Ritratto e pulsante sono quelli della base. Per averli tuoi, vedi [Immagini](#3-immagini) e cambia i percorsi nella classe e nel comando.
- L'IA non recluterà l'unità.
- Passa il risultato dal [validatore](validare.md) con `--dati`. Quello che genera è uguale all'esempio (tranne le immagini).

## Cosa genera (come farlo a mano)

Se preferisci scriverlo tu, o ti serve qualcosa che lo strumento non copre, ecco cosa genera lo strumento.

## 1. La classe

`data\classes\RVeteran.sc.xml`. Parti da una copia integrale di una classe simile del gioco, per esempio `RHastatus.sc.xml`. Per vedere i
file del gioco, estrai `data.pak` con [`estrai-dati.js`](strumenti/estrai-dati.js): `node estrai-dati.js "<cartella del gioco>"
<cartella di output>` li mette in `<cartella di output>\DATA`. Cambia l'`id` e le statistiche:

```xml
<class id="RVeteran" cpp_class="CVXUnit" parent="Melee" entity="Units/RHastatus/RHastatus.ent.xml">
	<properties race="RepublicanRome"/>
	<properties maxhealth="260"/>
	<properties damage="20" armor_slash="14" armor_pierce="14"/>
	<properties speed="60"/>
	<properties unit_specials="Deflection"/>
	<properties icon="gameres/icons/RVeteran.bmp"/>
	<properties display_name="Veteran" display_name_plural="Veterans"/>
	...
</class>
```

- Copiala per intero; non farne una figlia (`parent="RHastatus"`). Il gioco non eredita dal padre l'entità (`entity`, il modello) né i suoni, quindi
  una figlia senza il suo `entity` viene creata senza modello e sparisce.
- `entity` punta al modello e alle animazioni. Qui sono quelli dell'hastatus.
- `race` è la civiltà: `RepublicanRome`, `Gaul`… o una della tua mod ([Civiltà](civilta.md)).
- Le statistiche più usate sono `maxhealth`, `damage`, `armor_slash`, `armor_pierce`, `speed`, `sight` e `range`. Cosa fa ciascuna, e
  quali non servono a niente, è in [Statistiche](statistiche.md). Un'unità copiata da una normale eredita `feeds=1` (mangia); le macchine
  d'assedio e le sentinelle del gioco lo mettono a 0.
- `unit_specials` sono le [specialità](specialita.md), quelle del gioco e le tue.
- Non copiare l'`altid` dell'originale. È un alias che gli script usano per trovare «il soldato» di una civiltà; due classi con lo
  stesso alias si confondono.
- `edittree_pos` (per esempio `Units/Roman/Veteran`) è dove compare nell'editor di mappe e nel menu dei trucchi: una classe con `entity`
  il cui `edittree_pos` inizia con `Units/` compare lì, nel gruppo della sua civiltà. Lo strumento lo copia dalla base e ne cambia
  l'ultimo nome.
- Un file nuovo in `data\classes\` si carica da solo. Non va registrato.

## 2. Il comando di reclutamento

`data\commands\Veterano.xml`. È una copia del comando del gioco (`trainRHastatus`, in `data\commands\RBARRACKS.XML`) con un'altra classe e un altro
costo. Il gioco carica tutti i `.xml` di `data\commands`.

```xml
<cmd name="trainRVeteran" priority="3"
	button="actions/train RVeteran.bmp" queueicon="gameres/icons/RVeteran.bmp"
	key="v" costgold="180" costfood="0" costpop="1" execdelay="9000"
	rollover="Equip Veteran" method="train" param="RVeteran" sclass="RVeteran" ...>
	<src obj="RBarracks"/>
</cmd>
```

- `<src obj="RBarracks"/>` è l'edificio dove compare il pulsante. Per un altro edificio, un'altra classe (`RBlacksmith`, `RArena`…).
- `param` e `sclass` sono la classe che si recluta. `costgold`, `costpop` ed `execdelay` (millisecondi) sono prezzo e tempo.
- `key` è il tasto. Compare in Controlli, scheda Contenuti, e lì si cambia ([Tasti delle mod](tasti.md)).
- Un edificio nuovo si fa allo stesso modo: copia integrale di una classe di edificio, più i suoi comandi con `<src obj="MiEdificio"/>`.

## 3. Immagini

Il pulsante misura 153 x 51 e il ritratto 65 x 76. Si fanno da un PNG con `strumenti/converti-immagine.js` ([Immagini proprie](immagini.md)). Senza
immagini proprie, usa quelle dell'unità del gioco (`button="actions/train RHastatus.bmp"`, `icon="gameres/icons/RHastatus.bmp"`).

## 4. Testi

Il nome della classe e il testo del pulsante si traducono con un `loc\<idioma>.loc.xml` ([Lingue e testi](lingue.md)):

```xml
<translationtableentry text="Veteran@RVeteran class name" result="Veterano"/>
<translationtableentry text="Veterans@RVeteran class name plural" result="Veteranos"/>
<translationtableentry text="Equip Veteran@trainRVeteran" result="Equipar veterano"/>
```

## Multigiocatore e limiti

- Classi e comandi sono gameplay: tutti i giocatori devono avere la stessa mod.
- L'IA non recluterà la tua unità a meno che tu non porti script di IA propri. Vedi [Civiltà](civilta.md#lia).
- Un'unità di una civiltà nuova ha bisogno anche dei suoi edifici e comandi. Lo strumento di clonazione di [Civiltà](civilta.md)
  li genera.
