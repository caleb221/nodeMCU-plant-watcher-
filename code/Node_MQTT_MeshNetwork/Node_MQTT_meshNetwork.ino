#include<ESP8266WiFi.h>
#include <PubSubClient.h>
#include<DHT.h>
#include<WiFiClient.h>

#include "lwip/lwip_napt.h"
#include "lwip/app/dhcpserver.h"

#include<ArduinoJson.h>
#include<Adafruit_Sensor.h>
#include<Adafruit_ADS1015.h>

//=====================
//     PINS
#define dhtPin D3 //5v
#define dhtType DHT22 
#define valvePin D5 //motorPin  5//5v
//=====================

//===========================
//      MQTT stuff

//===========================
int nodeId =2;//CHANGE THIS FOR EACH NODE!
//===========================

const char* ssid = "Steal_your_cookies";
const char* passw =  "password";
const char* mqtt_server = "192.168.3.254";

char* topicOut="nodePlant/data";
char* topicIn="nodePlant/commands";

// credentials for ESP8266 AP
const char *ap_ssid = "plantFi";
const char *ap_password = "password";




WiFiClient espClient;
PubSubClient client(espClient);


char* jsonData;
    int idIn,opValve,sleep = 0;
const int capacity = JSON_OBJECT_SIZE(4);
StaticJsonBuffer<capacity> jb;

//============================
//    Sensor stuff
Adafruit_ADS1115 exAnalog;
DHT dht(dhtPin,dhtType);
int hydroLevel=0;
float humid=0;
float temp =0;
//int isFood=0;
int16_t adc0,adc1,adc2,adc3;
int exHydro,exLight,exBat;
//===========================


void setup() {

  dht.begin();//start up the DHT
  exAnalog.setGain(GAIN_ONE);// 1x gain  DO NOT EXCEED: +/- 4.096V  1 bit = 0.125mV
  
  //start reading from ADC
  exAnalog.begin();
  
  //set up pin 5 to open valve
  pinMode(valvePin,OUTPUT);
  digitalWrite(valvePin,LOW);//make sure valve is closed...
  
  Serial.begin(115200);
  
  //===========================
  // CHANGE TO AP_STA for mesh network
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid,passw);
  //===========================

  // connect to wifi
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected to: ");
  Serial.println(ssid);
  Serial.println(WiFi.localIP());
  
  Serial.println("Configuring access point...");
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  Serial.println("");
  
  // Initialize the NAT feature
  ip_napt_init(IP_NAPT_MAX, IP_PORTMAP_MAX);

  // Enable NAT on the AP interface
  ip_napt_enable_no(1, 1);

  // Set the DNS server for clients of the AP to the one we also use for the STA interface
  dhcps_set_DNS(WiFi.dnsIP());
  
  //set up MQTT
  client.setServer(mqtt_server, 1883);//default port, change it...
  client.setCallback(callback);

}

void loop() {

//always check sensors before sending any data
jsonData=makeJson();

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  //add a timer here, no need to flood the network every few miliseconds
  client.publish(topicOut, jsonData);
  
  
}

void callback(char* topic, byte* payload, unsigned int length) {
  char input[length];
//===========================
  //ADD JSON PROCESSING HERE 
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) 
  {
      input[i]=(char)payload[i];
  }
    /* EXAMPLE WORKING INPUT
     "{"id":1,"sleep":1,"open":0}"
    */
       JsonObject& obj = jb.parseObject(input);
       idIn = obj["id"];
       opValve = obj["open"];
       sleep = obj["sleep"];
      //using the memory for 4 objects, can add another if needed
       
       jb.clear();//clear memory
  Serial.println();

  // if I got a message for the right node
  if(idIn == nodeId)
  {
    Serial.println("thats me! \nsleep: ");
    //1 for sleep 0 for no sleep
    if(sleep==1)
    {
      sleepTime(true);  
    }
    //same here open/close
    if(opValve==1)
    {
      openValve(true);  
    }    
  }
}

void sleepTime(bool goSleep)
{
  if(goSleep)
  {
      ESP.deepSleep(10e6); //set for 10 seconds, lets make it 2.5hours after testing  
      //it will restart automatically after the timer is up
      //45 minutes: 2.7E+09
      //2.5 hours: 9.0000E+9
      //or set it dynamically
  }
}

//ADD TIMER (CANT USE DELAY), or base it off hygrometer sensor...
 void openValve(boolean fd)
 {
  if(fd)
  {
    digitalWrite(valvePin,HIGH);
   // Serial.println("WATERING....\n");
    delay(5000);
  }
  else
  {
    digitalWrite(valvePin,LOW);
    //Serial.println("NOT WATERING....");
  }
}


void checkSensors()
{
//GET ANALOG/DIGITAL INPUT, and do some conversions (need tuning)
  temp=dht.readTemperature();
  humid=dht.readHumidity();
  
  //isFood = digitalRead(foodPin);
  adc0=exAnalog.readADC_SingleEnded(0);//light
  adc1=exAnalog.readADC_SingleEnded(1);//hygrometer sensor
  adc2=exAnalog.readADC_SingleEnded(2);//extra (soil, whatever)
  adc3=exAnalog.readADC_SingleEnded(3);//voltage divider (battery power)
  
if(isnan(temp) || isnan(humid))
{
  //Serial.println("DHT READ FAIL...");
  //delay(1000);
  temp=404;
  humid=404;
  //checkSensors(); 
}
/*
  Serial.println("temp");
  Serial.print(temp);
  Serial.println("\nHumid:");
  Serial.print(humid);
  Serial.println("\nWater:");
*/
  exLight= map(adc0,26000,1000,0,1000);// fix this for more accuracy
  
  exHydro= map(adc1,26300,11300,0,1000);// fix this for more accuracy
  
  hydroLevel=map(adc2,26300,11300,0,1000);//,1024,350,0,100);//values for 10bit accuracy
  
  exBat= map(adc3,4000,21700,0,1000);
  //Serial.print("ADC1:");Serial.println(adc1);
 //Serial.print("ADC2:");Serial.println(adc2);


  /*
   Serial.print("ADC1:");Serial.println(adc1);
  Serial.print("HYDRO 1:");
  Serial.print(exHydro);
  Serial.print("\n");
  
  Serial.print("LIGHT:");
  Serial.print(exLight);
  Serial.print("\n");
  
  Serial.print("BATT:");
  Serial.print(exBat);
  Serial.print("\n");
  //3600 is 0 volts applied
  //21700 is 100%
  
  //Serial.print(hydroLevel);
  //Serial.print("\n");
  */
  delay(50);
 }

char* makeJson()
{
  checkSensors();
  char buff[200];
  DynamicJsonBuffer jBuffer;
  JsonObject& root = jBuffer.createObject();
   
   root["Id"]=nodeId;
   root["temp"]=temp;
   root["humidity"]=humid;
   root["soilWater"]=hydroLevel;
   root["lightLevel"]=exLight;
   root["batteryLevel"]=exBat;

// root["soilWater2"]=exHydro;

   /* EX output
    * {"Id":49,
    * "temp":404,
    * "humidity":404,
    * "soilWater":1753,
    * "lightLevel":1040,
    * "batteryLevel":-226}
    */
   root.printTo(buff);
   //jBuffer.clear();// no need for copies....
   return buff;  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    
    String clientId = ": "+nodeId;    
    // Attempt to connect
    if (client.connect(clientId.c_str())) 
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(topicOut, "hello");
      // ... and resubscribe
      client.subscribe(topicIn);
    } 
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
