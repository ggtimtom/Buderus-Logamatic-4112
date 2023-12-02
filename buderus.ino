#define MQTT_MAX_PACKET_SIZE 512

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>

static int counter = 0;
unsigned long lastMsg = 0;
SoftwareSerial myUART =  SoftwareSerial(13,15);

// WiFi
const char *ssid = "WLAN SSID"; // Enter your WiFi name
const char *password = "password";  // Enter WiFi password

// MQTT Topics
const char *mqtt_broker = "MQTT IP";
const char *topicHeizkreis0 = "esp8266/home/Heizkreis0/SENSOR";
const char *topicHeizkreis1 = "esp8266/home/Heizkreis1/SENSOR";
const char *topicWarmwasser1 = "esp8266/home/Warmwasser1/SENSOR";
const char *topicWarmwasser2 = "esp8266/home/Warmwasser2/SENSOR";
const char *topicWarmwasser3 = "esp8266/home/Warmwasser3/SENSOR";
const char *topicKessel = "esp8266/home/Kessel/SENSOR";
const char *topicSolar = "esp8266/home/Solar/SENSOR";
const char *topicStatus = "esp8266/home/Status/SENSOR";

const char *mqtt_username = "mqttuser";
const char *mqtt_password = "Password";
const int mqtt_port = 1883;

const int mqtt_interval = 10000;

//Rx Buffer Heizungssteuerung
const int max_line_length = 36;
static char buffer[max_line_length];

//Die Werte der Heizungssteuerung werden in diesen Variablen gehalten, Doublebuffering.
//Heizkreis0
int Heizkreis0Automatik =256;
int Heizkreis0WarmwasserVorrang=256;
int Heizkreis0Sommer=256;
int Heizkreis0Tag=256;
int Heizkreis0VorlaufSoll=256;
int Heizkreis0VorlaufIst=256;
int Heizkreis0Pumpe= 256;  
int Heizkreis0Stellglied= 256; 
String Heizkreis0Schalter="noData";
//Heizkreis1
int Heizkreis1Automatik=256;
int Heizkreis1WarmwasserVorrang=256;
int Heizkreis1Sommer=256;
int Heizkreis1Tag=256;
int Heizkreis1VorlaufSoll= 256;  
int Heizkreis1VorlaufIst= 256; 
int Heizkreis1Pumpe= 256; 
int Heizkreis1Stellglied= 256; 
String Heizkreis1Schalter="noData";

//Warmwasser
int WwAutomatik=256;
int WwDesinfektion=256;
int WwNachladung=256;
int WwFerien=256;
int WwFehlerDesinfektion=256;
int WwFehlerFühler=256;
int WwFehlerWWbleibtKalt=256;
int WwFehlerAnode=256;            
int WwLaden=256;
int WwManuell=256;
int WwNachladen=256;
int WwAusschaltoptimierung=256;
int WwEinschaltoptimierung=256;
int WwTag=256;
int WwWarm=256;
int WwVorrang=256;
int WwSoll=256;
int WwIst=256;          
int WwLadepumpe=256;
int WwZirkulationspumpe=256;
int WwAbsenkungSolar=256;
String WwSchalter="noData";

//Kessel
int KesselAnlagenvorlaufSoll=256;
int KesselAnlagenvorlaufIst=256;
//Monitorwerte Kessel
int BrennerSollModulationswert=256;
int BrennerIstModulationswert=256;
int KesselSollwert=256;
int KesselIstwert=256;

//Solar
String SolarspeicherUmschaltventil="noData";
int SolarKollektor_ModulationPumpeSpeicher=256;
int Solarspeicher_temperature_sensor=256;
String Solarspeicher1Status="noData";
int SolarspeicherTemperaturMitte=256;
int SolarKollektorTemp=256;
int Aussentemperatur=256;
int GedAussentemperatur=256;

//int counterBeginProtokol=0;
int counterEndProtokol=0;

WiFiClient espClient;
PubSubClient client(espClient);

//Eingehende Bytes auf das Zeilenende prüfen
int readline(int readch, char *buffer, int len)
{
  static int pos = 0;
  int rpos;
  int endflag;

  if (1 == 1) {
    switch (readch) {

      case 0x82: // new line
        if (pos > 1 && buffer[pos-1]== 0xAF)
        {
          rpos = pos;
          pos = 0;  // Reset position index ready for next time
          return rpos;
        }


      default:
        if (pos < len-1) {
          buffer[pos] = readch;
          pos++;
          buffer[pos] = 0;
        }
        else {
          pos = 0;
        }
    }
  }
  // No end of line has been found, so return -1.
  return -1;
}

//Verbindung zum MQTT 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
      String client_id = "esp8266-client-";
      client_id += String(WiFi.macAddress());
      Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
      if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
          Serial.println("mqtt broker connected");
      } else {
          Serial.print("failed with state ");
          Serial.print(client.state());
          delay(2000);
      }
  }
}

void setup() {
  // Set software serial baud to 115200;
  Serial.begin(115200);
 
  // Set the baud rate for the SoftwareSerial object
  myUART.begin(1200);

  // connecting to a WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  //connecting to a mqtt broker !!!????
  client.setServer(mqtt_broker, mqtt_port);

  client.setCallback(callback);
  reconnect();
  // publish and subscribe
  client.publish(topicStatus, "Start");
  //client.subscribe(topicHeizkreis0);
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
      Serial.print((char) payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");
}

void loop() {
  

  while(myUART.available()>0)
  {
    if(readline(myUART.read(), buffer, max_line_length) > 0) {
      //ein Zeilenende wurde gefunden und die Werte stehen im Buffer
      //Jetzt die Werte aus dem Buffer in die Variablen
      if (buffer[0] == 0x80){ //Wenn die empfangene Zeile die erste im Protokoll war
        if(buffer[1] == 0){ 
          //counterBeginProtokol++;
          if(buffer[2] & 4){Heizkreis0Automatik=1;}else{Heizkreis0Automatik=0;}
          if(buffer[2] & 8){Heizkreis0WarmwasserVorrang=1;}else{Heizkreis0WarmwasserVorrang=0;}
          if(buffer[3] & 1){Heizkreis0Sommer=1;}else{Heizkreis0Sommer=0;}
          if(buffer[3] & 2){Heizkreis0Tag=1;}else{Heizkreis0Tag=0;}
          Heizkreis0VorlaufSoll= buffer[4];
          Heizkreis0VorlaufIst= buffer[5];}
        else if(buffer[1] == 6){
          Heizkreis0Pumpe= buffer[4];  
          Heizkreis0Stellglied= buffer[5]; 
          if(buffer[6] & 32){Heizkreis0Schalter="Aus";}
          if(buffer[6] & 64){Heizkreis0Schalter="Hand";}
          if(buffer[6] & 128){Heizkreis0Schalter="Auto";}
        }
      }
         
      else if (buffer[0] == 0x81){
        if(buffer[1] == 0){ 
          if(buffer[2] & 4){Heizkreis1Automatik=1;}else{Heizkreis1Automatik=0;}
          if(buffer[2] & 8){Heizkreis1WarmwasserVorrang=1;}else{Heizkreis1WarmwasserVorrang=0;} 
          if(buffer[3] & 1){Heizkreis1Sommer=1;}else{Heizkreis1Sommer=0;}
          if(buffer[3] & 2){Heizkreis1Tag=1;}else{Heizkreis1Tag=0;}
          Heizkreis1VorlaufSoll= buffer[4];  
          Heizkreis1VorlaufIst= buffer[5]; 
        }
        else if (buffer[1] == 6){
          Heizkreis1Pumpe= buffer[4]; 
          Heizkreis1Stellglied= buffer[5]; 
          if(buffer[6] & 32){Heizkreis1Schalter="Aus";}
          if(buffer[6] & 64){Heizkreis1Schalter="Hand";}
          if(buffer[6] & 128){Heizkreis1Schalter="Auto";}
        }
      }

      //Warmwasser
      else if(buffer[0] == 0x84){
        if(buffer[1] == 0){
          if(buffer[2] & 1){WwAutomatik=1;}else{WwAutomatik=0;}
          if(buffer[2] & 2){WwDesinfektion=1;}else{WwDesinfektion=0;}
          if(buffer[2] & 4){WwNachladung=1;}else{WwNachladung=0;}
          if(buffer[2] & 8){WwFerien=1;}else{WwFerien=0;}
          if(buffer[2] & 16){WwFehlerDesinfektion=1;}else{WwFehlerDesinfektion=0;}
          if(buffer[2] & 32){WwFehlerFühler=1;}else{WwFehlerFühler=0;}
          if(buffer[2] & 64){WwFehlerWWbleibtKalt=1;}else{WwFehlerWWbleibtKalt=0;}
          if(buffer[2] & 128){WwFehlerAnode=1;}else{WwFehlerAnode=0;}
          if(buffer[3] & 1){WwLaden=1;}else{WwLaden=0;}
          if(buffer[3] & 2){WwManuell=1;}else{WwManuell=0;}
          if(buffer[3] & 4){WwNachladen=1;}else{WwNachladen=0;}
          if(buffer[3] & 8){WwAusschaltoptimierung=1;}else{WwAusschaltoptimierung=0;}
          if(buffer[3] & 16){WwEinschaltoptimierung=1;}else{WwEinschaltoptimierung=0;}
          if(buffer[3] & 32){WwTag=1;}else{WwTag=0;}
          if(buffer[3] & 64){WwWarm=1;}else{WwWarm=0;}
          if(buffer[3] & 128){WwVorrang=1;}else{WwVorrang=0;}    
          WwSoll=buffer[4];
          WwIst=buffer[5];
          if(buffer[7] & 1){WwLadepumpe=1;}else{WwLadepumpe=0;}
          if(buffer[7] & 2){WwZirkulationspumpe=1;}else{WwZirkulationspumpe=0;}
          if(buffer[7] & 4){WwAbsenkungSolar=1;}else{WwAbsenkungSolar=0;} 
        } 
        else if(buffer[1] == 6){
          //WwSchalter=buffer[2];
          if(buffer[2] & 32){WwSchalter="0";}
          if(buffer[2] & 64){WwSchalter="Hand";}
          if(buffer[2] & 128){WwSchalter="Auto";}
        } 
      }
       
      //Kessel
      else if(buffer[0] == 0x85 && buffer[1] ==0){
        KesselAnlagenvorlaufSoll=buffer[2];
        KesselAnlagenvorlaufIst=buffer[3];
      }
      //Konfiguration
      else if(buffer[0] == 0x89 && buffer[1] == 0){
        Aussentemperatur=buffer[2];
        GedAussentemperatur=buffer[3];
      }
          //Monitorwerte Kessel
      else if(buffer[0] == 0x92){
        if(buffer[1] == 0){
          BrennerSollModulationswert=buffer[2];
          BrennerIstModulationswert=buffer[3];  
        }
        else if(buffer[1] ==6){
          KesselSollwert=buffer[2];
          KesselIstwert=buffer[3];   
        }
      }      
    
      //Solar
      else if(buffer[0] == 0x9E){
        if(buffer[1] == 0){
          if(buffer[4] & 4){SolarspeicherUmschaltventil="Zu";}
          if(buffer[4] & 8){SolarspeicherUmschaltventil="Auf";}  
          SolarKollektor_ModulationPumpeSpeicher=buffer[7];
        }
        else if(buffer[1] == 6){
          Solarspeicher_temperature_sensor=buffer[2];
          if(buffer[3]==0){Solarspeicher1Status="Gesperrt";}
          if(buffer[3]==1){Solarspeicher1Status="Zu wenig Solarer Ertrag";}
          if(buffer[3]==2){Solarspeicher1Status="Low Flow";}
          if(buffer[3]==3){Solarspeicher1Status="High Flow";}
          if(buffer[3]==4){Solarspeicher1Status="Hand ein";}
          if(buffer[3]==5){Solarspeicher1Status="Umschalt-Check";}
          SolarspeicherTemperaturMitte=buffer[6];
        }
        else if(buffer[1] == 24){
          SolarKollektorTemp=buffer[7];
          counterEndProtokol++; //die letzten gestreamten Daten von der Heizung, da ist etwas Luft im Timing um die Daten zu MQTT zu senden.
          if(counterEndProtokol>2){break;}
        }  
      }
    }
  }
  //
  //Jetzt die MQTT Strings zusammenbauen und wegsenden. Spaghetti Start:  
  //Ist die MQTT Verbindung noch da?
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  //Nicht jeden Durchlauf in MQTT senden um die Datenflut zu reduzieren
  //Alternativ oder zusätzlich könnte man den 8266 auch nach dem Senden in Deepsleep versetzen, siehe unten.
  //unsigned long now = millis();
  //if(lastMsg>now){lastMsg=now;} // sicher ist sicher...
  //if (client.connected() && (now - lastMsg > mqtt_interval)){
  if (client.connected() && (counterEndProtokol>2)){  

    String payload = "{\"Heizkreis0Automatik\":";
    payload += Heizkreis0Automatik;
    payload += ",\"Hk0WWVorrang\":";
    payload += Heizkreis0WarmwasserVorrang;
    payload += ",\"Hk0Sommer\":";
    payload += Heizkreis0Sommer;
    payload += ",\"Hk0Tag\":";
    payload += Heizkreis0Tag;
    payload += ",\"Hk0VorlaufSoll\":";
    payload += Heizkreis0VorlaufSoll;
    payload += ",\"Hk0VorlaufIst\":";
    payload += Heizkreis0VorlaufIst;
    payload += ",\"Hk0Pumpe\":";
    payload += Heizkreis0Pumpe;
    payload += ",\"Hk0Stellglied\":";
    payload += Heizkreis0Stellglied;
    payload += ",\"Hk0Schalter\":\"";
    payload += Heizkreis0Schalter + "\"";
    payload += "}";
    Serial.print("Sending payload: ");
    Serial.println(payload);
    if (client.publish(topicHeizkreis0, (char*) payload.c_str())) {
      Serial.println("Publish ok");
    }
    else {
      Serial.println("Publish failed");
    }

    payload = "{\"Hk1Automatik\":"; 
    payload += Heizkreis1Automatik;
    payload += ",\"Hk1WWVorrang\":";
    payload += Heizkreis1WarmwasserVorrang;
    payload += ",\"Hk1Sommer\":";
    payload += Heizkreis1Sommer;
    payload += ",\"Hk1Tag\":";
    payload += Heizkreis1Tag;
    payload += ",\"Hk1VorlaufSoll\":";
    payload += Heizkreis1VorlaufSoll;
    payload += ",\"Hk1VorlaufIst\":";
    payload += Heizkreis1VorlaufIst;
    payload += ",\"Hk1Pumpe\":";
    payload += Heizkreis1Pumpe;
    payload += ",\"Hk1Stellglied\":";
    payload += Heizkreis1Stellglied;
    payload += ",\"Hk1Schalter\":\"";
    payload += Heizkreis1Schalter + "\"";
    payload += "}";
    Serial.print("Sending payload: ");
    Serial.println(payload);
    if (client.publish(topicHeizkreis1, (char*) payload.c_str())) {
      Serial.println("Publish ok");
    }
    else {
      Serial.println("Publish failed");
    }

    payload = "{\"WwAuto\":"; 
    payload += WwAutomatik;
    payload += ",\"WwDesinfektion\":";
    payload += WwDesinfektion;
    payload += ",\"WwNachladung\":";
    payload += WwNachladung;
    payload += ",\"WwFerien\":";
    payload += WwFerien;
    payload += ",\"WwFehlerDesinfektion\":";
    payload += WwFehlerDesinfektion;
    payload += ",\"WwFehlerFühler\":";
    payload += WwFehlerFühler;
    payload += ",\"WwFehlerWWbleibtKalt\":";
    payload += WwFehlerWWbleibtKalt;
    payload += ",\"WwFehlerAnode\":";
    payload += WwFehlerAnode;
    payload += ",\"WwLaden\":";
    payload += WwLaden;
    payload += "}";
    Serial.print("Sending payload: ");
    Serial.println(payload);
    if (client.publish(topicWarmwasser1, (char*) payload.c_str())) {
      Serial.println("Publish ok");
    }
    else {
      Serial.println("Publish failed");
    }

    payload = "{\"WwManuell\":";
    payload += WwManuell;
    payload += ",\"WwNachladen\":";
    payload += WwNachladen;
    payload += ",\"WwAusschaltoptimierung\":";
    payload += WwAusschaltoptimierung;
    payload += ",\"WwEinschaltoptimierung\":";
    payload += WwEinschaltoptimierung;
    payload += ",\"WwTag\":";
    payload += WwTag;
    payload += ",\"WwWarm\":";
    payload += WwWarm;
    payload += ",\"WwVorrang\":";
    payload += WwVorrang;
    payload += ",\"WwSoll\":";
    payload += WwSoll;
    payload += "}";
    Serial.print("Sending payload: ");
    Serial.println(payload);
    if (client.publish(topicWarmwasser2, (char*) payload.c_str())) {
      Serial.println("Publish ok");
    }
    else {
      Serial.println("Publish failed");
    }

    payload = "{\"KesselAnlagenvorlaufSoll\":";
    payload += KesselAnlagenvorlaufSoll;
    payload += ",\"KesselAnlagenvorlaufIst\":";
    payload += KesselAnlagenvorlaufIst;
    payload += ",\"BrennerSollModulationswert\":";
    payload += BrennerSollModulationswert;
    payload += ",\"BrennerIstModulationswert\":";
    payload += BrennerIstModulationswert;
    payload += ",\"KesselSollwert\":";
    payload += KesselSollwert;
    payload += ",\"KesselIstwert\":";
    payload += KesselIstwert;
    payload += "}";
    Serial.print("Sending payload: ");
    Serial.println(payload);
    if (client.publish(topicKessel, (char*) payload.c_str())) {
      Serial.println("Publish ok");
    }
    else {
      Serial.println("Publish failed");
    }

    payload = "{\"SolarKollektor_ModulationPumpeSpeicher\":";
    payload += SolarKollektor_ModulationPumpeSpeicher;
    payload += ",\"SolarspUmschaltventil\":\"";
    payload += SolarspeicherUmschaltventil + "\"";
    payload += ",\"Solarsp_temp_sensor\":";
    payload += Solarspeicher_temperature_sensor;
    payload += ",\"Solarsp1Status\":\"";
    payload += Solarspeicher1Status;
    payload += "\",\"SolarspTempMitte\":";
    payload += SolarspeicherTemperaturMitte;
    payload += ",\"SolarKollektorTemp\":";
    payload += SolarKollektorTemp;
    payload += ",\"Aussentemp\":";
    payload += Aussentemperatur;
    payload += ",\"GedAussentemp\":";
    payload += GedAussentemperatur;
    payload += "}";
    Serial.print("Sending payload: ");
    Serial.println(payload);   
    if (client.publish(topicSolar, (char*) payload.c_str())) {
      Serial.println("Publish ok");
    }
    else {
      Serial.println("Publish failed");
    }

    payload = "{\"WwIst\":";
    payload += WwIst;
    payload += ",\"WwLadepumpe\":";
    payload += WwLadepumpe;
    payload += ",\"WwZirkulationspumpe\":";
    payload += WwZirkulationspumpe;
    payload += ",\"WwAbsenkungSolar\":";
    payload += WwAbsenkungSolar;
    payload += ",\"WwSchalter\":\"";
    payload += WwSchalter;
    payload += "\"}";
    Serial.print("Sending payload: ");
    Serial.println(payload);
    if (client.publish(topicWarmwasser3, (char*) payload.c_str())) {
      Serial.println("Publish ok");
    }
    else {
      Serial.println("Publish failed");
    }
    //Den 8266 in Deepsleep versetzen.
    //Brücke vom D0 (GPIO16) zum Reset !
    delay(1500); //erstmal fertig machen lassen...
    ESP.deepSleep(40e6); // Zeit in µs!
  }
}
