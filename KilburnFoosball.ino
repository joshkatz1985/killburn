

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

const int startButton = 14;
const int resetButton = 32;
const int teamOnePlus = 27;
const int teamOneMinus = 26;
const int teamTwoPlus = 25;
const int teamTwoMinus = 13;
const int bigButtonLED = 2;
unsigned long lastTime = 0;

int function=0;
bool gameOn = false;

//Button Latches
bool StartPress = false;
bool ResetPress = false;
bool ScorePress1 = false;
bool ScorePress2 = false;
bool ScorePress3 = false;
bool ScorePress4 = false;


//<--------------------------------------------------------------
// mac and MQTT_ID must be unique for each device in an activation
//Needs to be changed
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0x3F};
char clientName[] = "abra";

//<-----------------------------------------------
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

void senseBigButton(){
  int state = digitalRead(startButton);
  if(!state && !StartPress){
    if(function ==1){
      publishToTopic("timer/start","Start Button");
      gameOn=true;
    }else if(function == 2){
      publishToTopic("timer/end","End Button");
      gameOn=false;
    }else if(function == 3){
      if(gameOn){
        publishToTopic("timer/end","End Button");
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

void senseResetButton(){
  int state = digitalRead(resetButton);
  if(!state && !ResetPress){

    DEBUG_PRINTLN("Reset Button Pressed");
    publishToTopic("activation/reset","RESET BUTTON");
    gameOn=false;    
    ResetPress=true;
  }else if(state && ResetPress){
    ResetPress=false;
  }
}

void Score(){
  int state1 = digitalRead(teamOnePlus);
  int state2 = digitalRead(teamOneMinus);
  int state3 = digitalRead(teamTwoPlus);
  int state4 = digitalRead(teamTwoMinus);
  

  if(gameOn){
  if(!state1 && !ScorePress1){
  publishToTopic("player1/score","+1" );
    ScorePress1=true;
  }else if(state1 && ScorePress1){
    ScorePress1=false;
  }
  if(!state2 && !ScorePress2){
 publishToTopic("player1/score","-1" );
   
    ScorePress2=true;
  }else if(state2 && ScorePress2){
    ScorePress2=false;
  }
  if(!state3 && !ScorePress3){
 publishToTopic("player2/score","+1" );
    ScorePress3=true;
  }else if(state3 && ScorePress3){
    ScorePress3=false;
  }
  if(!state4 && !ScorePress4){
 publishToTopic("player2/score","-1" );

    ScorePress4=true;
  }else if(state4 && ScorePress4){
    ScorePress4=false;
  }  
}
}

void setup() {
  // put your setup code here, to run once:
pinMode(startButton, INPUT_PULLUP);
pinMode(resetButton, INPUT_PULLUP);
pinMode(teamOnePlus, INPUT_PULLUP);
pinMode(teamOneMinus, INPUT_PULLUP);
pinMode(teamTwoPlus, INPUT_PULLUP);
pinMode(teamTwoMinus, INPUT_PULLUP);

Serial.begin(57600);
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



  // Establish MQTT Broker connection and subscribe to reset topic



}


void loop() {


if(function==1){
  if(!gameOn){
    senseBigButton();
    senseResetButton();
  }
  if(gameOn){
    senseResetButton();
  }
}else if(function==2){
    if (!gameOn){
      senseBigButton();
      senseResetButton();
    }
    if (gameOn){
      senseResetButton();
      Score();
    }
  }
  else if ( function == 2) {
    if (!gameOn){
      senseResetButton();
    }
    if (gameOn){
      senseBigButton();
      senseResetButton();
    }
}else if(function==3){    
    if (!gameOn){
      senseBigButton();
      senseResetButton();
    }
    if (gameOn)
    {

      senseBigButton();
      senseResetButton();
    }
  
}

  if (!client.connected()) {
    reconnect();
  }

  // check MQTT messages
  client.loop();
  // if reset: reset to top of game
  // if MQTT_ID: load new params
  
}
