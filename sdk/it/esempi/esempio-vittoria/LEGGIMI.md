# Esempio: modalità di vittoria

Aggiunge una modalità nuova, **Dominio**: vince chi controlla un certo numero di insediamenti. Quel numero viene dal menu a tendina "Limite" della lobby (2, 3, 4 o 5); se lo script non riceve un valore valido, usa 3. L'eliminazione vale ancora: chi resta senza municipi e senza truppe perde.

- `data\GameScripts\6 Dominio.vs`: lo script. Il gioco ne lancia una copia per giocatore (`//void, int player, str param`). Il nome del file, col suo numero, è l'id della modalità: non ripetere quello di un'altra mod né uno del gioco (da 1 a 5).
- `patches\data\const.ini`: riempie il menu a tendina "Limite" di questa modalità. Ogni riga, come `6_Dominio0 = 2 asentamientos, 2`, contiene la chiave (nome del file con `_` al posto degli spazi, più un numero a partire da 0), il testo che si vede e, dopo la virgola, il valore che lo script riceve come `param`.
- Lo script termina la partita di un giocatore con `EndGame(player, false)` (vince) o `EndGame(player, true)` (perde).

I testi dello script sono in inglese e l'esempio non ha traduzioni, quindi nel gioco in italiano compariranno in inglese. Si traducono con un `loc`: vedi [Lingue](../../lingue.md).

Per provarlo, copia questa cartella in `<gioco>\mods\`, attivala nel gestore dei contenuti, riavvia e guarda il menu a tendina "Modalità" in una schermaglia. Guida: [Modalità di vittoria](../../modalita-vittoria.md).
