# Specialità di unità proprie

Il gioco ha 36 specialità fisse (Parry, Penetration, Deflection…). Non si possono ampliare, ma una mod può dichiararne di nuove
il cui effetto programmi tu con gli script.

Esempio pronto da copiare: [`esempi/esempio-specialita`](esempi/esempio-specialita), «Golpe crítico»: il 25 % dei colpi degli
hastatus fa il triplo del danno.

## Come funziona

Ogni unità ha una maschera a 64 bit, un bit per specialità. I bit da 0 a 35 sono le 36 del gioco. Quelli da 36 a 63 sono tuoi e si salvano con la partita. Una specialità di mod occupa un bit.

Sono tre passi.

**1. Dichiararla.** Un file `specials\<id>.json` nella tua mod:

```json
{
  "id": "Critical",
  "bit": 36,
  "name": {"spanish": "Golpe crítico", "english": "Critical hit", "italian": "Colpo critico"},
  "description": {"spanish": "Uno de cada cuatro golpes hace el triple de daño.", "english": "One hit in four does triple damage.", "italian": "Un colpo su quattro fa il triplo del danno."}
}
```

| Campo | Cos'è |
|---|---|
| `id` | Il nome: lettere, numeri e `_`, inizia con una lettera, fino a 24 caratteri. Lo scrivi in `unit_specials` ed è la costante di script (`Critical`). Non può essere il nome di una delle 36 del gioco (`Triple strike`…). Evita anche gli id che, in minuscolo e con `_` al posto dello spazio, sono costanti del gioco (`triple_strike`, `keen_sight`, `life_steal`…): il validatore non lo controlla e la costante andrebbe in conflitto con quella del gioco. |
| `bit` | Un numero da 36 a 63. Non cambiarlo dopo la pubblicazione: le partite salvate lo ricordano. Due mod non possono usare lo stesso. |
| `name` | Come si chiama in ogni lingua, oppure un solo testo. |
| `description` | Viene letta, ma per ora non compare da nessuna parte. Descrivila nel testo d'aiuto dell'unità. |
| `icon` | (Opzionale) Viene letto, ma non è usato. |

**2. Assegnarla alle classi.** Con il solito attributo `unit_specials`. Il gioco ignora i nomi che non conosce e la mod imposta il bit. Meglio con
una [patch](patch.md), per non sostituire il file della classe:

```xml
<patch>
  <set select="/class/properties[@unit_specials='Deflection']" unit_specials="Deflection, Critical"/>
</patch>
```

L'ereditarietà è quella del gioco. Se una classe imposta `unit_specials`, la sua lista sostituisce quella del padre; se non lo imposta, eredita
per intero quella del padre. Aggiungendone una a una classe che già ne ha, ripeti quelle che aveva (`Deflection, Critical`). Nel gioco i nomi distinguono le maiuscole; quelli della tua mod no.

**3. Programmare l'effetto.** Il gioco salva solo il bit. Quello che fa lo scrivi in uno [script](script.md):

| Funzione | Cosa fa |
|---|---|
| `Mod_HasSpecial(Unit unidad, int bit)` : `bool` | L'unità ha quella specialità? Vale per qualsiasi bit da 0 a 63, anche quelli del gioco. Riceve una `Unit`; con un `Obj`, usa `objeto.AsUnit()` (se non è un'unità, dà `false`). |
| `Mod_SetSpecial(Unit unidad, int bit, bool puesta)` | Assegna o toglie la specialità a quell'unità, per esempio quando sale di livello. Si salva con la partita. |

Il bit è la costante con l'`id`: `Mod_HasSpecial(attacker.AsUnit(), Critical)`.

Il posto naturale per l'effetto è l'evento `OnAttackResolved`, che può cambiare il danno di ogni colpo (`events\OnAttackResolved.vs`). Va bene
qualsiasi altro script: un `<behavior>` che controlla le unità a intervalli, `OnObjectKill`, `OnObjectDie`…

```
// void, Obj attacker, Obj victim, int damage
if (!attacker.IsValid) return;
if (!Mod_HasSpecial(attacker.AsUnit(), Critical)) return;
if (rand(100) >= 25) return;
Mod_SetAttackDamage(damage * 3);
```

## Multigiocatore

`specials\`, `events\` e `civs\` contano come gameplay: tutti i giocatori devono avere la stessa mod.

## Errori

Compaiono in `Logs\imperivm\imperivm-mod.log` (righe con l'etichetta `specials`) e nella scheda della mod nel gestore dei contenuti: un `id` non valido, un `bit` mancante
o ripetuto, un nome che appartiene a una specialità del gioco.

Attenzione: se aggiungi una `[Sezione]` sconosciuta a `UNIT_SPECIALS.INI` (con una patch, per dare icona e testo a una
specialità), il gioco può chiudersi. Non farlo. Le specialità delle mod non usano quel file.

## Cosa manca

- La barra delle informazioni dell'unità, in alto, dove il gioco mostra fino a tre delle sue specialità con la loro icona, non mostra quelle delle mod.
- Il testo delle statistiche del pulsante di reclutamento dovrebbe aggiungerle sotto quelle del gioco, ma senza il titolo
  «Specials»: un'unità con solo specialità di mod le mostrerebbe senza titolo.
- Icone proprie: non vengono mostrate.
- Non si possono aggiungere alla tabella delle 36 del gioco né creare nuove abilità di eroe native. Quelle attive si
  fanno con [comandi e script](abilita.md).
