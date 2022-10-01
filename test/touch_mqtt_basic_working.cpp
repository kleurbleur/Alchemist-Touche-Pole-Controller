#include <Arduino.h>
#include <RunningMedian.h>
#include <SPI.h>
#include <ETH.h>
#include <PubSubClient.h>

// SETTINGS
char hostname[] ="Conductive sensor 1+2";                     // the hostname for board
char sub_topic[] = "Centrepiece_in";                          // out en in topics are best to seperate
char pub_topic[] = "Centrepiece_out";                         // out en in topics are best to seperate
char startMessage[] = "Conductive sensor 1+2 is online";      // start message when coming online to the mqtt server
IPAddress server(192, 168, 178, 214);                         // ip address of the server
const int sample_amount = 3;                                  // the amount of samples to check the median
const int touch_treshold = 10;                                // the number below which touch will be recognised   
const int DEBUG = 1;                                          // debugging on serial 0 = no messages, 1 = touch messages, 2 = mqtt messsages, 3 = both


// CODE
RunningMedian samples = RunningMedian(sample_amount);
WiFiClient ethclient;
PubSubClient client(ethclient);

static bool eth_connected = false;

bool if_true_input_0 = false;
bool if_false_input_0 = false;
bool if_true_input_4 = false;
bool if_false_input_4 = false;

// the function to check the touch input
// LAST FUNCTION SHOULD END WITH true OTHERWISE LAYOUT BREAKS
bool median_touch(int gpio_pin, bool end_ln) {

  int x = touchRead(gpio_pin);   // read from the sensor:
  
  samples.add(x); // add the samples
  int median_val = samples.getMedian();

  if (DEBUG == 1 || DEBUG == 3){
    Serial.print("Pin: ");
    Serial.print(gpio_pin);
    Serial.print(" Value: ");
    Serial.print(touchRead(gpio_pin));  
    Serial.print(" Median: ");
    if (end_ln == true){
      Serial.println(median_val);  
    }
    else {
      Serial.print(median_val);  
      Serial.print(" - ");
    }
  }

  if (median_val <= touch_treshold){
    return true;
  }
  else {
    return false;
  }
}





// the ethernet function
void WiFiEvent(WiFiEvent_t event)
{
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      Serial.println("ETH Started");
      ETH.setHostname(hostname);
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

// Callback function for mqtt when receiving a message
void callback(char* sub_topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(sub_topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  String msg = (char*)payload;
// INCOMING TEST MESSAGES
  if(msg.indexOf("start") >= 0){
    Serial.println("found start");
    client.publish(pub_topic,"running");
  } 
  if(msg.indexOf("shutdown") >= 0){
    Serial.println("found shutdown");
    client.publish(pub_topic,"halting");
  } 
}

// Reconnect function for mqtt
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(hostname)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(sub_topic, startMessage);
      // ... and resubscribe
      client.subscribe(sub_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}




void setup() {
  Serial.begin(115200);
  Serial.println("Conductive MQTT Sensor");

  WiFi.onEvent(WiFiEvent);
  ETH.begin();
  if (eth_connected){
      delay(5000); // wait for all the network things to resolve
      Serial.println("Ethernet Connected");
  }
  client.setServer(server, 1883);
  client.setCallback(callback);

  // Allow the hardware to sort itself out
  delay(1500);
}





void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();  

  bool touch_0 = median_touch(33, false);
  bool touch_4 = median_touch(32, true);


  if (touch_0 == true && if_true_input_0 == false){
    if_true_input_0 = true;
    if_false_input_0 = false;
    Serial.println("TOUCH on 3");
    client.publish(pub_topic, "sensor 3 active");
  } else if (touch_0 == false && if_false_input_0 == false) {
    if_false_input_0 = true;
    if_true_input_0 = false;
    Serial.println("no touch on 3");
    client.publish(pub_topic, "sensor 3 not active");
  }
  if (touch_4 == true && if_true_input_4 == false){
    if_true_input_4 = true;
    if_false_input_4 = false;    
    Serial.println("TOUCH on 4");
    client.publish(pub_topic, "sensor 4 active");
  } else if (touch_4 == false && if_false_input_4 == false) {
    if_false_input_4 = true;
    if_true_input_4 = false;    
    Serial.println("no touch on 4");
    client.publish(pub_topic, "sensor 4 not active");
  }

  delay(150);
}