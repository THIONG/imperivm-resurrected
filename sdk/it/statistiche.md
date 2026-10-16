# Statistiche di unità ed edifici

Quello che non ha un'etichetta si trova nei dati del gioco. *(dedotto)* è un'ipotesi su come il gioco usa quel valore, *(parziale)* la meno affidabile.

Le statistiche sono attributi di `<properties>` nella classe (`data\classes\<Classe>.sc.xml`). Per cambiarle senza copiare l'intera classe usa una [patch](patch.md); per creare un'unità, vedi [Unità nuove](unita.md).

```xml
<patch>
  <!-- I legionari corrono di più: velocità 60 -> 75 -->
  <set select="/class/properties[@speed]" speed="75"/>
</patch>
```

Va in `patches\data\classes\RHastatus.sc.xml`. Un attributo che la classe non imposta lo eredita dal padre.

## Quale attributo cambia cosa

| Voglio cambiare… | Attributo | Note |
|---|---|---|
| Velocità di movimento | `speed` | L'animazione della camminata si adatta da sola (dedotto) |
| Ritmo di attacco | nessun attributo | Durata dell'animazione di attacco, oppure `SetSpeedFactor` da script |
| Gittata di un arciere | `range` | |
| Gittata minima (catapulte) | `min_range` | |
| Dimensione reale (hitbox) | `radius` | Collisione, percorsi e gittata (dedotto) |
| Area di clic | `selection_radius` | Estetico (dedotto) |
| Danno | `damage` | |
| Armatura | `armor_slash` | `armor_pierce` non fa nulla (parziale) |
| Vita | `maxhealth` | |
| Visuale | `sight` | |
| Chi attacca per primo | `target_factor` | |
| Proiettili | `projectile_class` | Con dei limiti, vedi *Gittata* |

### Velocità (`speed`)

- Sono unità di mappa al secondo. Nel gioco: fanteria 60-80, arcieri 60-160, cavalleria 130-190, eroi 120, arieti 40, navi 240.
- Il ciclo dell'animazione della camminata dura `step × 1000 / speed` millisecondi, con `step` preso dal `.ent.xml` (l'hastatus: 42 × 1000 / 60 = 700 ms).
  Cambiando solo `speed` il disegno non si sfasa: l'animazione accelera o rallenta insieme al movimento. *(dedotto)*
- `speed="0"` su un'unità che cammina blocca il gioco per una divisione per zero. Lo stesso vale per una classe che non definisce `speed` e non lo eredita. *(dedotto)*

### Ritmo di attacco

Per le unità non esiste un attributo di «velocità di attacco». *(dedotto)*

- Un colpo dura quanto l'animazione di attacco, cioè la somma dei `<frame duration>` del `.ent.xml`, e il danno viene inflitto al momento `action_time` di
  quell'animazione. Nelle 118 animazioni del gioco i colpi vanno da 594 a 1584 ms, con una mediana di 990 ms (l'hastatus: 994 ms, `action_time` 426).
- `attack_delay` non serve per le unità: lo usano solo `Catapult` (500) e `FakeTower` (400), ed è l'unico caso nei dati. *(dedotto)*
- Per attaccare più in fretta ci sono due strade. La prima: accorcia i fotogrammi dell'animazione di attacco e riscala `action_time` nella stessa proporzione.
  La seconda: `Unit::SetSpeedFactor(150)` da script, che accelera passo e attacco insieme del 50 %. `SetSpeedFactor(0)` blocca il gioco. *(dedotto)*
- Per modificare il `.ent.xml` da una mod, una patch XML dovrebbe funzionare. Un `.ent.xml` isolato non sostituisce quello del gioco ([Grafica personalizzata](grafica.md)).
- Il ritmo e il momento del colpo fanno parte della simulazione: devono essere uguali per tutti i giocatori.

### Gittata (`range`, `min_range`)

- Valori del gioco: fanteria 15-40, arcieri 300-500 (le navi arrivano a 700), sentinelle 600, cavalleria 15-40, catapulte 700-800, torri
  600-800. `RCatapult` ha `min_range=201`.
- Si misura da bordo a bordo: la distanza tra i centri meno il `radius` delle due unità. Per questo conta anche `radius`. *(dedotto)*
- Un'unità da mischia diventa «a distanza» con `range` più un `projectile_class` (Arrow, Javelin, Axe, Slingstone) e un'animazione di
  attacco con `action_time`.
- Con `range` minore di `min_range` non attacca mai. Con `range="0"` su un'unità da mischia, nemmeno. *(dedotto)*
- Il proiettile impiega quanto dura la sua animazione, qualunque sia la distanza (freccia 495 ms, ascia e giavellotto 561, pietra di fionda 300). Non esiste un attributo
  per la velocità del proiettile. Un attaccante con proiettile fa metà del danno agli edifici. *(dedotto)*
- Catapulte: la parabola dipende da `shot_tan` e `shot_height`. Non cambiarli su un solo computer.

### Hitbox e dimensione

| Attributo | A cosa serve | Valori del gioco |
|---|---|---|
| `radius` | Dimensione fisica: collisione, percorsi, distanziamento e gittata (dedotto). Simulazione | Fanteria 10, cavalleria 20, eroi 20-30, arieti 80, catapulte 50 |
| `selection_radius` | Anello di selezione e area di clic. Estetico finché l'unità ha `formation_radius` (dedotto) | |
| `formation_radius` | Distanza tra i posti di una formazione. Simulazione | Fanteria 26, cavalleria 35, eroi 45 |

In un edificio nuovo imposta sempre `radius`. Quello ereditato (`Building`: 600) è enorme e gli attaccanti «arrivano» all'edificio da molto lontano. Quelli veri hanno 160-250 le caserme, 47 le torri, 80-110 le mura e 333-345 gli avamposti.

### Danno e armatura

Costanti di `data\const.ini`, sezione `[GamePlay]`: `MinDamage=3`, `MinPercentOfAttackersDamage=20`, `DamageAmplify=200`
e `LevelDifferenceDamageTable`. Come vengono usate è *(dedotto)*:

- Un colpo fa `damage − armor_slash` del difensore, con un minimo del 20 % del danno dell'attaccante e mai meno di 3. Poi si applica la
  differenza di livello tra i due: va da −90 % a +90 %, e se il difensore ha più livello, sottrae.
- `armor_pierce` non entra nel calcolo: puoi scriverlo, ma non fa nulla *(parziale: forse qualche script lo usa)*. Se vuoi dare armatura, usa `armor_slash`.
- `damage_type` (`slash`, `pierce`, `siege`, `none`) non cambia il danno: decide chi può attaccare l'unità. `slash` e `pierce` attaccano le unità
  (e gli edifici se c'è un proiettile), `siege` anche gli edifici, `none` non attacca.
- Gli edifici non hanno armatura: il danno passa per intero.
- `DamageAmplify` è il danno alla guarnigione di un edificio. Queste costanti si ritoccano con le [patch INI](patch.md).

### Altri attributi

| Attributo | Cosa fa |
|---|---|
| `maxhealth` | Vita. Fanteria 160-3000, eroi 1000-1300, avamposti 10000. `0` blocca un edificio o fa nascere l'unità già morta *(dedotto)* |
| `maxstamina` | L'energia che consumano le specialità e le [abilità](abilita.md). Tutti i soldati del gioco hanno 10. Con meno del costo, la specialità non si attiva *(dedotto)* |
| `sight` | Visuale: svela la nebbia (dedotto). Fanteria 500, sentinelle 600, civili 150 |
| `target_factor` | Chi si attacca per primo: più basso, prima. Default 100, eroi 200, civili 1000. Negativo = non lo sceglie mai da solo (la fauna ha -1) *(dedotto)* |
| `unit_specials` | [Specialità](specialita.md), quelle del gioco e le tue |
| `feeds`, `max_food` | `Unit` eredita `feeds=1` e `max_food=20`: tutte le unità normali mangiano. Con il cibo a 0 perdono vita *(dedotto; quando, non chiarito)*. `RamUnit`, `Sentry`, `GGhost` e gli animali hanno `feeds=0`; mettilo anche a macchine e creature magiche |
| `does_not_regenerate` | `1`: niente cibo né rigenerazione. Utile per evocati e macchine *(dedotto)* |
| `stamina_dec_time` | Nelle unità evocate: millisecondi tra una perdita di 1 di energia e l'altra. Vivono ≈ `(maxstamina+1) × questo valore`; con `0` o meno muoiono subito *(dedotto)* |
| `water_unit` | Navi e pesci: percorsi e bersagli in acqua |
| `desync_hash` | `1`: il movimento di questo oggetto entra nel controllo di sincronia. Metti `0` negli effetti visivi nuovi *(dedotto)* |

## Quello che non fa nulla

`armor_pierce` *(parziale)*, `speed_modify` e `taunt` (come proprietà è il nome di un suono) vengono letti ma non usati. *(dedotto)* `attack_delay` non
serve per le unità. `target_priority` conta solo per torri e catapulte; per le unità c'è `target_factor`.

## Cambiare durante la partita (da script)

| Cosa | Come |
|---|---|
| Vita, vita massima, energia, energia massima | `SetHealth`, `SetMaxHealth`, `SetStamina`, `SetMaxStamina`. Il gioco ricalcola i massimi quando equipaggi un oggetto o sali di livello e sovrascrive il valore |
| Visuale | `SetSight(n)`, da 100 a 2048 |
| Velocità e ritmo di attacco | `Unit::SetSpeedFactor(n)`, in percentuale (100 = normale). `0` blocca il gioco |
| Livello, esperienza, specialità | `SetLevel`, `SetExperience`, `Mod_SetSpecial` |
| Danno, armatura, gittata o raggio di una singola unità | Non esiste una funzione. Crea un'altra classe con quei valori e trasforma l'unità con `Unit::Mutate("OtraClase")`, che mantiene vita, esperienza e ordini |

Le funzioni sono in [funciones-del-juego.txt](funzioni-del-gioco.txt). Per cambiare il danno di
un singolo colpo, usa l'evento `OnAttackResolved` con `Mod_SetAttackDamage` ([Specialità](specialita.md)).

## Quello che non si può fare con l'XML

- La formula di combattimento (armatura piatta, minimi, tabella dei livelli). Si possono regolare solo le costanti di `const.ini`.
- La velocità del proiettile e la sua gittata di volo: il colpo va a segno se la distanza è minore di `range + 150`. *(dedotto)*
- Comportamenti di codice nuovi: i `cpp_class` sono solo quelli forniti dal gioco.
- La logica delle formazioni, a parte ciò che permette [Formazioni](formazioni.md).

## Multigiocatore

Quasi tutto quello che sta sopra è simulazione. Tutti i giocatori devono avere la stessa mod, e il gioco lo controlla all'ingresso in una lobby; una
differenza tra due giocatori desincronizza la partita, che viene interrotta. Sono estetici `selection_radius`, `healthbar_*`, `icon`,
`display_name`, `help` e i disegni.

I disegni (`.rle`) non contano per il controllo. L'elenco completo di ciò che conta è nel [README](README.md).

## Quello che non si sa

- L'equivalenza esatta tra unità di mappa e pixel dello schermo.
- Se `sight` limita anche a che distanza un'unità cerca un bersaglio. Si vede che svela la nebbia e che gli script lo usano per cercare; il calcolo esatto, no.
- Quale `formation_priority` va davanti in una formazione.
- Cosa succede con un `action_time` maggiore della durata dell'animazione. Evitalo.
