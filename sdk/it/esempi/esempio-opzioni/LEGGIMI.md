# Esempio: opzioni di partita personalizzate

Aggiunge due opzioni alle REGOLE della lobby: un interruttore, **Héroes veloces** (+50 %), e una lista, **Velocidad de héroes** (Normal, Rápida +25 %, Muy rápida +50 %).

- `options\heroes-veloces.json` e `options\velocidad-heroes.json`: le opzioni, con nome e aiuto per lingua e il loro valore di default. L'`id` è quello che usano gli script.
- `data\mod\eroi_veloci.vs` e `patches\data\classes\Hero.sc.xml`: un comportamento della classe `Hero` che legge `Mod_GetOption("heroes-veloces")` e `Mod_GetOptionValue("velocidad-heroes")` e accelera l'eroe.

Compaiono nella lobby per tutti i giocatori, ma le cambia solo l'host. Viaggiano con le altre regole e si salvano con la partita. Tra tutte le mod attive ci sono 8 bit per le opzioni: un interruttore ne consuma 1 e una lista da 1 a 4. Fuori dalla lobby (avventure, campagne) valgono sempre 0.

Per provarlo, copia questa cartella in `<gioco>\mods\`, attivala nel gestore dei contenuti, riavvia, crea una schermaglia e spunta "Héroes veloces" in REGOLE. Con `Mod_LogInt` lo script lascia una riga per eroe in `Logs\imperivm\imperivm-mod.log`. Guida: [Opzioni di partita](../../opzioni.md).
