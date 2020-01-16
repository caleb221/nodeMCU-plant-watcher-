/*===============================================
 * This is the base code for a NodeMCU 
 *  standalone IoT device!
 * If you want to be able to water a plant 
 * from your phone/computer/any connected device, here is the software!
 * --> in addition to watering your plant, you are also able to see:
 *     Temperature
 *     Humiditity
 *     the % of water in the plant's soil
 *     the amount of light near the module
 *     The % of battery left on 
 *     --> In order to conserve energy, the node will automatically shut down at 
             11pm (indoChina time) and restart at 7am
 *     --> ALSO there is code for a 5V water storage detector commented out
 * The NodeMCU is using the websocket protocol for bi-directional communication
 *     between you (your computer/phone) and the microcontroller
 *     the data is packaged as JSON, should you wish to keep a log its already
 *     in a usable format.
 * The circuit should be pretty easy to build according to the code.
    --> please see schematics for more details.
 * the small 5V DC water pump is connected to pin D7
 * --> please note the water pump is connected to a TIP120 transistor
 *     and 2 external batteries are used to power the device,
 *     but given a larger battery it should be fine
 * the NodeMCU is given 5V through a buck voltage regulator
 *****************************************  
 *  Special thanks to all those helpful
 *  people on the internet who found
 *  solutions to all my problems
 ****************************************  
 * -Caleb Seifert
 *=============================================== 
 */



#include<WebSocketsServer.h>
#include<ESP8266WiFi.h>
#include<ESP8266WebServer.h>
#include<WiFiClient.h>
#include<ArduinoJson.h>
#include<DHT.h>
#include<Adafruit_Sensor.h>
#include<time.h>
#include<Adafruit_ADS1015.h>

//=====================
//  PINS

#define dhtPin D3 //5v
#define dhtType DHT22 
#define feederPin D5 //motorPin  5//5v

//===========================
//      socket stuff
WebSocketsServer socket= WebSocketsServer(88);//port: 666
ESP8266WebServer server(80);
String jsonData;
StaticJsonDocument<200> root;
// SocketVariables are sent to client every 50 milliseconds

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
//      Server stuff
const char* ssid="WIFI_SSID";
const char* passw="WIFI_PASSWORD";
String html,javaScript,css;
//===========================
//      time location stuff
int tzone = 7 *3600;//7 for indochina
int dst =0;//date swing time ??


void setup() {

  dht.begin();
  exAnalog.setGain(GAIN_ONE);// 1x gain  DO NOT EXCEED: +/- 4.096V  1 bit = 0.125mV
  exAnalog.begin();
  
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid,passw);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("");
  Serial.println(WiFi.localIP());
  Serial.println("setup...WHERE THE DATA?");
  
  //pinMode(foodPin,INPUT);
  pinMode(feederPin,OUTPUT);
  server.on("/",makeWebsite);
  socket.begin();
  socket.onEvent(socketFunction);
  server.onNotFound(handleNotFound);
  server.begin();
  digitalWrite(feederPin,LOW);
  configTime(tzone,dst,"pool.ntp.org","time.nist.gov");

  while(!time(nullptr))
  {
    Serial.print(">");
    delay(100);   
  }

}

void loop() {


 time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);
  int hour =p_tm->tm_hour;
  //Serial.println(hour);
  /*
  // IN TESTING
  if(hour > 21 || hour < 7)
  {
    //sleep for 3 hours or so
    goSleep();  
  }

*/

socket.loop();
server.handleClient();

jsonData=makeJson();
socket.broadcastTXT(jsonData);

}


void socketFunction(uint8_t num, WStype_t type, uint8_t * payload, size_t plength)
{
    String payloadString=(const char *)payload;
  if(type == WStype_TEXT)
  {
     //call watering function here
     //Serial.print(payloadString);
      if(payloadString == "GO")
      {
        waterPlant(true); 
      }
      else
      {
       waterPlant(false); 
      }
  }
}
void goSleep()
{
      uint32_t sleepTime = ESP.deepSleepMax();
      ESP.deepSleep(sleepTime); //sleep for about 3-3.5 hours 
}
 void waterPlant(boolean fd)
 {
  if(fd)
  {
    digitalWrite(feederPin,HIGH);
   // Serial.println("WATERING....\n");
    delay(5000);
  }
  else
  {
    digitalWrite(feederPin,LOW);
    //Serial.println("NOT WATERING....");
  }
 }


void makeWebsite()
{
  makeJscript();
  makeStyle();
  html="<!DOCTYPE html>";
  html+="<head>";
  html+="<meta name='viewport' content='width=device-width, initial-scale=1' /> ";
  html+="<meta charset='utf-8'>";
  html+="<style>";
  html+=css;
  html+="</style>";
  html+="</head>";
  html+="<body onload='javascript:start()'>";
  html+="<h1>Dat Node 001</h1>";
  html+="<div class='leaf'> ";
  html+="<div class='dataFlow' id = 'temp'>";
  html+="Temp</div></div>";
  html+="<p class='dataLabel'>";
  html+="Temperature</p>";
  html+="<br><br>";
  html+="<div class='leaf'>";
  html+="<div class='dataFlow' id='humid'>100%</div>";
  html+=" </div>";
  html+="<p class='dataLabel'>Humidity</p>";
  html+="<br><br>";
  html+="<div class='leaf'>";
  html+="<div class='dataFlow' id='waterLev'>";
  html+="100%</div> </div>";
  html+="<p class='dataLabel'>Soil Water (%)</p>";
  html+="<br><br>";

//============================================================
  
  html+="<div class='leaf'>";
  html+="<div class='dataFlow' id='waterLev2'>";
  html+="100%</div> </div>";
  html+="<p class='dataLabel'>Plant #2 Soil Water (%)</p>";
  html+="<br><br>";
  
  html+="<div class='leaf'>";
  html+="<div class='dataFlow' id='lightLev'>";
  html+="100%</div> </div>";
  html+="<p class='dataLabel'>Light (%)</p>";
  html+="<br><br>";



  html+="<div class='leaf'>";
  html+="<div class='dataFlow' id='batteryLev'>";
  html+="100%</div> </div>";
  html+="<p class='dataLabel'>Battery (%)</p>";
  html+="<br><br>";

//============================================================

 /*
  html+="<div class='leaf'>";
  html+="<div class='dataFlow' id='storage'>";
  html+="Full</div> </div>";
  html+="<p class='dataLabel'>Food Storage</p>";
  html+="<br>";
  */
  html+="<input class='leaf' type='button' id='waterPlant' onclick='waterPlant()' value='Water!' >";
  html+="<br><br>";
  html+=" <input  class ='leaf' type='button' onclick='location.href=location.href' value='Refresh!'>";
  html+="<br> <br> <br> <br> <br> <br> <br>";
  html+="<script>";
  html+=javaScript;
  html+="</script>";
  html+="</body>";
  html+="</html>";
  server.send(200,"text/html",html);
}

void makeJscript()
{
  javaScript+="var wBtn = document.querySelector('#waterPlant');\n";
  javaScript+="var socket;";
  javaScript+="function start(){\n var loc = window.location.hostname;";
  javaScript+="socket = new WebSocket('ws://'+loc+':88/');\n "; 
  javaScript+="console.log(socket);\n";
  javaScript+="socket.onmessage = function(evt){\n";  
  javaScript+="var data = JSON.parse(evt.data);\n";
  javaScript+="//console.log(data);\n";
  javaScript+="updateApp(data);\n";
  javaScript+="};";
  javaScript+="}\n";
  javaScript+="function updateApp(data){\n";
  javaScript+="var humid=document.querySelector('#humid'); \n";
  javaScript+="var waterLev =document.querySelector('#waterLev');\n";
  javaScript+="var temp=document.querySelector('#temp');\n";
 
  javaScript+="var water2=document.querySelector('#waterLev2');\n";
  javaScript+="var battery=document.querySelector('#batteryLev');\n";
  javaScript+="var light=document.querySelector('#lightLev');\n";
 


 
 
//ADD MORE STUFF HERE

  
  javaScript+="temp.innerHTML=data.temp;\n";
  javaScript+="humid.innerHTML=data.humidity;\n";
  javaScript+="waterLev.innerHTML=data.soilWater/10;\n";
  
  javaScript+="water2.innerHTML=data.soilWater2/10;\n";
  javaScript+="light.innerHTML=data.lightLevel/10;\n";
  javaScript+="battery.innerHTML=data.batteryLevel/10;\n";
 //POST TO HTML HERE
 
 /*
  //javaScript+="var foodStore=document.querySelector('#storage');\n";
  javaScript+="if(data.waterSupply == 0) { \n";  
  javaScript+="foodStore.innerHTML='没有';\n";
  javaScript+="} else { \n";    
  javaScript+="foodStore.innerHTML='有'; } \n";
  */
  javaScript+="}\n";
  javaScript+="function waterPlant() { \n";
  javaScript+="var delayTime = 1000*5;\n";
  javaScript+="var goingOut='GO';\n";
  javaScript+="socket.send(goingOut);\n";
  javaScript+="wBtn.disabled=true;\n";
  javaScript+="setInterval(stopTimer,delayTime);\n";
  javaScript+="goingOut='STOP';\n";
  javaScript+="socket.send(goingOut);\n";
  javaScript+="}\n";
  javaScript+="function stopTimer() { \n";
  javaScript+="clearInterval(50);\n";
  javaScript+="wBtn.disabled=false; \n";
  javaScript+="}\n";
}
void makeStyle()
{  
  css="@media screen and (max-width: 1020px) {";
  css+="#container, #header, #content, #footer{";
  css+="float: none; width: auto; }";
  css+=" body{";
  css+="background-image: linear-gradient(powderblue,darkgreen);";
  css+="font-family: monospace;";
  css+="color:cornsilk;";
  css+="background-color: darkgreen;";
  css+="background-repeat: no-repeat;";
  css+=" }";
  css+="h1{ color: black; }";
  css+=".leaf {";
  css+="float: left; width: 95px; height: 70px; background-color: #A0DE21; ";
  css+=" -moz-border-radius: 100px 0px; -webkit-border-radius: 100px 0px;";
  css+=" border-radius: 100px 0px; border-image-slice: 5; text-align: center;";
  css+=" text-indent: inherit; }";
  css+=".dataFlow {";
  css+="  margin: 20px; color:black; font-size: 16px"; 
  css+=" }";
  css+=".dataLabel { padding-left:80px; padding-top: 15px; }";
  css+=" }";

 
}

 //I WANNA MAKE A JSON FILE...SEND IT TO THE PISERVER FOR DATA COLLECTION
 //ANALYSIS/LOGGING
void checkSensors()
{
  
  temp=dht.readTemperature();
  humid=dht.readHumidity();
  
  //isFood = digitalRead(foodPin);
  adc0=exAnalog.readADC_SingleEnded(0);//light
  adc1=exAnalog.readADC_SingleEnded(1);//hygrometer sensor
  adc2=exAnalog.readADC_SingleEnded(2);//extra (soil, whatever)
  adc3=exAnalog.readADC_SingleEnded(3);//voltage sensor (battery power)


if(isnan(temp) || isnan(humid))
{
  Serial.println("DHT READ FAIL...");
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
  
    
  exLight= map(adc0,26000,1000,0,1000);//divide by 10 on client side for float accuracy
  
  exHydro= map(adc1,26300,11300,0,1000);// divide by 10 on client side for float accuracy
  
  hydroLevel=map(adc2,26300,11300,0,1000);//,1024,350,0,100);
  
  exBat= map(adc3,4000,29700,0,1000);
//exBat= map(adc3,4000,21700,0,1000);
     //Serial.print("ADC1:");Serial.println(adc1);
   //Serial.print("ADC2:");Serial.println(adc2);


  /*
   Serial.print("ADC1:");Serial.println(adc1);
  Serial.print("HYDRO 2:");
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

String makeJson()
{
  checkSensors();
  String buff;
   
  
   root["temp"]=temp;
   root["humidity"]=humid;
   root["soilWater"]=hydroLevel;
   root["soilWater2"]=exHydro;
   root["lightLevel"]=exLight;
   root["batteryLevel"]=exBat;
   serializeJson(root,buff);
   root.clear();
   return buff;  
}


void handleNotFound() {  
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}
