/*Kilburn Nerf Experience Balance Beam Code
 * Contributors Joe Mertz, Bob McGrath and Josh Katz @ L3DFX in Bolingbrook Illinois
 *        Joe Mertz www.github.com/jcmertz
 *       Bob McGrath www.github.com/bobwmcgrath
 *       L3DFX www.l3dfx.com
 *  Version 0.1
 *  Date: 9/23/19
*/

#define DEBUG

#include <SPI.h> // Required for Ethernet
#include <Ethernet.h>
#include <PubSubClient.h> //This is the MQTT Library

// Debugging Code. Enables debug printer if DEBUG flag is enabled
#ifdef DEBUG
#define DEBUG_PRINT(x)  Serial.print (x)
#define DEBUG_PRINTLN(x)  Serial.println (x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

//<--------------------------------------------------
const int b1=27;
const int b2=14;
const int bigButtonLED = 26;
const int blinkDuration=500;
unsigned int lastTime=0; 
int resettime =0;
int resetTimer=3000;
int ledState=LOW;
bool PenPress=false;
bool StartPress=false;
bool gameOn = false;

int function=0;
//<-------------------------------------------------------------------------------------------------------------------------------------------
// mac and MQTT_ID must be unique for each device in an activation

byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0x0A};
char clientName[] = "caterpie";

//<-------------------------------------------------------------------------------------------------------------------------------------------

const char* MQTT_ID = (char*)mac;

//MQTT Broker/Server IP address
IPAddress server(10, 100, 100, 10); // This should be the same within each activation


//Set Up MQTT Client
EthernetClient ethClient;
PubSubClient client(ethClient);

void callback(char* topic, byte* payload, unsigned int length) {
  DEBUG_PRINT("Message arrived [");
  DEBUG_PRINT(topic);
  DEBUG_PRINT("] ");
  for (int i = 0; i < length; i++) {
    DEBUG_PRINTLN((char)payload[i]);
  }
  if ((String)topic == "activation/reset") {
    gameOn = false;
  }
  else if ((String)topic == "timer/end") {
    gameOn = false;
  }
  else if ((String)topic == "timer/start") {
    gameOn = true;
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientName)) {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe("activation/reset");
      client.subscribe("timer/start");
      client.subscribe("timer/end");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      Ethernet.maintain();
      DEBUG_PRINTLN(Ethernet.localIP());
      DEBUG_PRINTLN(Ethernet.gatewayIP());
    }
  }
}
void publishToTopic(String subTopic, String message) {
  char subTopicOut[subTopic.length() + 1];
  subTopicOut[subTopic.length()] = '\0';
  subTopic.toCharArray(subTopicOut, subTopic.length() + 1);

  char messageOut[message.length() + 1];
  messageOut[message.length()] = '\0';
  message.toCharArray(messageOut, message.length() + 1);

  client.publish(subTopicOut, messageOut);
}
//<---------------------------------------------------

void senseBigButton(){
  int state = digitalRead(b1);
  

  if(!state && !StartPress){
    if(function ==1){
      publishToTopic("timer/start","Start Button");
      gameOn=true;
    }else if(function == 2){
      publishToTopic("timer/end","End Button");
      gameOn=false;
    }else if(function == 3){
      if(gameOn){
        publishToTopic("player1/score","+1");
        gameOn=false;
      }else{
        publishToTopic("timer/start","Start Button");
        gameOn=true;
      }
      StartPress=true;
    }
  }else if(state && StartPress){
    StartPress=false;
  }
}

void sensePenButton(){
  int state=digitalRead(b2);
  if(!state && !PenPress){
    publishToTopic("player1/score","-1");
    PenPress=true;
  }else if(state && PenPress){

    PenPress=false;
  }
  if(!state && PenPress){
    if(millis() > resettime +resetTimer){   
     publishToTopic("activation/reset","RESET BUTTON");
     resettime=millis();
     PenPress=false;
   
    }
    
  }
}
//<----------------------------------------------------------------------------

void setup() {

Serial.begin(57600);
//Configure IO
pinMode(b1,INPUT_PULLUP);
pinMode(b2,INPUT_PULLUP);
pinMode(bigButtonLED, OUTPUT);

  //Init MQTT
  client.setServer(server, 1883);
  client.setCallback(callback);
  Ethernet.init(33);  // ESP32 with Adafruit Featherwing Ethernet

  Serial.println("Initialize Ethernet with DHCP:");
 
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // no point in carrying on, so do nothing forevermore:
    delay(5000);
    ESP.restart();  
  }

  // Allow the hardware to sort itself out
  delay(1500);

//  Load config from IP ADDRESS
  DEBUG_PRINTLN(Ethernet.localIP());
  DEBUG_PRINTLN(Ethernet.gatewayIP());
  int index = Ethernet.localIP().toString().indexOf('.');
  index = Ethernet.localIP().toString().indexOf('.', index + 1) + 1;
  int endex = Ethernet.localIP().toString().indexOf('.', index);
  String octet = Ethernet.localIP().toString().substring(index, endex);
  DEBUG_PRINTLN(octet);
  function = octet.toInt();


  if (function == 1) {
    digitalWrite (bigButtonLED, HIGH);
  }
  else if (function == 2) {
    digitalWrite (bigButtonLED, LOW);
  }


  // Establish MQTT Broker connection and subscribe to reset topic


}

void loop() {


if(function==1){
  if(!gameOn){
    digitalWrite(bigButtonLED,HIGH);
    senseBigButton();
    sensePenButton();
  }
  if(gameOn){
    digitalWrite(bigButtonLED,LOW);
    sensePenButton();
  }
}else if(function ==2){
    if (!gameOn)
    { digitalWrite(bigButtonLED, LOW);
      sensePenButton();
    }
    if (gameOn)
    { digitalWrite(bigButtonLED, HIGH);
      senseBigButton();
      sensePenButton();
    }
}else if(function ==3){
    if (!gameOn)
    { digitalWrite(bigButtonLED, HIGH);
      senseBigButton();
      sensePenButton();
    }
    if (gameOn)
    {
      if ( millis() > lastTime + blinkDuration) {
        ledState = !ledState;
        digitalWrite(bigButtonLED, ledState);
        lastTime = millis();
      }
      senseBigButton();
      sensePenButton();
    }  
}

  // Establish MQTT Broker connection and subscribe to reset topic

  if (!client.connected()) {
    reconnect();
  }

  // check MQTT messages
  client.loop();
  // if reset: reset to top of game
  // if MQTT_ID: load new params
  
}
