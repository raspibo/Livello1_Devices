/*

* SerialToMQTT_Repeater *

Questo programma riceve il testo sulla seriale e lo pubblica sul server MQTT

Il testo dev'essere composto da almeno due stringhe, la prima identifica
il "topic", la seconda il messaggio da scrivere in quel "topic".

Nella prima parte del programma ci sono le varibili da definire:
- SSID della WiFi e relativa password
- Server MQTT e se necessario, la porta e le credenziali per autenticazione

Versione 1.0.0 del 2017.4.25
- La seriale e` configurata per trasmettere/ricevere a 9600 baud

*/

/* LICENSE

The MIT License (MIT)

Copyright (c) 2017 davide

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <user_config.h>
#include <SmingCore/SmingCore.h>

// If you want, you can define WiFi settings globally in Eclipse Environment Variables
#ifndef WIFI_SSID
	//#define WIFI_SSID "PleaseEnterSSID" // Put you SSID and Password here
	//#define WIFI_PWD "PleaseEnterPass"
#endif

// ... and/or MQTT username and password
#ifndef MQTT_USERNAME
	#define MQTT_USERNAME ""
	#define MQTT_PWD ""
#endif

// ... and/or MQTT host and port
#ifndef MQTT_HOST
	#define MQTT_HOST "Please Enter MQTT Broker"
	#define MQTT_PORT 1883
#endif

// Forward declarations
void startMqttClient();
void onDataCallback(Stream& stream, char arrivedChar, unsigned short availableCharsCount);

Timer procTimer;

// MQTT client
// For quick check you can use: http://www.hivemq.com/demos/websocket-client/ (Connection= test.mosquitto.org:8080)
MqttClient mqtt(MQTT_HOST, MQTT_PORT);

// Check for MQTT Disconnection
void checkMQTTDisconnect(TcpClient& client, bool flag){
	
	// Called whenever MQTT connection is failed.
	if (flag == true)
		Serial.println("MQTT Broker Disconnected!!");
	else
		Serial.println("MQTT Broker Unreachable!!");
	
	// Restart connection attempt after few seconds
	procTimer.initializeMs(2 * 1000, startMqttClient).start(); // every 2 seconds
}

// Legge lo stream della seriale e lo mette sulla variabile "Message" come stringa
// Credo che questa funzione sia in un ".h/.cpp" del framework. Funziona ? La uso ;)
// Aggiungo l'invio del dato al broker mqtt.
void onDataCallback(Stream& stream, char arrivedChar, unsigned short availableCharsCount)
{
  String MessageP = "";	// Inizializzo stringa publish, DOVE pubblicare
  String MessageS = "";	// Inizializzo stringa string, COSA pubblicare

  int countspace = 0;	// Mi serve per sapere se ho incontrato il primo carattere spazio, per dividere la stringa
  if (arrivedChar == '\n')
  {
    while (stream.available())
    {
      char cur = stream.read();
      // Ci infilo in mezzo il controllo carattere ?
      /*
      if ( cur >= '.' && cur <= 'z' ) {
        Serial.print("E` un carattere (ok)");
      }
      else {
        Serial.print("Non e` un carattere - ERROR");
      }
      */
      if (countspace == 1)
      {
        MessageS += cur;
      }
      if (countspace == 0)
      {
        if ( cur == ' ')
        {
          countspace = 1;
        }
        else
        {
          MessageP += cur;
        }
      }
    //Serial.print(cur);
    //Message += cur;   // Aggiunge caratteri (cur) alla stringa
    }

    /* Forse questo non serve piu` ? */
    //Message.remove(Message.length()-2);   // Elimino dalla stringa i caratteri "\" e "n", perche` inviano un CR

    /* Come devo manipolare "Message" per ottenere i parametri che mi servono ? */

    if (mqtt.getConnectionState() != eTCS_Connected)
        startMqttClient(); // Auto reconnect

    Serial.println("Let's publish message now!");

    Serial.print("MessaggioP: ");
    Serial.println(MessageP);
    Serial.print("MessaggioS: ");
    Serial.println(MessageS);

    /*
    Controllo dei caratteri nella stringa, causa problema di lettura di MQTT
    da parte del client utilizzato (python paho mqtt client) che andava in
    crash:
        message.topic = message.topic.decode('utf-8')
    UnicodeDecodeError: 'utf-8' codec can't decode byte 0xe1 in position 3: invalid continuation byte
    */
    int Error = 0;  // Inizializzo variabile di errore
    //Serial.print("Error:");
    //Serial.println(Error);
    // Cerco l'errore
    for (int i = 0; i < strlen(MessageP.c_str()); i++)
    {
        //Serial.println(MessageP[i]);
        // Se non e` un carattere ascii riconosciuto ..
        if (!( MessageP[i] >= ' ' && MessageP[i] <= '~' ))
        {
            //Serial.println(MessageP[i]);
            //Serial.println("Error char P");
            Error = 1;  // Se c'e` errore
        }
    }
    //Serial.print("Error:");
    //Serial.println(Error);
    // Controllo se esiste la seconda stringa, senno` l'ESP va in banana !
    if ( MessageS != "" )
    {
        // Tolgo due caratteri, il "\n" finale della stringa
        for (int i = 0; i < strlen(MessageS.c_str())-2; i++)
        {
            //Serial.println(MessageS[i]);
            // Se non e` un carattere ascii riconosciuto ..
            if (!( MessageS[i] >= ' ' && MessageS[i] <= '~' ))
            {
                //Serial.println(MessageS[i]);
                //Serial.println("Error char S");
                Error = 1;  // Se c'e` errore
            }
        }
    }
    //Serial.println(Error);
    
    
	//MessageS.remove(MessageS.length()-2);	// Elimino dalla stringa i caratteri "\" e "n", perche` inviano un CR	// NON FUNZIONA ? Mi ha tolto anche la }
	
	//mqtt.publish(MQTT_PUBLISH, "{ \"ID\" : \"" + String(ID) + "\", \"Valore\" : \"" + String(Message) + "\" }"); // or publishWithQoS
	/* Quella sopra e` la vecchia stringa
	Adesso (sopra) devo vedere COME per dividere la stringa in arrivo ed ottenere due parametri: PUBLISH e MESSAGE
	*/
	//mqtt.publishWithQoS(String(MessageP),String(MessageS),1);	// Il qos 2 non va proprio, l'1 non mi sembra che vada cosi` bene, torno a ..
    // Pubblico il dato solo se non ho trovato errori, altrimenti mando un messaggio di errore alla seriale.
    if (Error == 0)
    {
	    mqtt.publish(String(MessageP),String(MessageS));
    }
    else
    {
        Serial.println("*** Non e` stato pubblicato per un'errore sulla stringa ***");
    }
  }
}

// Run MQTT client
void startMqttClient()
{
	procTimer.stop();
	if(!mqtt.setWill("last/will","The connection from this device is lost:(", 1, true)) {
		debugf("Unable to set the last will and testament. Most probably there is not enough memory on the device.");
	}
	mqtt.connect("esp8266", MQTT_USERNAME, MQTT_PWD);
	// Assign a disconnect callback function
	//mqtt.setCompleteDelegate(checkMQTTDisconnect); // Non funziona e non c'e` piu` nel nuovo esempio
}

// Will be called when WiFi station was connected to AP
void connectOk()
{
	Serial.println("I'm CONNECTED");

	// Praticamente e` tutto qua!
	Serial.setCallback(onDataCallback);

	// Run MQTT client
	startMqttClient();

}

// Will be called when WiFi station timeout was reached
void connectFail()
{
	Serial.println("I'm NOT CONNECTED. Need help :(");

	// .. some you code for device configuration ..
}

void init()
{
	//Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.begin(9600);
	Serial.systemDebugOutput(false); // Debug output to serial

	WifiStation.config(WIFI_SSID, WIFI_PWD);
	WifiStation.enable(true);
	WifiAccessPoint.enable(false);

	// Run our method when station was connected to AP (or not connected)
	WifiStation.waitConnection(connectOk, 20, connectFail); // We recommend 20+ seconds for connection timeout at start
}
