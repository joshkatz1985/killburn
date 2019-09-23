/* Kilburn Nerf Experience Button Code
   Contributors: Joe Mertz and Bob McGrath @ L3DFX in Bolingbrook, Illinois
        Joe Mertz www.github.com/jcmertz
        Bob McGrath www.github.com/bobwmcgrath
        L3DFX www.l3dfx.com
   Version 0.9
   Date: 7.30.19
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


//Which input pin is the start/stop button connected to?
const int bigButton = 5;
bool bigButtonLatch = false;
//Which input pin is the start/stop button LED connected to?
const int bigButtonLED = 21;
//Which input pin is the reset button connected to?
const int resetButton = 22;
bool resetButtonLatch = false;
//game on or off
bool gameOn = false;

//Button Function
int function = 0;
//<-------------------------------------------------------------------------------------------------------------------------------------------
// mac and MQTT_ID must be unique for each device in an activation

byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0x24};
char clientName[] = "clefable";

//<-------------------------------------------------------------------------------------------------------------------------------------------

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
    Serial.println(clientName);
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

void senseBigButton() {
  int bigButtonState = digitalRead(bigButton);
  if (!bigButtonState == LOW && !bigButtonLatch)
  {
    DEBUG_PRINTLN("Big Button Pressed");
    if (function == 1) {
      publishToTopic("timer/start", "Start Button");
      gameOn = true;
    }
    else if (function == 2) {
      publishToTopic("timer/end", "End Button");
      gameOn = false;
    }
    else if (function == 3) {
      if (gameOn) {
        publishToTopic("timer/end", "End Button");
        gameOn = false;
      }
      else {
        publishToTopic("timer/start", "Start Button");
        gameOn = true;
      }
    }
    bigButtonLatch=true;
  }else if(bigButtonState == HIGH && bigButtonLatch){
    bigButtonLatch=false;
  }
}


void senseResetButton() {
  int resetButtonState = digitalRead(resetButton);
  if (resetButtonState == LOW && !resetButtonLatch)
  {
    DEBUG_PRINTLN("Reset Button Pressed");
    publishToTopic("activation/reset", "RESET BUTTON");
    gameOn = false;
    resetButtonLatch=true;
  }if(resetButtonState == HIGH && resetButtonLatch ){
    resetButtonLatch=false;
  }
}



void setup() {
  //Configure IO
  pinMode(bigButtonLED, OUTPUT);
  pinMode(bigButton, INPUT_PULLUP);
  pinMode(resetButton, INPUT_PULLUP);
  digitalWrite(bigButton, HIGH);
  digitalWrite(resetButton, HIGH);

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

  // Load config from IP ADDRESS
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

int ledState = LOW;
long lastTime = 0;
int blinkDuration = 500;

void loop() {
  Serial.println("test");
  if (function == 1) {
    if (!gameOn)
    { digitalWrite(bigButtonLED, HIGH);
      senseBigButton();
      senseResetButton();
    }
    if (gameOn)
    { digitalWrite(bigButtonLED, LOW);
      senseResetButton();
    }
  }
  else if ( function == 2) {
    if (!gameOn)
    { digitalWrite(bigButtonLED, LOW);
      senseResetButton();
    }
    if (gameOn)
    { digitalWrite(bigButtonLED, HIGH);
      senseBigButton();
      senseResetButton();
    }
  }
  else if (function == 3) {
    if (!gameOn)
    { digitalWrite(bigButtonLED, HIGH);
      senseBigButton();
      senseResetButton();
    }
    if (gameOn)
    {
      if ( millis() > lastTime + blinkDuration) {
        ledState = !ledState;
        digitalWrite(bigButtonLED, ledState);
        lastTime = millis();
      }
      senseBigButton();
      senseResetButton();
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
  //
}
