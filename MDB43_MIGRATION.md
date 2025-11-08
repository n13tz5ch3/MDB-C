# Riepilogo Aggiornamento MDB/ICP Version 4.3

## Panoramica Generale

Il documento CLAUDE.md è stato aggiornato per riflettere le specifiche MDB/ICP Version 4.3 (Settembre 2020). Le modifiche si concentrano **esclusivamente sulla Sezione 7 - Cashless Devices**, mentre tutte le altre sezioni (1-6, 8-10) rimangono identiche alla versione 4.2.

## Le 8 Principali Novità della Sezione 7

### 1. **Segnalazione Numero Articolo** (Item Number Reporting)
**Cosa fa**: Il VMC comunica al dispositivo cashless quale articolo è stato effettivamente erogato.

**Impatto sul codice**:
- Aggiornamento comandi `Vend Success` (7.4.7) e `Cash Sale` (7.4.10)
- Aggiunta campo EVA-DTS PA101 per identificazione standardizzata prodotti
- Tracciabilità completa delle transazioni

### 2. **Vendita Remota** (Remote Vend)
**Cosa fa**: Permette di selezionare e pagare prodotti tramite app mobile senza toccare i pulsanti del distributore.

**Impatto sul codice**:
- Nuova macchina a stati per transazioni avviate da dispositivo cashless
- Comando espansione per richiesta vend remoto
- Gestione timeout e validazione sicurezza

**Caso d'uso**: Cliente scansiona QR code, seleziona prodotto su smartphone, paga, e il distributore eroga automaticamente.

### 3. **Carrello / Multi-Vendita** (Basket / Multi-Vend)
**Cosa fa**: Consente acquisto di più prodotti con una singola transazione (un solo tocco della carta).

**Impatto sul codice**:
- Gestione sessione estesa per vendite multiple
- Array per tracciare articoli e prezzi nel carrello
- Accumulo valore totale transazione
- Segnalazione di ogni articolo erogato individualmente

**Caso d'uso**: Cliente tocca carta NFC, seleziona 3 snack + 2 bevande, paga una volta sola alla fine.

### 4. **Rimborso Parziale** (Partial Refund)
**Cosa fa**: Abilita rimborsi parziali quando alcuni prodotti del carrello non vengono erogati.

**Impatto sul codice**:
- Calcolo automatico importo rimborso
- Notifica al dispositivo cashless con importo
- Gestione scenari di fallimento erogazione
- Tracking articoli previsti vs erogati

**Caso d'uso**: Carrello con 4 articoli, uno non disponibile → rimborso automatico del prezzo del quarto articolo.

### 5. **Supporto Coupon**
**Cosa fa**: Metodo per accettare e processare coupon digitali nel protocollo MDB.

**Impatto sul codice**:
- Nuove strutture dati per coupon (tipo, valore, ID)
- Handler per validazione e riscatto coupon
- Supporto coupon prepagati, QR code vendita gratuita, sconti percentuali
- Logging transazioni con coupon

**Caso d'uso**:
- Scansione QR code per bibita gratuita
- Coupon prepagato da 5€ aggiunge credito alla carta
- Codice sconto 20% su tutti i prodotti

### 6. **Considerazione Informazioni Carta** (Card Information)
**Cosa fa**: Il VMC può usare informazioni della carta (fondi disponibili, tipo pagamento) nel flusso "selezione prima del pagamento".

**Impatto sul codice**:
- Validazione fondi prima di approvare selezione
- Restrizioni prodotto basate su tipo pagamento
- Miglior esperienza utente e tasso successo transazioni

**Caso d'uso**: Cliente seleziona prodotto, sistema verifica fondi sulla carta prima di procedere, evita fallimenti.

### 7. **Informazioni Articolo Potenziate** (Enhanced Item Number - PA101)
**Cosa fa**: Standardizza numerazione articoli con campo EVA-DTS PA101 per migliore integrazione inventario.

**Impatto sul codice**:
- Aggiunta campo PA101 a strutture dati articolo
- Funzioni codifica/decodifica PA101
- Integrazione con database `DTS_DICTIONARY.sql`
- Identificazione consistente attraverso sistemi

### 8. **Flag Vendita Mista** (Mixed Vend Flags)
**Cosa fa**: Nuovo byte nel comando Cash Sale per indicare caratteristiche transazione (solo contanti, solo cashless, misto).

**Impatto sul codice**:
- Aggiunta byte `Mixed Vend Flags` a struttura comando
- Bit flags per tipo pagamento
- Migliore categorizzazione per reporting

## Priorità di Implementazione

Per un'implementazione minima conforme a MDB 4.3:

### Priorità Alta (Fondamentali)
1. **Item Number Reporting** - Base per tutte le altre funzionalità
2. **Enhanced Item Number (PA101)** - Standardizzazione necessaria
3. **Remote Vend** - Alta richiesta mercato, funzionalità chiave moderna

### Priorità Media (Valore aggiunto)
4. **Basket/Multi-Vend** - Esperienza shopping moderna
5. **Coupon Support** - Capacità marketing/promozionale

### Priorità Bassa (Robustezza)
6. **Partial Refund** - Migliora affidabilità sistema
7. **Card Information** - Ottimizzazione flusso selezione
8. **Mixed Vend Flags** - Enhancement reporting

## Compatibilità Retroattiva

**IMPORTANTE**: Tutte le funzionalità MDB 4.3 sono **opzionali e retrocompatibili**:
- Dispositivi annunciano capacità supportate tramite bit di opzione
- VMC e dispositivo cashless negoziano funzionalità durante SETUP
- Dispositivi legacy funzionano normalmente se funzionalità non supportate
- Livelli di Feature (01, 02, 03) indicano crescenti capacità

## File da Modificare

### File Principali
1. **Protocol_Files/PreProcessors.h** - Nuove costanti MDB 4.3
2. **Devices/Cashless Payment Device/Cashless.h** - Strutture dati estese
3. **Devices/Cashless Payment Device/Cashless.cpp** - Handler comandi aggiornati
4. **Devices/Vending Machine Controller/VMC.h** - Logica coordinamento
5. **Protocol_Files/EVA-DTS.h** - Supporto PA101

### Nuovi File da Creare
1. **mdb43_config.h** - Configurazione funzionalità MDB 4.3
2. **MDB43_MIGRATION.md** - Guida migrazione dettagliata
3. **mdb43_features.c** - Implementazioni funzionalità specifiche 4.3

### Database
- **DTS_DICTIONARY.sql** - Aggiungere tabella mappatura PA101

## Roadmap Implementazione

### Fase 1: Fondamenta (Obbligatoria)
- Tracking numero articolo nella macchina a stati VMC
- Codifica/decodifica campo EVA-DTS PA101
- Aggiornamento comando SETUP per negoziazione capacità MDB 4.3
- Strutture supporto Level 03

### Fase 2: Funzionalità Core (Alta priorità)
- Implementazione flusso comando Remote Vend
- Gestione sessione Basket/Multi-Vend
- Strutture dati e handler Coupon

### Fase 3: Funzionalità Avanzate (Media priorità)
- Calcolo e reporting Partial Refund
- Verifica Card Information per selezione-prima
- Aggiunta Mixed Vend Flags a comando Cash Sale

### Fase 4: Test e Integrazione
- Test con dispositivi cashless MDB 4.3 reali
- Validazione compatibilità retroattiva con dispositivi MDB 4.2
- Performance testing transazioni basket
- Test integrazione app mobile per Remote Vend

## Testing Specifico MDB 4.3

### Test Obbligatori
- ✅ Negoziazione capacità SETUP con varie combinazioni
- ✅ Compatibilità retroattiva con dispositivi MDB 4.2
- ✅ Remote Vend: richiesta da dispositivo cashless
- ✅ Basket: 1 articolo, 2 articoli, 5+ articoli
- ✅ Partial Refund: calcolo accuratezza
- ✅ Coupon: validazione e riscatto
- ✅ Item Number: verifica in VEND SUCCESS e CASH SALE
- ✅ PA101: codifica/decodifica corretta

### Test Integrazione
- Test con lettori NFC/EMV contactless reali
- Integrazione app mobile (se Remote Vend implementato)
- Integrazione sistema POS/inventario (campi PA101)
- Integrazione sistema audit (raccolta dati DTS)

## Note Tecniche Importanti

### Comunicazione Seriale
- Rimane 9600 baud, 8-O-1 (invariato da MDB 4.2)
- Protocollo 9-bit (8 data + 1 mode bit) invariato
- Timing inter-byte < 1ms invariato

### Hardware
- Compatibile con implementazione ESP32 esistente
- Nessuna modifica richiesta a MDB_ESP32.c/h
- Tutte le modifiche sono a livello protocollo applicativo

### Configurazione
- Funzionalità MDB 4.3 configurabili a tempo di compilazione
- Feature bits permettono attivazione runtime
- Supporto graceful degradation se funzionalità non disponibili

## Risorse Aggiuntive

Il file CLAUDE.md aggiornato contiene:
- Descrizione dettagliata di ogni funzionalità
- Esempi di codice per ogni modifica
- Diagrammi macchina a stati per nuovi flussi
- Strutture dati complete
- Piano migrazione codice completo

## Prossimi Passi Consigliati

1. **Studiare** il CLAUDE.md aggiornato in dettaglio
2. **Pianificare** l'ordine di implementazione delle funzionalità
3. **Creare** branch git per sviluppo MDB 4.3
4. **Implementare** Fase 1 (fondamenta) prima
5. **Testare** ogni funzionalità incrementalmente
6. **Documentare** le scelte implementative
7. **Validare** con hardware reale prima del rilascio

---

**Nota Finale**: Questa è una migrazione significativa ma ben strutturata. Il protocollo è stato progettato per retrocompatibilità, quindi l'implementazione può essere incrementale. Priorità deve essere data a stabilità e conformità specifica piuttosto che velocità di implementazione.