# Esempio: formazione nuova

Aggiunge una formazione per gli eroi, il **Cuneo** (+3 di danno e +1 di armatura; la cavalleria davanti). Si sceglie come le altre, dal pulsante formazione dell'eroe.

- `patches\data\formations.xml`: la formazione (dimensione, bonus e dove si dispone ogni classe di unità).
- `patches\data\commands\Hero.xml`: i suoi due pulsanti, `form-wedge-menu` (quello che si vede quando è l'attuale) e `form-wedge` (quello che la sceglie, tasto `i`). Usano il disegno della formazione Front.
- `patches\data\subai\hero_formation_verify.vs`: patch dello script del gioco, che lascia scegliere la formazione solo con i suoi quattro menu; aggiunge quello nuovo.

I bonus valgono solo con le truppe ferme ("stand ground"). Il testo del pulsante lo mette il gioco; il `Description` dell'XML non viene mostrato.

Per provarlo, copia questa cartella in `<gioco>\mods\`, attivala nel gestore dei contenuti, riavvia, inizia una schermaglia, metti delle truppe agli ordini di un eroe e apri il suo menu delle formazioni. Guida: [Formazioni](../../formazioni.md).
