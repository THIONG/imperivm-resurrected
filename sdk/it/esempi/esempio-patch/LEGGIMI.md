# Esempio: patch

Cambia la caserma romana e una costante senza sostituire nessun file del gioco. Guida: [Patch](../../patch.md).

Cosa fa:

- L'hastatus passa da 100 a 60 di oro, si addestra in 3 secondi invece di 6 e non consuma più popolazione.
- La caserma ha un nuovo ordine, `trainRArcherEjemplo`, che addestra arcieri (tasto `z`).
- `DamageAmplify` passa da 200 a 250 in `CONST.INI`.

Cambia il gameplay: in multigiocatore tutti devono avere la mod.

Per provarlo, copia questa cartella in `<gioco>\mods\`, attivala nel gestore dei contenuti e riavvia. Guarda `Logs\imperivm\imperivm-mod.log`: le righe con l'etichetta `patch` dicono cosa è stato applicato e cosa è fallito.
