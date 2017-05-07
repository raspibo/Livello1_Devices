/*
 * The MIT License
 * 
 * Copyright 2017 Davide (mail4davide@gmail.com)
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated 
 * documentation files (the "Software"), to deal in the 
 * Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, 
 * sublicense, and/or sell copies of the Software, and to 
 * permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall 
 * be included in all copies or substantial portions of the 
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY 
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE 
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
 * PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 * 
 * ============================================================
 * 
 * Sensore pioggia FC-37 + YL-38
 *   Analogica A0
 *   
 * Temperatura e umidita`, per ora DHT11, poi DHT22
 *   Digitale 4
 *   
 * Potenziometro per selezioni/cambio tempi
 *   Analogica A1
 *   
 * Fotoresistenza
 *   Analogica A2
 *   
 * Invio dati ad ESP8266, la versione "serial repeater"
 * 
 * Versione 0.0.5
 * Inizializzazione DHT alla fine del setup
 * 
 * Versione >= 0.0.3
 * - Aggiunto watchdog ("if" del currentMillis)
 * - Modificato on/off per ESP8266
 * 
 *  * Versione >= 0.0.2
 * - Eliminato softwareserial
 */

/*
 * Modifiche da provare:
 * 
 * 2017.4.24
 * Out eneble ESP
 * (Teoricamente funziona, manca di vedere un vero errore della WiFi
 * e di collegare un transistor sul run dell' ESP8266)
 * 
 */

#include <avr/wdt.h>

// Setup DHT
#include "DHT.h"
DHT dht;


int rain = 0;  // Pin analogico sensore pioggia
int ptime = 1;  // Pin analogico potenziometro tempo
int light = 2;  // Pin analogico fotoresistenza

int dhtpin = 6; // Pin DHT22
int runesp = 7; // Pin digitale run ESP
int onesp = 4;  // Pin accensione ESP (alimentatore)

// Altre variabili in uso nel programma
int raining;  // Pioggia
int rainingMem;  // Pioggia
int pValue;  // Valore potenziometro letto
//int pValueMem;  // Memoria
int lValue;  // Valore luminosita` letta
int lValueMem;  // Memoria
float DTemperature;  // Temperatura
float DTemperatureMem;  // Memoria
float DHumidity;  // Umidita`
float DHumidityMem;  // Memoria
int tCycle;  // Valore tempo ciclo
int tCycleMem = 0;  // Memoria
int tCycleDelay = 0;  // Memoria
String espString;  // Stringa da inviare all'ESP8266
String TopicBase = "I/Test/Test/Test/";  // Stringa Topic MQTT
String Topic; // Stringa Topic MQTT
String ID1;  // Stringa per composizione iniziale
String IDend;  // Stringa per composizione finale
String inputString = "";  // Stringa letta dalla seriale
boolean stringIsOk = true;  // Stringa arrivata

/*
 * Lo uso perche` voglio implementare il watchdog, mi serve per
 * i calcoli del tempo passato.
 */
// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time LED was updated
unsigned long runespMillis; // Uscita ESP


void setup() {

  /*
   * Watchdog
   * Disattiva e riattiva a 8 secondi (il massimo)
   */
  wdt_disable();
  wdt_enable(WDTO_8S);
  
  // initialize the serial communication:
  Serial.begin(9600);
  Serial.println();  // linea vuota, voglio vedere se inizia con dei caratteri vuoti ancora..

  // ESP, uscite comando e attivazione
  pinMode(onesp, OUTPUT);
  pinMode(runesp, OUTPUT);

  /*
   * Ritardo accensione ESP, l'ho messo dopo il DHT perche`
   * senno` quest'ultimo non is accende.
   * (Forse e` troppo presto e dovro` spostarlo ancora piu` in la` nel programma)
   */
  // ritardo avvio n/100 perche` e` in ms
  delay(3000);
  digitalWrite(onesp, HIGH);  // Accensione WiFi
  wdt_reset();
  delay(3000);
  digitalWrite(runesp, HIGH);  // Run WiFi
  wdt_reset();
  runespMillis = millis(); // Memorizzo accensione

  delay(3000);
  wdt_reset();

  // Start string
  Topic = TopicBase+"Start";
  ID1 = "{ \"ID\" : \"WDT\", \"Valore\" : \"";
  IDend = "\" }";
  espString = Topic+" "+ID1+String("1")+IDend;
  // Invio dato alla seriale
  Serial.println(espString);
  delay(3000);  // Rimetto il ritardo perche` ho dei disturbi sugli invii dei dati
  wdt_reset();

  // DHT
  dht.setup(dhtpin);  // Dev'essere inizializzato per ultimo, senno` non funziona

  
}


void loop() {

  /*  Controllo il valore del potenziometro per definire 
   *  le tempistiche di lettura.
   *  Voglio sia impostabile in secondi, da 1 a 10 minuti,
   *  10m = 600s
   *  Quindi utilizzero` la funzione map
   *  Il delay e` in millisecondi, quindi moltiplico gia` per 1000
   */
  pValue = analogRead(ptime);
  tCycle = map(pValue, 1, 1023, 0, 6);  // 5 minuti max
  // Nel modo seguente, dovrei stabilizzare le lettura
  /*
   * Se tempo letto e` diverso dal memorizzato
   *   Memorizza il tempo
   * ma lo trasferisce nel delay solo se al ciclo successivo
   * si e` stabilizzato, altrimenti ricalcola, finche` non
   * e` uguale, solo allora lo "conferma".
   */
  if (tCycle != tCycleMem) {
    tCycleMem = tCycle;
    // Timer
    // Creazione stringa da inviare
    Topic = TopicBase+"DelayRead";
    ID1 = "{ \"ID\" : \"KTdelay\", \"Valore\" : \"";
    IDend = "\" }";
    espString = Topic+" "+ID1+String(tCycleMem)+IDend;
    // Invio dato alla seriale
    Serial.println(espString);
    //wdt_reset();
    //Serial.println(tCycle);
    //Serial.println(tCycleMem);
    //Serial.println(tCycleDelay);
  }
  else {
    tCycleDelay = tCycleMem;
  }

  // Tempo corrente
  unsigned long currentMillis = millis();

  /* Controllo se devo resettare il WiFi perche` non manda piu` la stringa "Let's ..."
   * La stringa e` controllata dal Loop Event in fondo al programma, dopo la void loop()
   * Ho aggiunto 60s (60000ms) perche` se il tempo ciclo e` zero ...
   * 
   * Se e` arrivato il tempo (maggiorato di un minuto),
   * controllo se la stringa e` "false", quindi non e` mai arrivata la frase "Let's .." dalla wifi,
   * se e` cosi`, spengo e riaccendo, e mando avviso alla centralina
   * poi metto/rimetto a "false" l'avviso e azzero il tempo per ricominciare un nuovo ciclo di controllo.
   */
  if (currentMillis - runespMillis >= tCycleDelay*60*1000UL+60000) {
    if (!stringIsOk) {
      //Serial.println("Comando_uscita!"); // myDebug
      digitalWrite(runesp, LOW);  // Spengimento WiFi
      delay(2000);
      digitalWrite(runesp, HIGH);  // Riaccensione WiFi
      /*
       * Provo a mettere qua l'avviso e lo mando a level1
       */
      Topic = TopicBase+"Errore";
      ID1 = "{ \"ID\" : \"Error\", \"Valore\" : \"";
      IDend = "\" }";
      espString = Topic+" "+ID1+String(runesp)+IDend;
      // Invio dato alla seriale ESP8266
      Serial.println(espString);
      delay(3000);  // Rimetto il ritardo perche` ho dei disturbi sugli invii dei dati
      wdt_reset();
    }
    /*
     * Azzero il conteggio dei tempi di controllo e
     * predispongo come se non fosse arrivata la stringa
     */
    stringIsOk = false;
    runespMillis=millis();
    //Serial.println("Set_to_False");  // myDebug
  }

  // Precedentemente usato per ritardo ciclo, e` controllato dal potenziometro
  // Tempo ciclo (calcolato all'inizio) da 0 a 5, *60 (s) *1000 (ms)
  //delay(tCycleDelay*60*1000UL); // Si deve specificare UL (unsigned long)

  if (currentMillis - previousMillis >= tCycleDelay*60*1000UL) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // Devo comunque considerare il min
    delay(dht.getMinimumSamplingPeriod());
    wdt_reset();
  
    DTemperature = dht.getTemperature();
    wdt_reset();
    DHumidity = dht.getHumidity();
    wdt_reset();
  
    // send the value RAIN of analog input 0:
    raining = analogRead(rain);
    // Luminosita`
    lValue = analogRead(light);

    // RAIN
    if (raining != rainingMem) {
      // Creazione stringa da inviare - RAIN
      Topic = TopicBase+"Pioggia";
      //String ID = "{ \"ID\" : \"Rain\", \"Valore\" : raining }";
      ID1 = "{ \"ID\" : \"Rain\", \"Valore\" : \"";
      IDend = "\" }";
      espString = Topic+" "+ID1+String(raining)+IDend;
      // Invio dato alla seriale ESP8266
      Serial.println(espString);
      rainingMem = raining;
      delay(3000);  // Rimetto il ritardo perche` ho dei disturbi sugli invii dei dati
      wdt_reset();
    }

    // LIGHT
    if (lValue != lValueMem) {
      // Creazione stringa da inviare
      Topic = TopicBase+"Luce";
      ID1 = "{ \"ID\" : \"Luce\", \"Valore\" : \"";
      IDend = "\" }";
      espString = Topic+" "+ID1+String(lValue)+IDend;
      // Invio dato alla seriale
      Serial.println(espString);
      lValueMem = lValue;
      delay(3000);  // Rimetto il ritardo perche` ho dei disturbi sugli invii dei dati
      wdt_reset();
    }

    // Temperatura
    if (DTemperature != DTemperatureMem) {
      // Creazione stringa da inviare
      Topic = TopicBase+"Temperatura";
      ID1 = "{ \"ID\" : \"STtemp\", \"Valore\" : \"";
      IDend = "\" }";
      espString = Topic+" "+ID1+String(DTemperature)+IDend;
      // Invio dato alla seriale
      Serial.println(espString);
      DTemperatureMem = DTemperature;
      delay(3000);  // Rimetto il ritardo perche` ho dei disturbi sugli invii dei dati
      wdt_reset();
    }

    // Umidita
    if (DHumidity != DHumidityMem) {
      // Creazione stringa da inviare
      Topic = TopicBase+"Umidita";
      ID1 = "{ \"ID\" : \"SThum\", \"Valore\" : \"";
      IDend = "\" }";
      espString = Topic+" "+ID1+String(DHumidity)+IDend;
      // Invio dato alla seriale
      Serial.println(espString);
      DHumidityMem = DHumidity;
      delay(3000);  // Rimetto il ritardo perche` ho dei disturbi sugli invii dei dati
      wdt_reset();
    }

    wdt_reset();
  }
  
  wdt_reset();
}

/*
 * Questa funzione, descritta negli esempi,
 * viene attivata ad ogni ciclo loop, 
 * quindi soffre dei ritardi inclusi in quello.
 */
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    /*
     * Se arriva un carattere 'newline'
     * E` arrivato qualcosa, quindi l'esp sta` funzionando
     */
    if (inChar == '\n') {
      //Serial.println("Newline");
      stringIsOk = true;
      //Serial.println("Set_to_True");
      inputString = "";  // Azzero la stringa
    }
  }
}

