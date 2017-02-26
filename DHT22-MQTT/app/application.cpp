#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <Libraries/DHT/DHT.h>

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
	#define MQTT_HOST "EnterMqttServerName"  // MQTT Server
	#ifndef ENABLE_SSL
		#define MQTT_PORT 1883
	#else
		#define MQTT_PORT 8883
	#endif
#endif

/* (My) User settings */
//#define MQTT_PUBLISH "I/Casa/Cantina/Fumetti"	// Percorso di pubblicazione dei dati MQTT, senza "Tipo"
#define MQTT_PUBLISHH "I/Casa/Cantina/Fumetti/Umidita"	// Percorso di pubblicazione dei dati MQTT, senza "Tipo"
#define MQTT_PUBLISHT "I/Casa/Cantina/Fumetti/Temperatura"	// Percorso di pubblicazione dei dati MQTT, senza "Tipo"
#define MQTT_SUBSCRIBE "I/#"			// Percorso di lettura dei dati MQTT
#define MQTT_ID "TH"										// Identificatore

#define WORK_PIN 2 // GPIO2

DHT dht(WORK_PIN);


// Forward declarations
void startMqttClient();
//void onMessageReceived(String topic, String message);
void publishMessage();	// Serve questa dichiarazione ?

Timer procTimer;

// MQTT client
// For quick check you can use: http://www.hivemq.com/demos/websocket-client/ (Connection= test.mosquitto.org:8080)
MqttClient *mqtt;

/*
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
*/

/*
void onMessageDelivered(uint16_t msgId, int type) {
	Serial.printf("Message with id %d and QoS %d was delivered successfully.", msgId, (type==MQTT_MSG_PUBREC? 2: 1));
}
*/

// Publish our message
void publishMessage()
{
	Serial.println("\t\t DHT improved lib");
	Serial.println("wait 1 second for the sensor to boot up");
	
	//disable watchdog
	WDT.enable(false);
	//wait for sensor startup
	delay(1000);

	dht.begin();

	/*first reading method (Adafruit compatible) */
	Serial.print("Read using Adafruit API methods\n");
	float h = dht.readHumidity();
	float t = dht.readTemperature();

	// check if returns are valid, if they are NaN (not a number) then something went wrong!
	if (isnan(t) || isnan(h))
	{
		Serial.println("Failed to read from DHT");
		// Ho dovuto aggiungerla anche qua, perche` senno` si blocca i timer, non so perche`.
		if (mqtt->getConnectionState() != eTCS_Connected)
			startMqttClient(); // Auto reconnect
	} else {
		Serial.print("\tHumidity: ");
		Serial.print(h);
		Serial.print("% Temperature: ");
		Serial.print(t);
		Serial.print(" *C\n");
		
		if (mqtt->getConnectionState() != eTCS_Connected)
			startMqttClient(); // Auto reconnect
		
		Serial.println("Let's publish messages now!");
		Serial.println(String(MQTT_PUBLISHH) + " ID:" + String(MQTT_ID) + " Valore:" + String(h));
		mqtt->publish(MQTT_PUBLISHH, "{ \"ID\" : \"" + String(MQTT_ID) + "\", \"Valore\" : \"" + String(h) + "\" }");
		Serial.println(String(MQTT_PUBLISHT) + " ID:" + String(MQTT_ID) + " Valore:" + String(t));
		mqtt->publish(MQTT_PUBLISHT, "{ \"ID\" : \"" + String(MQTT_ID) + "\", \"Valore\" : \"" + String(t) + "\" }");
		Serial.println("******************************************");
		
		//mqtt->publish("main/frameworks/sming", "Hello friends, from Internet of things :)"); 
		
		//mqtt->publishWithQoS("important/frameworks/sming", "Request Return Delivery", 1, false, onMessageDelivered); // or publishWithQoS
	}
}

/*
// Callback for messages, arrived from MQTT server
void onMessageReceived(String topic, String message)
{
	Serial.print(topic);
	Serial.print(":\r\n\t"); // Pretify alignment for printing
	Serial.println(message);
}
*/

// Run MQTT client
void startMqttClient()
{
	// Blocco, secondo me non lo devo piu` fermare, per come ho fatto il software
	//procTimer.stop();
	if(!mqtt->setWill("last/will","The connection from this device is lost:(", 1, true)) {
		debugf("Unable to set the last will and testament. Most probably there is not enough memory on the device.");
	}
	mqtt->connect("esp8266", MQTT_USERNAME, MQTT_PWD, true);
#ifdef ENABLE_SSL
	mqtt->addSslOptions(SSL_SERVER_VERIFY_LATER);

	#include <ssl/private_key.h>
	#include <ssl/cert.h>

	mqtt->setSslClientKeyCert(default_private_key, default_private_key_len,
							  default_certificate, default_certificate_len, NULL, true);

#endif

	// Assign a disconnect callback function
	//mqtt->setCompleteDelegate(checkMQTTDisconnect);
	//mqtt->subscribe(MQTT_SUBSCRIBE);
}

// Will be called when WiFi station was connected to AP
void connectOk()
{

	// Run MQTT client
	startMqttClient();

	// Start publishing loop
	procTimer.initializeMs(59 * 1000, publishMessage).start(); // every 59 seconds
}

// Will be called when WiFi station timeout was reached
void connectFail()
{
	Serial.println("I'm NOT CONNECTED. Need help :(");

	// .. some you code for device configuration ..
}

void init()
{
	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(true); // Debug output to serial

	//mqtt = new MqttClient(MQTT_HOST, MQTT_PORT, onMessageReceived);
	mqtt = new MqttClient(MQTT_HOST, MQTT_PORT);
	WifiStation.config(WIFI_SSID, WIFI_PWD);
	WifiStation.enable(true);
	WifiAccessPoint.enable(false);

	// Run our method when station was connected to AP (or not connected)
	WifiStation.waitConnection(connectOk, 20, connectFail); // We recommend 20+ seconds for connection timeout at start

}
