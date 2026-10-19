# Example: custom unit special

A new unit special, Critical hit (bit 36), assigned to the Roman Hastatus: one in four of its blows does triple damage.

| File | What it is |
|---|---|
| `specials\critical.json` | The declaration: name, bit and description. |
| `patches\data\classes\RHastatus.sc.xml` | Assigns it to the Hastatus with `unit_specials="Deflection, Critical"`. |
| `events\OnAttackResolved.vs` | The effect: on every blow, if the attacker has it, 25% of the time the damage is tripled. |

To test it: copy this folder to `<game>\mods\`, enable it in the content manager and restart. In `Logs\imperivm\imperivm-mod.log`, `specials: registered Critical (bit 36…)` appears at startup, and `script: Critical hit: damage N` each time a Hastatus lands one.

It changes gameplay: every player has the mod. Guide: [Unit specials](../../specials.md).
