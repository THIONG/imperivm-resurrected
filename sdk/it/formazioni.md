# Formazioni nuove

Le formazioni degli eroi (Line, Front, Block, Cavalry) sono dati: un `formations.xml` con bonus e disposizione, e due ordini per formazione in `data\commands\Hero.xml`. Una mod aggiunge le sue con le [patch](patch.md), senza compilare nulla.

Esempio: [`esempi/esempio-formazione`](esempi/esempio-formazione) (il Cuneo).

## Via rapida

`crea-formazione.js` scrive i tre pezzi. Ti serve la cartella `DATA` del gioco ([Dati del gioco](README.md#dati-del-gioco)):

```
node strumenti/crea-formazione.js <DATA> mods/mi-mod Wedge --danno 3 --armatura 1 --come "Cavalry core" --tasto i
```

| Opzione | A cosa serve |
|---|---|
| `<Nome>` | Il `Name` della formazione: lettere e numeri, niente spazi. Deve essere nuovo |
| `--danno`, `--armatura`, `--difesa-pierce`, `--difesa-slash`, `--raggio`, `--livello` | I bonus (da -1000 a 1000). Senza nessuno, la formazione cambia solo la disposizione |
| `--come` | La formazione del gioco da cui copia l'elenco delle classi, la dimensione, la spaziatura e il disegno del pulsante: `Front` (default), `Line`, `Block`, `Cavalry core`, `Horse Wings`, `Ranged Wings` |
| `--classi "Unit=ali,Horse=fronte"` | La tua disposizione, al posto di quella di `--come`. I posti sono `fronte`, `ali` e `centro` |
| `--larghezza`, `--altezza`, `--fronte`, `--ala-x`, `--ala-y` | `Width`, `Height` e le tre spaziature (`Offset…`) |
| `--tasto` | La lettera che sceglie la formazione. Senza, ne cerca una libera |
| `--descrizione`, `--pulsante` | Il `Description` (in inglese) e un pulsante diverso (`States/…`) |

Genera:

```
patches\data\formations.xml                     la formazione
patches\data\commands\Hero.xml                  i suoi due pulsanti, form-<nome>-menu e form-<nome>
patches\data\subai\hero_formation_verify.vs     la patch dello script
mod.json                                        solo se la cartella non ne ha uno
```

Se la mod ha già un'altra formazione, la nuova si aggiunge agli stessi file senza sovrascrivere la precedente. Vale anche per la patch dello script: ogni formazione aggiunge il suo `@replace`.

Controlla che il tasto sia libero per l'eroe, che eredita gli ordini di `Unit` e `Military`. Se è in conflitto, avvisa e genera comunque. Nel gioco resta libera solo la `i`, quindi la seconda formazione di una mod dovrà condividere il tasto: scegli una lettera e verifica che il conflitto non ti dia fastidio.

I pulsanti usano il disegno della formazione di base. Per averne uno tuo, vedi [Immagini personalizzate](immagini.md) e passa `--pulsante`.

## Cosa fa dentro (come farlo a mano)

### I tre pezzi

**1. La formazione** (`patches\data\formations.xml`):

```xml
<patch>
  <add select="/Formations">
    <FormationClass Name="Wedge"
        Description="The warriors form a wedge with the cavalry at the point."
        Width="3" Height="1"
        BonusDamage="3" BonusArmor="1"
        OffsetFrontLineByY="40" OffsetWingsByX="30" OffsetWingsByY="0">
      <Class Name="Unit" Wings="1"/>
      <Class Name="Horse" FrontLine="1"/>
      <Class Name="BaseMage" CentralBlock="1"/>
      <Class Name="Peasant" CentralBlock="1"/>
    </FormationClass>
  </add>
</patch>
```

| Attributo | A cosa serve |
|---|---|
| `Name` | Nome della formazione; è quello che usano gli ordini |
| `Description` | Non si vede sul pulsante: il gioco mostra un testo fisso ("Army bonus during stand ground") con le icone dei bonus |
| `Width`, `Height` | Dimensione della griglia del gruppo in prima linea (`FrontLine`). Nel Cuneo, dove le `Unit` vanno sui fianchi, `Width=3` riguarda solo i cavalli |
| `BonusDamage`, `BonusArmor`, `BonusRange`, `BonusLevel` | Bonus dell'esercito, e solo mentre la truppa è ferma ("stand ground"). Limitati tra -1000 e 1000 |
| `BonusDefencePierce`, `BonusDefenceSlash` | Difesa contro perforante e contro taglio, separate. `BonusArmor` le imposta entrambe |
| `OffsetFrontLineByY`, `OffsetWingsByX`, `OffsetWingsByY` | Quanto si allontanano dall'eroe la prima linea e i fianchi |
| `<Class Name="…" …/>` | Dove vanno le unità di quella classe: `FrontLine="1"` davanti, `Wings="1"` ai lati, `CentralBlock="1"` al centro, intorno all'eroe |

`BonusAttack` compare in qualche formazione del gioco, ma non fa nulla: non contarci.

**2. I pulsanti** (`patches\data\commands\Hero.xml`). Due ordini: `form-<nome>-menu` (il pulsante che si vede quando quella è la formazione attuale) e `form-<nome>` (quello che la sceglie). Entrambi hanno `param="<Name>"` e `menu="1"`. Copia `form-front-menu` e `form-front` e cambia `name`, `param`, `key` e `button` (il disegno; vedi [Immagini](immagini.md)).

Il tasto non deve andare in conflitto con un altro ordine dell'eroe, che eredita anche quelli di `Unit`. In `data\commands` (con `estrai-dati.js`, vedi [Patch](patch.md)) cerca `key="x"` in `HERO.XML` e `UNIT.XML`. L'esempio usa `i`, che non è in nessuno dei due; `q`, per esempio, è di `reveal`.

**3. Una patch dello script che dà il via libera ai pulsanti** (`patches\data\subai\hero_formation_verify.vs`). Lo script del gioco lascia scegliere la formazione solo quando è aperto uno dei suoi quattro menu, elencati per nome. Aggiungi il tuo:

```
@replace if (cmd_name != "form-front-menu")
if (cmd_name != "form-front-menu")
	if (cmd_name != "form-wedge-menu")
@end
```

Se un'altra mod modifica la stessa riga per aggiungere un'altra formazione, le patch si sommano: ogni `@replace` riscrive la riga del gioco e lascia la tua dopo.

## Online

I bonus cambiano il gameplay: tutti i giocatori devono avere la stessa mod.
