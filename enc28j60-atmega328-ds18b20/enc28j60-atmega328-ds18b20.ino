#include <SPI.h>
#include <UIPEthernet.h>
#include <PubSubClient.h>
#include <OneWire.h>


// Update these with values suitable for your network.
byte mac[]    = { 0x19, 0x21, 0x68, 0x00, 0x22, 0x01 };
IPAddress ip(192, 168, 2, 201);
//IPAddress server(192, 168, 2, 11);
char server[] = "level1.home.local";    // name address (using DNS)


EthernetClient ethClient;
PubSubClient client(ethClient);


OneWire  ds(9);  // on pin 10 (a 4.7K resistor is necessary)


int counter = 0;

void setup()
{
  Serial.begin(9600);

  client.setServer(server, 1883);
  //client.setCallback(callback);

  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  Serial.print("Local IP is: ");
  Serial.println(Ethernet.localIP());
  // Allow the hardware to sort itself out
  delay(1500);
}

void loop()
{

  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;
  
  if ( !ds.search(addr)) {
    //Serial.println("No more addresses.");
    //Serial.println();
    ds.reset_search();
    counter=0;
    delay(250);
    return;
  }
  
  //Serial.print("ROM =");
  //for( i = 0; i < 8; i++) {
  //  Serial.write(' ');
  //  Serial.print(addr[i], HEX);
  //}
  counter+=1; // Incrementa numero sensore

  /*
  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return;
  }
  Serial.println();
  
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return;
  } 
  */

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  //Serial.print("  Data = ");
  //Serial.print(present, HEX);
  //Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    //Serial.print(data[i], HEX);
    //Serial.print(" ");
  }
  //Serial.print(" CRC=");
  //Serial.print(OneWire::crc8(data, 8), HEX);
  //Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  /*
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.println(" Celsius.");
  Serial.print("Counter: ");
  Serial.println(counter);
  */
  
  if (client.connect("arduinoClient")) {
    // Composizione della stringa json
    String TempN = "{ \"ID\" : \"ST";
    TempN += counter;
    TempN += "\", \"Valore\" : \"";
    TempN += celsius;
    TempN += "\" }";
    // sta` roba l'ho copiata, serve per convertire una stringa in un array di caratteri
    char charBuf[40];
    TempN.toCharArray(charBuf,40);
    // Once connected, publish an announcement...
    client.publish("I/Arduino/Ethernet/Mobile/Temperatura",charBuf);
    Serial.print(charBuf);
    Serial.println("... to MQTT!");
  } else {
    // Errore:
    Serial.print("failed, rc=");
    Serial.println(client.state());
  }

  // Questo e` il tempo da variare (ogni quanto intendo inviare i dati):
  delay(5000);  // 1000=1s
  client.loop();
}
