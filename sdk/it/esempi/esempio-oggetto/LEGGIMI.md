# Esempio: oggetto nuovo

Aggiunge un oggetto, l'Amuleto de ejemplo (+10 di danno e +5 di armatura), e fa in modo che ogni eroe lo riceva quando compare sulla mappa.

- `patches\data\items.xml`: l'oggetto, aggiunto all'elenco del gioco. Ogni `<item>` ha un `id` (il nome con cui lo chiamano gli script), un'
  icona (`image`), un testo (`description`) e i suoi bonus (`<bonus>`: vita, danno, armature, livello, esperienza). Un oggetto con effetto proprio ha
  in più `use_script="file://data/mod/mi.vs"`; quelli del gioco sono in `data\ItemScripts` (estraili con `estrai-dati.js`, vedi
  [Patch](../../patch.md)).
- `patches\data\classes\Hero.sc.xml`: un comportamento in più per la classe `Hero`.
- `data\mod\dai_amuleto.vs`: ciò che fa quel comportamento, `Obj::AddItem("Amuleto de ejemplo")`.

Se il
gioco non trova l'icona o lo script fallisce, lo segnala `Logs\vxlog.log`.

Per provarlo: copia questa cartella in `<gioco>\mods\`, attivala nel gestore dei contenuti, riavvia e inizia una schermaglia con un eroe qualsiasi. In
`Logs\imperivm\imperivm-mod.log` deve comparire «Un heroe ha recibido el Amuleto de ejemplo», e l'eroe avrà l'oggetto nell'inventario.

Altri modi per dare un oggetto: `Obj::AddItem` da qualsiasi script (per esempio un `OnObjectKill` che lo fa cadere quando si uccide), oppure metterlo
nelle rovine e nei tesori della mappa (guarda `data\classes\BaseRuins.sc.xml`). Guida: [Oggetti nuovi](../../oggetti.md).
