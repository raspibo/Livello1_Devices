/*
 * Sensore pioggia FC-37 + YL-38
 *   Analogica A0
 *   
 * Temperatura e umidita`, per ora DHT11, poi DHT22
 *   Digitale 4
 *   
 * Potenziometro per selezioni/cambio tempi
 *   Analogica A1
 *   
 * Invio dati ad ESP8266, la versione "serial repeater"
 */

// Setup DHT
#include "DHT.h"
DHT dht;

// Seriale per ESP8266
#include <SoftwareSerial.h>
SoftwareSerial ESP(2,3);  // RX e` il 2, TX e` il 3


int rain = 0;  // Pin analogico sensore pioggia
int ptime = 1;  // Pin analogico potenziometro tempo

// Variabili in uso nel programma
int pValue;  // Valore potenziometro letto
int pValueTmp;  // Temporaneo
int tCycle;  // Valore tempo ciclo
int tCycleMem = 1;  // Memoria
int tCycleDelay = 1;  // Memoria
String espString;  // Stringa da inviare all'ESP8266
String Topic;  // Stringa Topic MQTT
String ID1;  // Stringa per composizione iniziale
String IDend;  // Stringa per composizione finale

void setup() {
  // initialize the serial communication:
  Serial.begin(115200);

  // serial ESP8266
  ESP.begin(9600);  // Ho abbassato la velocita` perche` comparivano strani caratteri
  ESP.setTimeout(15000);  // C'era nell'esempio

  // DHT
  dht.setup(4);  // Data pin 4
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
  tCycle = map(pValue, 1, 1023, 1, 100);
  // Nel modo seguente, dovrei stabilizzare le lettura
  Serial.print("tCycle: ");
  Serial.println(tCycle);
  Serial.print("tCycleMem: ");
  Serial.println(tCycleMem);
  Serial.print("tCycleDelay: ");
  Serial.println(tCycleDelay);
  if (tCycle != tCycleMem) {
    if (1 < tCycleMem < 101) {
          tCycleMem = tCycle;
    }
    else {
      Serial.println("Error tCycle");
    }
  }
  else {
    Serial.println("Il ciclo totale e` maggiorato di 10 secondi dovuto ai \"delay\" nel programma");
    Serial.print("Tempo ciclo: ");
    Serial.println(tCycle+10);  // Sono 10 secondi in piu`
    tCycleDelay = tCycleMem;
  }
  
  // Devo comunque considerare il min
  delay(dht.getMinimumSamplingPeriod());
  Serial.print("Tempo per campionamento (DHT): ");
  Serial.println(dht.getMinimumSamplingPeriod());
  
  Serial.print("Temperature: ");
  float DTemperature = dht.getTemperature();
  Serial.println(DTemperature);
  Serial.print("Humidity: ");
  float DHumidity = dht.getHumidity();
  Serial.println(DHumidity);
  
  // send the value RAIN of analog input 0:
  int raining = analogRead(rain);
  Serial.print("Raining: ");
  Serial.println(raining);

  // Creazione stringa da inviare - RAIN
  Topic = "I/Casa/PianoTerra/SalaGrande/Pioggia";
  //String ID = "{ \"ID\" : \"Rain\", \"Valore\" : raining }";
  ID1 = "{ \"ID\" : \"Rain\", \"Valore\" : \"";
  IDend = "\" }";
  espString = Topic+" "+ID1+String(raining)+IDend;
  // Invio dato alla seriale ESP8266
  Serial.println(espString);
  ESP.println(espString);
  delay(3000);  // Sembra che abbia risolto tutti i problemi

  // Creazione stringa da inviare - Temperatura
  Topic = "I/Casa/PianoTerra/SalaGrande/Temperatura";
  ID1 = "{ \"ID\" : \"STtemp\", \"Valore\" : \"";
  IDend = "\" }";
  espString = Topic+" "+ID1+String(DTemperature)+IDend;
  // Invio dato alla seriale ESP8266
  Serial.println(espString);
  ESP.println(espString);
  delay(3000);
  
  // Creazione stringa da inviare - Umidita`
  Topic = "I/Casa/PianoTerra/SalaGrande/Umidita";
  ID1 = "{ \"ID\" : \"SThum\", \"Valore\" : \"";
  IDend = "\" }";
  espString = Topic+" "+ID1+String(DHumidity)+IDend;
  // Invio dato alla seriale ESP8266
  Serial.println(espString);
  ESP.println(espString);
  delay(3000);

  /*
  if (ESP.available()) {
    Serial.write(ESP.read());
  }
  */
  
  
  // Tempo ciclo (calcolato all'inizio)nga da inviare
  Serial.print("Tempo ciclo effettivo (millisecondi): ");
  Serial.println((tCycleDelay+10)*1000UL);  // Si deve specificare UL (unsigned long)
  delay((tCycleDelay+10)*1000UL);
  //delay(3000);  // HO DOVUTO DISATTIVARE LA VARIABILE, CI SONO PROBLEMI CON L'ESP

  // Linee vuote
  Serial.println("\n");

}

