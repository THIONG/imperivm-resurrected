# Oggetti nuovi

Gli oggetti che portano gli eroi (amuleti, erbe, gemme…) sono dati. Il gioco ne ha 42 in `data\items.xml`, e una mod può aggiungere i suoi
con una [patch](patch.md), senza compilare nulla.

Esempio pronto da copiare: [`esempi/esempio-oggetto`](esempi/esempio-oggetto).

## Via rapida

L'oggetto dell'esempio, con ogni eroe che lo riceve, si ottiene con:

```
node strumenti/crea-oggetto.js <cartella della mod> "Amuleto de ejemplo" --danno 10 --armatura 5 --dai-eroi
```

Genera, dentro la cartella della mod:

- `patches\data\items.xml`: l'oggetto con tutti i suoi attributi. Se la patch esiste già, lo aggiunge senza toccare quelli presenti.
- Con `--dai-eroi`: `patches\data\classes\Hero.sc.xml` (un comportamento in più per la classe `Hero`) e `data\mod\dar_<id>.vs`, lo script che dà l'oggetto a ogni eroe che non ce l'ha ancora. Se `Hero.sc.xml` esiste già, ci aggiunge anche questo.
- `mod.json`, solo se non ce n'era uno.

| Opzione | A cosa serve |
|---|---|
| `--vita`, `--danno`, `--armatura`, `--livello`, `--esperienza` | Quello che aggiunge a chi lo porta. `--armatura` imposta `armor_slash` e `armor_pierce` |
| `--icona` | Un'icona del gioco, per esempio `"gameres/icons/items/Belt of might.bmp"`. Senza, quella dei denti d'orso |
| `--descrizione` | Il testo della finestra dell'oggetto. Senza, uno in inglese come `Adds 10 to damage and 5 to armor` |
| `--dai-eroi` | Ogni eroe riceve l'oggetto quando compare |
| `--dati <DATA>` | Controlla che nessun oggetto del gioco abbia già quell'id |

Ripetere il comando con un altro id aggiunge un altro oggetto. Se l'id esiste già nella tua patch o nel gioco, lo segnala e non scrive nulla.

Non genera `use_script`, `equip_script` né `kill_script`, né `usecount` o `cursed`: aggiungili a mano all'`<item>` ([Un oggetto con effetto proprio](#un-oggetto-con-effetto-proprio)). Quello che genera è uguale all'esempio.

## Cosa fa dentro (come farlo a mano)

## Un oggetto

Una patch di `data\items.xml`, in `patches\data\items.xml` dentro la tua mod:

```xml
<patch>
  <add select="/items">
    <item id="Amuleto de ejemplo"
          image="gameres/icons/items/Bear Teeth.bmp"
          sound="data/sound entities/ItemUse.xml"
          level="0"
          name="Amuleto de ejemplo"
          description="Adds 10 to damage and 5 to armor"
          important="no">
      <bonus health="0" damage="10" armor_slash="5" armor_pierce="5" level="0" experience="0"/>
    </item>
  </add>
</patch>
```

| Attributo | A cosa serve |
|---|---|
| `id` | Il nome con cui lo chiamano gli script (`AddItem("…")`). Unico |
| `image` | L'icona: una del gioco, o la tua dentro `gameres/icons/items/` nella tua mod (un BMP) |
| `name`, `description` | Quello che si legge nella finestra dell'oggetto |
| `level` | Livello minimo dell'eroe per portarlo (nessun oggetto del gioco usa un valore diverso da 0) |
| `important` | `yes` o `no`. Se è un oggetto speciale (viene mostrato a parte) |
| `usecount` | Quante volte si usa: `1` per quelli monouso, di più per quelli a cariche (`1000` di cibo nel grano) |
| `cursed` | `1` se non si può togliere, `0` se sì. È un numero, non `yes`: il gioco lo legge come numero e qualsiasi testo vale 0, senza avvisi. |

Il `<bonus>` si somma a chi lo porta: `health`, `damage`, `armor_slash`, `armor_pierce`, `level` ed `experience`. Con `health_percent`,
`damage_percent`, `armor_slash_percent` e `armor_pierce_percent` aggiunge una percentuale invece di un numero fisso.

`items.xml` accetta qualche attributo in più (`customdata`, `location_script`, `object_script`, `attacheddie_script`), non documentato; guarda come li usano gli oggetti del gioco.

## Un oggetto con effetto proprio

Uno script che parte quando l'eroe lo usa:

| Attributo | Quando parte | Firma |
|---|---|---|
| `use_script="file://data/mod/mi.vs"` | Quando il giocatore usa l'oggetto | `// void, Obj owner` |
| `equip_script` | Quando lo equipaggia | Guarda quelli del gioco |
| `kill_script` | Quando uccide qualcuno con quell'oggetto | Guarda quelli del gioco |

Scrivi il percorso con `file://`, come gli oggetti del gioco (`file://data/ItemScripts/….vs`).

Gli script del gioco sono in `data\ItemScripts`; per vederli, estrai i dati con [`estrai-dati.js`](strumenti/estrai-dati.js). Quello
delle erbe curative:

```
//void, Obj owner

CreateFeedback("Heal", owner.AsUnit());
owner.Heal(owner.maxhealth);
ItemUsed(1);
```

`ItemUsed(n)` consuma `n` usi dell'oggetto. Con `usecount="1"` sparisce quando lo usi.

## Come ottenerlo in partita

- Darlo con uno script: `Obj::AddItem("id")` restituisce `true` se c'è posto nell'inventario. L'esempio lo fa in un comportamento della classe `Hero`
  (`patches\data\classes\Hero.sc.xml`), che gira con ogni eroe che compare. Controlla prima che non ce l'abbia già (`FindItem("id").IsValid`): quando
  carichi una partita salvata lo script riparte da capo.
- Farlo cadere quando si uccide: un evento [`OnObjectKill`](script.md) che chiami `AddItem` su chi uccide, oppure `DropItem` a terra.
- Nelle rovine e nei tesori della mappa: guarda `data\classes\BaseRuins.sc.xml`. Distribuisce oggetti con righe `itemtype` e `levelperitem`. Nei dati
  del gioco non lo usa nessun'altra classe, nemmeno i negozi delle civiltà.

Gli oggetti cambiano il gameplay: in una partita online tutti i giocatori hanno la stessa mod.
