#include <Arduino.h>
#include <DHT.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include  <FirebaseESP32.h>
#include "addons/RTDBHelper.h"
#include <time.h>
//Firebase Server
#define API_KEY ""
#define DATABASE_URL ""
#define Database_Secrets ""
//PING Server
#define PING "www.google.com"
//NTP Server
#define NTPserver1 "pool.ntp.org"
#define NTPserver2 "asia.pool.ntp.org"
#define NTPserver3 "oceania.pool.ntp.org"
//MQTT Server
#define MQTT_SERVER "test.mosquitto.org"
#define MQTT_PORT 1883
#define MQTT_Pump_TOPIC "GardenIoT/Pump"
#define MQTT_LED_TOPIC "GardenIoT/LED"
//Port GPIOs
#define DHTPIN 1 //Read DHT22 Sensor
#define LDR 3 //Read Light Sensor
#define Soil_Moisture 2 //Read Soild Sensor
#define Pumps 4 // Control Pump
#define Light 5 //Control Light
//DHT11 Variable
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);
float Humidity = 0; 
float Temperature = 0;
int Save_Temp = 25; //Limit Temperature Low
int Danger_Temp = 40; //Limit Temperature High
int Save_Humi = 0;
int Danger_Humi = 0;
boolean DHT_Err = false;
//Light Sensor Variable
int lumen = 0; //Store value from LDR
int DARK_LIGHT = 65; //Change Point
boolean LDR_Err = false;
//Soild Sensor Variable
int soilMoist = 0; //Store value from Moisure
int DRY_SOIL = 55; //Limit soilMoist Low
int WET_SOIL = 80; //Limit soilMoist High
boolean Soil_Err = false;
//Catch Error Sensor
boolean Err = false;
//Days
int Days = 0; 
unsigned long Time_Passed = 0;
const unsigned long A_Day_milis = 24*60*60*1000;//24 hours
const unsigned long A_Day_timestamp = 60 * 60 * 24;//24 hours
//Pump
unsigned long Times_Pumps=0;
const unsigned long Next_Pump = 43200000; //12 hours
unsigned long Still_Pumps = 60000; //Water in 1 minute
boolean PumpsStatus = false; //Current Status Pump
//Light
boolean LightStatus = false; //Current Status Light
//WIFI Variable
String sta_ssid ; 
String sta_password ;
String ap_ssid = "ESP32";
String ap_password = "123456789";
const long Network_TimeOut = 5000;// Wait 5 minutes to Connect Wifi
//Ping
const unsigned long time_delay_to_ping = 300000; // 5 minutes/ping
unsigned long Last_ping_time = 0;
boolean ping_flag = false; //Result Ping
//NTP
unsigned long timestamp;
const long  gmtOffset_sec = 7 * 60 * 60; // UTC +7
const int daylightOffset_sec = 0; //Daylight saving time
//Firebase Variable
FirebaseData firebaseData;
FirebaseJson json;
FirebaseJson Djson;
const String Child_Path[11] = {"Error/DHT11","Error/LDR","Error/Soil","Plant/Days","Sensor/DHT11/Humi","Sensor/DHT11/Temp","Sensor/Light","Sensor/Solid","Status/Led","Status/Pump","Status/MQTT"};
const String Parent_Path = "ESP32/";
String Destinate;
const int total_key = 17; //Total number of key in DataLogging
const unsigned long time_delay_send_datalogging = 180000; //3 minutes/Send
const unsigned long expired_data = 30 * 60 * 60 * 24; //30 days
unsigned long Last_datalogging_time = 0;
unsigned long Delay_Delete_Data = 0;
unsigned long Wait_Delete = 0;
//MQTT Variable
WiFiClient wifiClient;
PubSubClient client(wifiClient);
boolean MQTTStatus = false; //Status Connect Broker
//Local Server Variable
AsyncWebServer server(80); //Create a web server listening on port 80
AsyncWebSocket ws("/ws");//Create a path for web socket server
int Person = 0; // Number clients access local host
String messanger;
const String Local_Path[11] = {"StatusD","StatusL","StatusM","","humi","temp","ligh","mois","LED","Pump",""};
//Command from client
int Command_Pump = 0; // 0: Nothing, 1:ON, 2:OFF
int Command_Light = 0; // 0: Nothing, 1:ON, 2:OFF
boolean Ig_Pump = false;// Ignore Command_Pump
boolean Ig_Led = false;// Ignore Command_Light 
//flag
boolean sta_flag = false;
boolean first_sta = true;
//Sercurity
String http_username = "admin";
String http_password = "admin";
boolean sercurity_backend_key = false;
boolean tolerance_backend_key = false;
//Loop variable
int i;
// Store Recent Value
int Temp[11] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};//Array {DHT_Err,LDR_Err,Soil_Err,Days,Humidity,Temperature,lumen,soilMoist,LightStatus,PumpsStatus,MQTTStatus}
#pragma region File HTML Files
#pragma region main_html
const char main_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html> 
<head>
  <title>ESP Web Server</title>
  <style>
    *{
      padding: 0;
      margin: 0;
      box-sizing: border-box;
    }
    body{
      height: 100vh;
      background: linear-gradient(to right, #2193b0, #6dd5ed);
      overflow: hidden;
      display: flex;
      justify-content: center;
      align-items: center;
    }
    button{
    margin-bottom: 20px;
    background-color: azure;
    border: 0;
    width: 300px;
    padding: 15px 0 15px 0;
    border-radius: 100px;
    transition-duration: 0.3s;
    transition-property: color,background-color ;
    -webkit-box-shadow: 0px 11px 22px -9px #000000; 
    box-shadow: 0px 11px 22px -9px #000000;
    cursor: pointer;
    display: block;
    }
    button:hover{
    color: whitesmoke;
    background-color: black;
}
    .hide{
        display: none;
    }
    .gauge{
      width: 160px;
      height: 160px;
      /*background: cornflowerblue;*/
      position: relative;
    }
    .outer{
      width: 160px;
      height: 160px;
      border-radius: 50%;
      box-shadow: rgba(0, 0, 0, 0.16) 0px 3px 6px, rgba(0, 0, 0, 0.23) 0px 3px 6px;
      padding: 20px;
    }
    .inner{
      height: 120px;
      width: 120px;
      display: flex;
      align-items: center;
      justify-content: center;
      border-radius: 50%;
      box-shadow: rgba(50, 50, 93, 0.25) 0px 30px 60px -12px inset, rgba(0, 0, 0, 0.3) 0px 18px 36px -18px inset;
    }
    .number{
      display: flex;
      align-items: center;
      justify-content: center;
      padding-top: 70px;
      font-weight: 600px;
      color: #555;
    }
    circle{
      fill: none;
      stroke: url(#GradientColor);
      stroke-width: 20px;
      stroke-dasharray: 472; /*Exact circle*/
      stroke-dashoffset: 0;
    }
    svg{
      margin-top: 22px;
      position: absolute;
      top: 0;
      left: 0;
    }
    .Sensor_Value{
      display: flex;
      height: 200px;
    }
    .gauge{
      margin: 0 10px 0 10px;
    }
    .gauge h3{
      text-align: center;
    }
    .Bool{
      display: flex;
      justify-content: space-between;
    }
    .Error_Sensor{
      display: flex;
    }
  </style>
</head>
<body>
<div id="Container2">
  <div id="Wifi"> <!--Login Wifi-->
    <h1>Wifi</h1>
    <h3 id = "Info">%INFO%</h3>
    <form  id = "LogIn">
      <label for="ssid">Name Wifi: </label>
      <input type="text" id="ssid" name="ssid"><br><br>
      <label for="password">Password: </label>
      <input type="text" id="password" name="password"><br><br>
    </form>    
  <button id="Submit">Submit</button>
  <button id = "Disconnect" class="hide">Disconnect</button>
</div>  
<div class="Sensor_Value">
<!--Độ ẩm không khí-->
<div class="gauge">
  <h3>Humidity</h3>
  <div class="outer">
    <div class="inner">
      <div id="humi" class="number">
        %HUMIDITY%
      </div>
    </div>
  </div>
  <svg xmlns="http://www.w3.org/2000/svg" version="1.1" width="160px" height="160px">
    <defs>
       <linearGradient id="GradientColor">
          <stop offset="0%" stop-color="#e91e63">
          <stop offset="100%" stop-color="#673ab7">
       </linearGradient>
    </defs>
    <circle id="c_humi" cx="80" cy="80" r="70" stroke-linecap="round">
  </svg>
</div>
<!--Độ ẩm đất-->
<div class="gauge">
  <h3>Moisture</h3>
  <div class="outer">
    <div class="inner">
      <div class="number" id="mois">
        %MOISTURE%
      </div>
    </div>
  </div>
  <svg xmlns="http://www.w3.org/2000/svg" version="1.1" width="160px" height="160px">
    <defs>
       <linearGradient id="GradientColor">
          <stop offset="0%" stop-color="#e91e63">
          <stop offset="100%" stop-color="#673ab7">
       </linearGradient>
    </defs>
    <circle id="c_mois" cx="80" cy="80" r="70" stroke-linecap="round">
  </svg>
</div>
<!--Nhiệt độ-->
<div class="gauge">
  <h3>Temperature</h3>
  <div class="outer">
    <div class="inner">
      <div class="number" id="temp">
        %TEMPERATURE%
      </div>
    </div>
  </div>
  <svg xmlns="http://www.w3.org/2000/svg" version="1.1" width="160px" height="160px">
    <defs>
       <linearGradient id="GradientColor">
          <stop offset="0%" stop-color="#e91e63">
          <stop offset="100%" stop-color="#673ab7">
       </linearGradient>
    </defs>
    <circle id="c_temp" cx="80" cy="80" r="70" stroke-linecap="round">
  </svg>
</div>
<!--ÁNh sáng-->
<div class="gauge">
  <h3>Light</h3>
  <div class="outer">
    <div class="inner">
      <div class="number" id="ligh">
        %LIGHT%
      </div>
    </div>
  </div>
  <svg xmlns="http://www.w3.org/2000/svg" version="1.1" width="160px" height="160px">
    <defs>
       <linearGradient id="GradientColor">
          <stop offset="0%" stop-color="#e91e63">
          <stop offset="100%" stop-color="#673ab7">
       </linearGradient>
    </defs>
    <circle id="c_ligh" cx="80" cy="80" r="70" stroke-linecap="round">
  </svg>
</div>
</div>
<div class="Bool">
  <div class="All_Status">
    <h2>Status Sensor</h2>
    <div class="Error_Sensor">
      <h3>DHT11: </h3><h3 id="DHT11_Status">%dht11_S%</h3>
    </div>
    <div class="Error_Sensor">
      <h3>Soil Moisture: </h3><h3 id="Mois_Status">%mois_S%</h3>
    </div>
    <div class="Error_Sensor">
      <h3>Light Sensor: </h3><h3 id="Ligh_Status">%Ligh_S%</h3>
    </div>
  </div>
  <div class="Action">
    <h2>Do</h2>
    <button id="Pump">Pump: %Pump%</button>
    <button id="Led">Led: %LED%</button>
    <button id ="LogOut">Log Out</button>
  </div>
</div>
</div>
  <script>
    var gateway = `ws://${window.location.hostname}/ws`;
    var websocket;
    var ConnBut = 0;
    document.getElementById("LogOut").addEventListener("click",LogOut)
    window.addEventListener('load', onLoad);
    function initWebSocket() {
      console.log('Trying to open a WebSocket connection...');
      websocket = new WebSocket(gateway);
      websocket.onopen    = onOpen;
      websocket.onclose   = onClose;
      websocket.onmessage = onMessage; // <-- add this line
    }
    function onOpen(event) {
      console.log('Connection opened');
    }
    function onClose(event) {
      console.log('Connection closed');
      setTimeout(initWebSocket, 2000);
    }
    function onMessage(event){
      console.log(String(event.data))
      for(var data of String(event.data).split("/"))
      {
        if(String(data).indexOf("humi") >= 0)
        {
          var value = Number(String(data).split(" ")[1]);
          Gauge2("humi","c_humi",value);
        }
        if(String(data).indexOf("temp") >= 0)
        {
          var value = Number(String(data).split(" ")[1]);
          Gauge2("temp","c_temp",value);
        }
        if(String(data).indexOf("mois") >= 0)
        {
          var value = Number(String(data).split(" ")[1]);
          Gauge1("mois","c_mois",value);
        }
        if(String(data).indexOf("ligh") >= 0)
        {
          var value = Number(String(data).split(" ")[1]);
          Gauge1("ligh","c_ligh",value);
        }
        if(String(data).indexOf("Wifi") >= 0)
        {
          if(String(data).indexOf("OFF") >= 0)
          {
            document.getElementById("Submit").classList.remove("hide");
            document.getElementById('LogIn').classList.remove("hide");
            ConnBut = 0;
            document.getElementById('Disconnect').classList.add("hide");
            document.getElementById('Info').innerHTML = "Status: Not connected";
          }
          if(String(data).indexOf("ON") >= 0)
          {
            if(ConnBut == 0)
            {
              document.getElementById("Submit").classList.add("hide");
              document.getElementById('LogIn').classList.add("hide");
            }
            document.getElementById('Info').innerHTML = "Status: Connected";
            document.getElementById('Disconnect').classList.remove("hide");
          }
        }
        if(String(data).indexOf("StatusL") >= 0)
        {
          if(String(data).indexOf("1") >= 0)
            document.getElementById("Ligh_Status").innerHTML = "Error";
          if(String(data).indexOf("0") >= 0)
            document.getElementById("Ligh_Status").innerHTML = "Ok";
        }
        if(String(data).indexOf("StatusD") >= 0)
        {
          if(String(data).indexOf("1") >= 0)
            document.getElementById("DHT11_Status").innerHTML = "Error";
          if(String(data).indexOf("0") >= 0)
            document.getElementById("DHT11_Status").innerHTML = "Ok";
        }
        if(String(data).indexOf("StatusM") >= 0)
        {
          if(String(data).indexOf("1") >= 0)
            document.getElementById("Mois_Status").innerHTML = "Error";
          if(String(data).indexOf("0") >= 0)
            document.getElementById("Mois_Status").innerHTML = "Ok";
        }
        if(String(data).indexOf("Pump") >= 0)
        {
          if(String(data).indexOf("1") >= 0)
            document.getElementById("Pump").innerHTML = "Pump: ON";
          if(String(data).indexOf("0") >= 0)
            document.getElementById("Pump").innerHTML = "Pump: OFF";
        }
        if(String(data).indexOf("LED") >= 0)
        {
          if(String(data).indexOf("1") >= 0)
            document.getElementById("Led").innerHTML = "Led: ON";
          if(String(data).indexOf("0") >= 0)
            document.getElementById("Led").innerHTML = "Led: OFF";
        }
      }
    }
    function onLoad(event) {
      initWebSocket();
      initButton();
    }
    function initButton() {
      document.getElementById("Submit").addEventListener("click",Log);
      document.getElementById("Disconnect").addEventListener("click",Dis);
      document.getElementById("Pump").addEventListener("click",ActiveP);
      document.getElementById("Led").addEventListener("click",ActiveL);
    }
    function ActiveL(){
      websocket.send("LED");
    }
    function ActiveP(){
      websocket.send("Pump");
    }
    function Dis(){
        websocket.send("Disconnect");
    }
    function Log(){
      ConnBut = 1;
      websocket.send("Username: " + document.getElementById('ssid').value);
      websocket.send("Password: " + document.getElementById('password').value);
      document.getElementById('ssid').value = "";
      document.getElementById('password').value = "";
      document.getElementById("Submit").classList.add("hide");
      document.getElementById('LogIn').classList.add("hide");
      document.getElementById('Info').innerHTML = "Waiting....";
    }
    function Gauge1(name1,name2,value){
      document.getElementById(name1).innerHTML = value+ "%";
      var temp = Math.floor(504 * (1 - (value/100)));
      document.getElementById(name2).style.strokeDashoffset = temp;
    }
    function Gauge2(name1,name2,value){
      document.getElementById(name1).innerHTML = value;
      var temp = Math.floor(504 * (1 - (value/100)));
      document.getElementById(name2).style.strokeDashoffset = temp;
    }
    function LogOut(){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/logout", true);
      xhr.send();
      setTimeout(function(){ window.open("/","_self"); }, 1000);
    }
  </script>
</body>
</html>
)rawliteral";
#pragma endregion main_html
#pragma region 404_Error
const char Error_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>404 NOT FOUND</title>
    <style>
@charset "UTF-8";
* {
  margin: 0;
  padding: 0;
  -webkit-text-size-adjust: none;
}
html, body {
  height: 100%;
  overflow: hidden;
}
body {
  padding: 0;
  margin: 0;
  background: #181828;
  font-size: 14px;
  line-height: 1;
}
label {
  cursor: pointer;
}
a {
  margin: 0;
  padding: 0;
  vertical-align: baseline;
  background: transparent;
  text-decoration: none;
  color: #000;
}
input, select, button, textarea {
  margin: 0;
  font-size: 100%;
}
html, div, span, applet, object, iframe, h1, h2, h3, h4, h5, h6, p, blockquote, pre,
a, abbr, acronym, address, big, cite, code, del, dfn, em, font, img, ins, kbd, q, s, samp,
small, strike, strong, sub, sup, tt, var, b, u, i, center, dl, dt, dd, ol, ul, li,
fieldset, form, label, legend, table, caption, tbody, tfoot, thead, tr, th, td, input {
  border: 0;
  outline: 0;
  font-size: 100%;
  vertical-align: baseline;
  background: transparent;
}
.top-header:before {
    background-image: url(https://1.bp.blogspot.com/-gxsOcYWghHA/Xp_izTh4sFI/AAAAAAAAU8s/y637Fwg99qAuzW9na_NT_uApny8Vce95gCEwYBhgL/s1600/header-footer-gradient-bg.png);
}
.top-header:before {
    content: '';
    display: block;
    width: 100%;
    height: 4px;
    background-repeat: repeat-x;
    background-size: contain;
    position: absolute;
    top: 0;
    left: 0;
  opacity:0.5;
}
.starsec{
  content: " ";
    position: absolute;
    width: 3px;
    height: 3px;
    background: transparent;
   box-shadow: 571px 173px #00BCD4, 1732px 143px #00BCD4, 1745px 454px #FF5722, 234px 784px #00BCD4, 1793px 1123px #FF9800, 1076px 504px #03A9F4, 633px 601px #FF5722, 350px 630px #FFEB3B, 1164px 782px #00BCD4, 76px 690px #3F51B5, 1825px 701px #CDDC39, 1646px 578px #FFEB3B, 544px 293px #2196F3, 445px 1061px #673AB7, 928px 47px #00BCD4, 168px 1410px #8BC34A, 777px 782px #9C27B0, 1235px 1941px #9C27B0, 104px 1690px #8BC34A, 1167px 1338px #E91E63, 345px 1652px #009688, 1682px 1196px #F44336, 1995px 494px #8BC34A, 428px 798px #FF5722, 340px 1623px #F44336, 605px 349px #9C27B0, 1339px 1344px #673AB7, 1102px 1745px #3F51B5, 1592px 1676px #2196F3, 419px 1024px #FF9800, 630px 1033px #4CAF50, 1995px 1644px #00BCD4, 1092px 712px #9C27B0, 1355px 606px #F44336, 622px 1881px #CDDC39, 1481px 621px #9E9E9E, 19px 1348px #8BC34A, 864px 1780px #E91E63, 442px 1136px #2196F3, 67px 712px #FF5722, 89px 1406px #F44336, 275px 321px #009688, 592px 630px #E91E63, 1012px 1690px #9C27B0, 1749px 23px #673AB7, 94px 1542px #FFEB3B, 1201px 1657px #3F51B5, 1505px 692px #2196F3, 1799px 601px #03A9F4, 656px 811px #00BCD4, 701px 597px #00BCD4, 1202px 46px #FF5722, 890px 569px #FF5722, 1613px 813px #2196F3, 223px 252px #FF9800, 983px 1093px #F44336, 726px 1029px #FFC107, 1764px 778px #CDDC39, 622px 1643px #F44336, 174px 1559px #673AB7, 212px 517px #00BCD4, 340px 505px #FFF, 1700px 39px #FFF, 1768px 516px #F44336, 849px 391px #FF9800, 228px 1824px #FFF, 1119px 1680px #FFC107, 812px 1480px #3F51B5, 1438px 1585px #CDDC39, 137px 1397px #FFF, 1080px 456px #673AB7, 1208px 1437px #03A9F4, 857px 281px #F44336, 1254px 1306px #CDDC39, 987px 990px #4CAF50, 1655px 911px #00BCD4, 1102px 1216px #FF5722, 1807px 1044px #FFF, 660px 435px #03A9F4, 299px 678px #4CAF50, 1193px 115px #FF9800, 918px 290px #CDDC39, 1447px 1422px #FFEB3B, 91px 1273px #9C27B0, 108px 223px #FFEB3B, 146px 754px #00BCD4, 461px 1446px #FF5722, 1004px 391px #673AB7, 1529px 516px #F44336, 1206px 845px #CDDC39, 347px 583px #009688, 1102px 1332px #F44336, 709px 1756px #00BCD4, 1972px 248px #FFF, 1669px 1344px #FF5722, 1132px 406px #F44336, 320px 1076px #CDDC39, 126px 943px #FFEB3B, 263px 604px #FF5722, 1546px 692px #F44336;
  animation: animStar 150s linear infinite;
}
.starthird
{
  content: " ";
    position: absolute;
    width: 3px;
    height: 3px;
    background: transparent;
   box-shadow: 571px 173px #00BCD4, 1732px 143px #00BCD4, 1745px 454px #FF5722, 234px 784px #00BCD4, 1793px 1123px #FF9800, 1076px 504px #03A9F4, 633px 601px #FF5722, 350px 630px #FFEB3B, 1164px 782px #00BCD4, 76px 690px #3F51B5, 1825px 701px #CDDC39, 1646px 578px #FFEB3B, 544px 293px #2196F3, 445px 1061px #673AB7, 928px 47px #00BCD4, 168px 1410px #8BC34A, 777px 782px #9C27B0, 1235px 1941px #9C27B0, 104px 1690px #8BC34A, 1167px 1338px #E91E63, 345px 1652px #009688, 1682px 1196px #F44336, 1995px 494px #8BC34A, 428px 798px #FF5722, 340px 1623px #F44336, 605px 349px #9C27B0, 1339px 1344px #673AB7, 1102px 1745px #3F51B5, 1592px 1676px #2196F3, 419px 1024px #FF9800, 630px 1033px #4CAF50, 1995px 1644px #00BCD4, 1092px 712px #9C27B0, 1355px 606px #F44336, 622px 1881px #CDDC39, 1481px 621px #9E9E9E, 19px 1348px #8BC34A, 864px 1780px #E91E63, 442px 1136px #2196F3, 67px 712px #FF5722, 89px 1406px #F44336, 275px 321px #009688, 592px 630px #E91E63, 1012px 1690px #9C27B0, 1749px 23px #673AB7, 94px 1542px #FFEB3B, 1201px 1657px #3F51B5, 1505px 692px #2196F3, 1799px 601px #03A9F4, 656px 811px #00BCD4, 701px 597px #00BCD4, 1202px 46px #FF5722, 890px 569px #FF5722, 1613px 813px #2196F3, 223px 252px #FF9800, 983px 1093px #F44336, 726px 1029px #FFC107, 1764px 778px #CDDC39, 622px 1643px #F44336, 174px 1559px #673AB7, 212px 517px #00BCD4, 340px 505px #FFF, 1700px 39px #FFF, 1768px 516px #F44336, 849px 391px #FF9800, 228px 1824px #FFF, 1119px 1680px #FFC107, 812px 1480px #3F51B5, 1438px 1585px #CDDC39, 137px 1397px #FFF, 1080px 456px #673AB7, 1208px 1437px #03A9F4, 857px 281px #F44336, 1254px 1306px #CDDC39, 987px 990px #4CAF50, 1655px 911px #00BCD4, 1102px 1216px #FF5722, 1807px 1044px #FFF, 660px 435px #03A9F4, 299px 678px #4CAF50, 1193px 115px #FF9800, 918px 290px #CDDC39, 1447px 1422px #FFEB3B, 91px 1273px #9C27B0, 108px 223px #FFEB3B, 146px 754px #00BCD4, 461px 1446px #FF5722, 1004px 391px #673AB7, 1529px 516px #F44336, 1206px 845px #CDDC39, 347px 583px #009688, 1102px 1332px #F44336, 709px 1756px #00BCD4, 1972px 248px #FFF, 1669px 1344px #FF5722, 1132px 406px #F44336, 320px 1076px #CDDC39, 126px 943px #FFEB3B, 263px 604px #FF5722, 1546px 692px #F44336;
  animation: animStar 10s linear infinite;
}
.starfourth
{
  content: " ";
    position: absolute;
    width: 2px;
    height: 2px;
    background: transparent;
   box-shadow: 571px 173px #00BCD4, 1732px 143px #00BCD4, 1745px 454px #FF5722, 234px 784px #00BCD4, 1793px 1123px #FF9800, 1076px 504px #03A9F4, 633px 601px #FF5722, 350px 630px #FFEB3B, 1164px 782px #00BCD4, 76px 690px #3F51B5, 1825px 701px #CDDC39, 1646px 578px #FFEB3B, 544px 293px #2196F3, 445px 1061px #673AB7, 928px 47px #00BCD4, 168px 1410px #8BC34A, 777px 782px #9C27B0, 1235px 1941px #9C27B0, 104px 1690px #8BC34A, 1167px 1338px #E91E63, 345px 1652px #009688, 1682px 1196px #F44336, 1995px 494px #8BC34A, 428px 798px #FF5722, 340px 1623px #F44336, 605px 349px #9C27B0, 1339px 1344px #673AB7, 1102px 1745px #3F51B5, 1592px 1676px #2196F3, 419px 1024px #FF9800, 630px 1033px #4CAF50, 1995px 1644px #00BCD4, 1092px 712px #9C27B0, 1355px 606px #F44336, 622px 1881px #CDDC39, 1481px 621px #9E9E9E, 19px 1348px #8BC34A, 864px 1780px #E91E63, 442px 1136px #2196F3, 67px 712px #FF5722, 89px 1406px #F44336, 275px 321px #009688, 592px 630px #E91E63, 1012px 1690px #9C27B0, 1749px 23px #673AB7, 94px 1542px #FFEB3B, 1201px 1657px #3F51B5, 1505px 692px #2196F3, 1799px 601px #03A9F4, 656px 811px #00BCD4, 701px 597px #00BCD4, 1202px 46px #FF5722, 890px 569px #FF5722, 1613px 813px #2196F3, 223px 252px #FF9800, 983px 1093px #F44336, 726px 1029px #FFC107, 1764px 778px #CDDC39, 622px 1643px #F44336, 174px 1559px #673AB7, 212px 517px #00BCD4, 340px 505px #FFF, 1700px 39px #FFF, 1768px 516px #F44336, 849px 391px #FF9800, 228px 1824px #FFF, 1119px 1680px #FFC107, 812px 1480px #3F51B5, 1438px 1585px #CDDC39, 137px 1397px #FFF, 1080px 456px #673AB7, 1208px 1437px #03A9F4, 857px 281px #F44336, 1254px 1306px #CDDC39, 987px 990px #4CAF50, 1655px 911px #00BCD4, 1102px 1216px #FF5722, 1807px 1044px #FFF, 660px 435px #03A9F4, 299px 678px #4CAF50, 1193px 115px #FF9800, 918px 290px #CDDC39, 1447px 1422px #FFEB3B, 91px 1273px #9C27B0, 108px 223px #FFEB3B, 146px 754px #00BCD4, 461px 1446px #FF5722, 1004px 391px #673AB7, 1529px 516px #F44336, 1206px 845px #CDDC39, 347px 583px #009688, 1102px 1332px #F44336, 709px 1756px #00BCD4, 1972px 248px #FFF, 1669px 1344px #FF5722, 1132px 406px #F44336, 320px 1076px #CDDC39, 126px 943px #FFEB3B, 263px 604px #FF5722, 1546px 692px #F44336;
  animation: animStar 50s linear infinite;
}
.starfifth
{
  content: " ";
    position: absolute;
    width: 1px;
    height: 1px;
    background: transparent;
   box-shadow: 571px 173px #00BCD4, 1732px 143px #00BCD4, 1745px 454px #FF5722, 234px 784px #00BCD4, 1793px 1123px #FF9800, 1076px 504px #03A9F4, 633px 601px #FF5722, 350px 630px #FFEB3B, 1164px 782px #00BCD4, 76px 690px #3F51B5, 1825px 701px #CDDC39, 1646px 578px #FFEB3B, 544px 293px #2196F3, 445px 1061px #673AB7, 928px 47px #00BCD4, 168px 1410px #8BC34A, 777px 782px #9C27B0, 1235px 1941px #9C27B0, 104px 1690px #8BC34A, 1167px 1338px #E91E63, 345px 1652px #009688, 1682px 1196px #F44336, 1995px 494px #8BC34A, 428px 798px #FF5722, 340px 1623px #F44336, 605px 349px #9C27B0, 1339px 1344px #673AB7, 1102px 1745px #3F51B5, 1592px 1676px #2196F3, 419px 1024px #FF9800, 630px 1033px #4CAF50, 1995px 1644px #00BCD4, 1092px 712px #9C27B0, 1355px 606px #F44336, 622px 1881px #CDDC39, 1481px 621px #9E9E9E, 19px 1348px #8BC34A, 864px 1780px #E91E63, 442px 1136px #2196F3, 67px 712px #FF5722, 89px 1406px #F44336, 275px 321px #009688, 592px 630px #E91E63, 1012px 1690px #9C27B0, 1749px 23px #673AB7, 94px 1542px #FFEB3B, 1201px 1657px #3F51B5, 1505px 692px #2196F3, 1799px 601px #03A9F4, 656px 811px #00BCD4, 701px 597px #00BCD4, 1202px 46px #FF5722, 890px 569px #FF5722, 1613px 813px #2196F3, 223px 252px #FF9800, 983px 1093px #F44336, 726px 1029px #FFC107, 1764px 778px #CDDC39, 622px 1643px #F44336, 174px 1559px #673AB7, 212px 517px #00BCD4, 340px 505px #FFF, 1700px 39px #FFF, 1768px 516px #F44336, 849px 391px #FF9800, 228px 1824px #FFF, 1119px 1680px #FFC107, 812px 1480px #3F51B5, 1438px 1585px #CDDC39, 137px 1397px #FFF, 1080px 456px #673AB7, 1208px 1437px #03A9F4, 857px 281px #F44336, 1254px 1306px #CDDC39, 987px 990px #4CAF50, 1655px 911px #00BCD4, 1102px 1216px #FF5722, 1807px 1044px #FFF, 660px 435px #03A9F4, 299px 678px #4CAF50, 1193px 115px #FF9800, 918px 290px #CDDC39, 1447px 1422px #FFEB3B, 91px 1273px #9C27B0, 108px 223px #FFEB3B, 146px 754px #00BCD4, 461px 1446px #FF5722, 1004px 391px #673AB7, 1529px 516px #F44336, 1206px 845px #CDDC39, 347px 583px #009688, 1102px 1332px #F44336, 709px 1756px #00BCD4, 1972px 248px #FFF, 1669px 1344px #FF5722, 1132px 406px #F44336, 320px 1076px #CDDC39, 126px 943px #FFEB3B, 263px 604px #FF5722, 1546px 692px #F44336;
  animation: animStar 80s linear infinite;
}
@keyframes animStar
{
  0% {
    transform: translateY(0px);
}
  100% {
    transform: translateY(-2000px);
}
}
button {
  border: none;
  padding: 0;
  font-size: 0;
  line-height: 0;
  background: none;
  cursor: pointer;
}
:focus {
  outline: 0;
}
.clearfix:before, .clearfix:after {
  content: "\0020";
  display: block;
  height: 0;
  visibility: hidden;
}
.clearfix:after {
  clear: both;
}
.clearfix {
  zoom: 1;
}
.error {
  min-height: 100vh;
  position: relative;
  padding: 240px 0;
  box-sizing: border-box;
  width: 100%;
  height: 100%;
  text-align: center;
  margin-top: 70px;
}
.error__overlay {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  overflow: hidden;
}
.error__content {
  position: absolute;
  top: 50%;
  left: 50%;
  width: 100%;
  -webkit-transform: translate(-50%, -50%);
          transform: translate(-50%, -50%);
}
.error__message {
  text-align: center;
  color: #181828;
}
.message__title {
  font-family: 'Montserrat', sans-serif;
  font-weight: 900;
  text-transform: uppercase;
  letter-spacing: 5px;
   font-size: 5.6rem;
  padding-bottom: 40px;
  max-width: 960px;
  margin: 0 auto;
}
.message__text {
  font-family: 'Montserrat', sans-serif;
  line-height: 42px;
  font-size: 18px;
  padding: 0 60px;
  max-width: 680px;
  margin: auto;
}
.error__nav {
  max-width: 600px;
  margin: 40px auto 0;
  text-align: center;
}
.e-nav__form {
  position: relative;
  height: 45px;
  overflow: hidden;
  width: 170px;
  display: inline-block;
  vertical-align: top;
  border: 1px solid #212121;
  padding-left: 10px;
  padding-right: 46px;
}
.e-nav__icon {
  position: absolute;
  right: 15px;
  top: 50%;
  -webkit-transform: translateY(-50%);
          transform: translateY(-50%);
  color: #212121;
  -webkit-transition: color .25s ease;
  transition: color .25s ease;
}
.e-nav__link {
  height: 45px;
  line-height: 45px;
  width: 170px;
  display: inline-block;
  vertical-align: top;
  margin: 0 15px;
  border: 1px solid #181828;
  color: #181828;
  text-decoration: none;
  font-family: 'Montserrat', sans-serif;
  text-transform: uppercase;
  font-size: 11px;
  letter-spacing: .1rem;
  position: relative;
  overflow: hidden;
}
.e-nav__link:before {
  content: '';
  height: 200px;
  background: #212121;
  position: absolute;
  top: 70px;
  right: 70px;
  width: 260px;
  -webkit-transition: all .3s;
  transition: all .3s;
  -webkit-transform: rotate(50deg);
          transform: rotate(50deg);
}
.e-nav__link:after {
  -webkit-transition: all .3s;
  transition: all .3s;
  z-index: 999;
  position: relative;
}
.e-nav__link:after {
  content: "Home Page";
}
.e-nav__link:hover:before {
  top: -60px;
  right: -50px;
}
.e-nav__link:hover {
  color: #fff;
}
.e-nav__link:nth-child(2):hover:after {
  color: #fff;
}
.error__social {
  position: absolute;
  top: 50%;
  -webkit-transform: translateY(-50%);
          transform: translateY(-50%);
  left: 20px;
  z-index: 10;
}
.e-social__list {
  margin: 0;
  padding: 0;
  list-style-type: none;
}
.e-social__icon {
  padding-bottom: 30px;
}
.e-social__icon:last-child {
  padding-bottom: 0;
}
.e-social__link {
  color: #fff;
  -webkit-transition: all .25s ease;
  transition: all .25s ease;
  display: block;
}
.e-social__link:hover {
  opacity: .7;
}
.lamp {
  position: absolute;
  left: 0px;
  right: 0px;
  top: 0px;
  margin: 0px auto;
  width: 300px;
  display: flex;
  flex-direction: column;
  align-items: center;
  transform-origin: center top;
  animation-timing-function: cubic-bezier(0.6, 0, 0.38, 1);
  animation: move 5.1s infinite;
}
@keyframes move {
  0% {
    transform: rotate(40deg);
  }
  50% {
    transform: rotate(-40deg);
  }
  100% {
    transform: rotate(40deg);
  }
}
.cable {
  width: 8px;
    height: 248px;
    background-image: linear-gradient(rgb(32 148 218 / 70%), rgb(193 65 25)), linear-gradient(rgba(0,0,0,0.7), rgba(0,0,0,0.7)), linear-gradient(rgba(0,0,0,0.7), rgba(0,0,0,0.7));
}
.cover {
  width: 200px;
  height: 80px;
  background: #0bd5e8;
  border-top-left-radius: 50%;
  border-top-right-radius: 50%;
  position: relative;
  z-index: 200;
}
.in-cover {
  width: 100%;
  max-width: 200px;
  height: 20px;
  border-radius: 100%;
  background: #08ffff;
  position: absolute;
  left: 0px;
  right: 0px;
  margin: 0px auto;
  bottom: -9px;
  z-index: 100;
}
.in-cover .bulb {
     width: 50px;
    height: 50px;
    background-color: #08fffa;
    border-radius: 50%;
    position: absolute;
    left: 0px;
    right: 0px;
    bottom: -20px;
    margin: 0px auto;
    -webkit-box-shadow: 0 0 15px 7px rgba(0,255,255,0.8), 0 0 40px 25px rgba(0,255,255,0.5), -75px 0 30px 15px rgba(0,255,255,0.2);
    box-shadow: 0 0 25px 7px rgb(127 255 255 / 80%), 0 0 64px 47px rgba(0,255,255,0.5), 0px 0 30px 15px rgba(0,255,255,0.2);
}
.light {
      width: 200px;
    height: 0px;
    border-bottom: 900px solid rgb(44 255 255 / 24%);
    border-left: 50px solid transparent;
    border-right: 50px solid transparent;
    position: absolute;
    left: 0px;
    right: 0px;
    top: 270px;
    margin: 0px auto;
    z-index: 1;
    border-radius: 90px 90px 0px 0px;
}
.error {
  overflow: hidden;
  max-height: 100vh;
}
@media (max-width: 1400px) { 
  .lamp {
    zoom: .5;
  }
  .error__content {
    top: 55%;
  }
  .message__title {
    font-size: 3.5rem;
  }
}
@media (max-width: 900px) {
  .message__title {
    font-size: 34px;
  }
  .error__content {
    top: 55%;
  }
  }
@media (max-width: 950px) {
  .lamp__wrap {
    max-height: 100vh;
    overflow: hidden;
    max-width: 100vw;
  }
  .error__social {
    bottom: 30px;
    top: auto;
    transform: none;
    width: 100%;
    position: fixed;
    left: 0;
  }
  .e-social__icon {
    display: inline-block;
    padding-right: 30px;
  }
  .e-social__icon:last-child {
    padding-right: 0;
  }
  .e-social__icon {
    padding-bottom: 0;
  }
}
@media (max-width: 750px) {
  body, html {
    max-height: 100vh;
  }
   .error__content {
    position: static;
    margin: 0 auto;
    transform: none;
    padding-top: 300px;
  }
  .error {
    padding-top: 0;
    padding-bottom: 100px;
    height: 100vh;
  }
  }
@media (max-width: 650px) {
  .message__title {
    font-size: 36px;
    padding-bottom: 20px;
  }
  .message__text {
    font-size: 16px;
    line-height: 2;
    padding-right: 20px;
    padding-left: 20px;
  }
  .lamp {
    zoom: .6;
  }
  .error__content {
    padding-top: 180px;
  }
  }
@media (max-width: 480px) {
  .message__title {
    font-size: 30px;
  }
  .message__text {
    padding-left: 10px;
    padding-right: 10px;
    font-size: 15px;
  }
  .error__nav {
    margin-top: 20px;
  }
}
    </style>
</head>
<body>
    <header class="top-header">
      </header>
      <!--dust particel-->
      <div>
        <div class="starsec"></div>
        <div class="starthird"></div>
        <div class="starfourth"></div>
        <div class="starfifth"></div>
      </div>
      <!--Dust particle end--->
      <div class="lamp__wrap">
        <div class="lamp">
          <div class="cable"></div>
          <div class="cover"></div>
          <div class="in-cover">
            <div class="bulb"></div>
          </div>
          <div class="light"></div>
        </div>
      </div>
      <!-- END Lamp -->
      <section class="error">
        <!-- Content -->
        <div class="error__content">
          <div class="error__message message">
            <h1 class="message__title">Page Not Found</h1>
            <p class="message__text">Oops! We seem to have lost that page. Our bad!</p>
          </div>
        </div>
        <!-- END Content -->
      </section>
</body>
</html>
)rawliteral";
#pragma endregion 404_Error
#pragma region Sercurity_html
const char Sercurity_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Sercurity</title>
  <style>
      *{
        padding: 0;
        margin: 0;
        box-sizing: border-box;
      }
      body{
        height: 100vh;
        width: 100vw;
        background: linear-gradient(to right, #2193b0, #6dd5ed);
        overflow: hidden;
        display: flex;
        justify-content: center;
        align-items: center;
      }
      .hide{
        display: none;
      }
      button{
        margin-bottom: 20px;
        background-color: azure;
        border: 0;
        width: 300px;
        padding: 15px 0 15px 0;
        border-radius: 100px;
        transition-duration: 0.3s;
        transition-property: color,background-color ;
        -webkit-box-shadow: 0px 11px 22px -9px #000000; 
        box-shadow: 0px 11px 22px -9px #000000;
        cursor: pointer;
        display: block;
      }
      button:hover{
        color: whitesmoke;
        background-color: black;
      }
  </style>
</head>
<body>
  <div class="container">
    <div id="Admin">
      <h1>Change Login User</h1>
      <form>
        <label for="ssid">User Name: </label>
        <input type="text" id="user_id" name="user"><br><br>
        <label for="password">User Password: </label>
        <input type="text" id="user_password" name="password"><br><br>
      </form>
      <button id="Change">Change User</button>
    </div>
  </div>
  <div id="WiFi" class="hide">
    <h1>Config</h1>
    <form>
      <label for="ssid">SSID: </label>
      <input type="text" id="wifi_id" name="user"><br><br>
      <label for="password">Password: </label>
      <input type="text" id="wifi_password" name="password"><br><br>
    </form>
    <button id="Configuring">Configuring WiFI</button>
  </div>
  <button id="switch">Switch</button>
</div>
  <script>
    document.getElementById("Change").addEventListener("click",Change)
    document.getElementById("Configuring").addEventListener("click",Change)
    document.getElementById("switch").addEventListener("click",SwitchMode)
    function Change(){
      var noti = "Hello";
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 204)
            noti = "Success"
        if (this.readyState == 4 && this.status == 400)
            noti = "Fail"
        if (this.readyState == 4 && this.status == 511)
            noti = "Connect again"
        };
      if (this.id=="Change")
        var DataSend ="ADMIN: " +document.getElementById("user_id").value + "/"+ document.getElementById("user_password").value + "/";
      if (this.id == "Configuring")
        var DataSend ="AP: " +document.getElementById("wifi_id").value + "/"+ document.getElementById("wifi_password").value + "/";
      console.log(DataSend)
      document.getElementById("user_id").value = ""
      document.getElementById("user_password").value = ""
      document.getElementById("wifi_id").value = ""
      document.getElementById("wifi_password").value = ""
      xhr.open("POST", "/BackEndSercure", false);
      xhr.send(DataSend);

      var win = window.open("","_blank","width=200,height=100",false);
      win.document.body.innerHTML = '<h1>'+noti+'</h1>';
      window.open(location.href, "_self", "");
      window.close();
    }
    function SwitchMode(){
      if(document.getElementById("Admin").classList.contains("hide"))
        document.getElementById("Admin").classList.remove("hide")
      else
        document.getElementById("Admin").classList.add("hide")
      if(document.getElementById("WiFi").classList.contains("hide"))
        document.getElementById("WiFi").classList.remove("hide")
      else
        document.getElementById("WiFi").classList.add("hide")
    }
  </script>
</body>
</html>
)rawliteral";
#pragma endregion Sercurity_html
#pragma region Parameters_html
const char Tolerance_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Tolerance ranges of plant</title>
    <style>
        *{
        padding: 0;
        margin: 0;
        box-sizing: border-box;
        
      }
      body{
        height: 100vh;
        background: linear-gradient(to right, #a8ff78, #78ffd6);
        overflow: hidden;
        display: flex;
        justify-content: center;
        align-items: center;
      }
      button{
        margin-bottom: 20px;
        background-color: azure;
        border: 0;
        width: 400px;
        padding: 15px 0 15px 0;
        border-radius: 100px;
        transition-duration: 0.3s;
        transition-property: color,background-color ;
        -webkit-box-shadow: 0px 11px 22px -9px #000000; 
        box-shadow: 0px 11px 22px -9px #000000;
        cursor: pointer;
        display: block;
        margin-inline: auto;
        font-size: 18px;
        font-weight: bolder;
      }
      button:hover{
        color: whitesmoke;
        background-color: black;
      }
      label{
        font-weight: bolder;
        font-size: 1.2em;
      }
      .interact{
        display: inline;
      }
      .container{
        margin: auto;
        max-width: 1000px;
        padding: 0 16px;
        justify-content: center;
        align-items: center;
      }
      .container h1{
        display: flex;
        justify-content: center;
      }
      .group{
        display: flex;
        justify-content:space-around;
      }
      .container2{
        width: 1000px;
      }

    </style>
</head>
<body>
    <div class="container">
        <h1>Change the limit</h1><br>
        <div class="container2">
            <div class="group">
                <div id="Tempature" class="interact">
                    <h2>Environment Temperature</h2>
                    <form>
                        <label for="ssid">The upper limit: </label>
                        <input type="number" id="upT" name="upT" value="%UPT%"><br><br>
                        <label for="ssid">The lower limit: </label>
                        <input type="number" id="lowT" name="lowT" value="%LOWT%" ><br><br>
                    </form>
                </div>
                <div id="Humidity" class="interact">
                    <h2>Environmental Humidity</h2>
                    <form>
                        <label for="ssid">The upper limit: </label>
                        <input type="number" id="upH" name="upH" value="%UPH%"><br><br>
                        <label for="ssid">The lower limit: </label>
                        <input type="number" id="lowH" name="lowH" value="%LOWH%" ><br><br>
                    </form>
                </div>
            </div>
            <div class="group">
                <div id="Soil_Moisture" class="interact">
                    <h2>Environmental Soil Moisture</h2>
                    <form>
                        <label for="ssid">The upper limit: </label>
                        <input type="number" id="upSM" name="upSM" value="%UPSM%" ><br><br>
                        <label for="ssid">The lower limit: </label>
                        <input type="number" id="lowSM" name="lowSM" value="%LOWSM%" ><br><br>
                    </form>
                </div>
                <div id="Soil_Moisture" class="interact">
                    <h2>Environmental brightness</h2>
                    <form>
                        <label for="ssid">The Glow Point: </label>
                        <input type="number" id="L" name="L" value="%L%"><br><br>
                    </form>
                </div>
            </div>
        </div>
        <button id="SubmitChange">Change</button>
    </div>
    <script>
        document.getElementById("SubmitChange").addEventListener('click',Send);
        function Send(){
          if(document.getElementById("upT").value <= document.getElementById("lowT").value || document.getElementById("upT").value > 80 || document.getElementById("lowT").value < -40
              || document.getElementById("upH").value <= document.getElementById("lowH").value || document.getElementById("upH").value > 100 || document.getElementById("lowH").value < 0
              || document.getElementById("upSM").value <= document.getElementById("lowSM").value || document.getElementById("upSM").value > 100 || document.getElementById("lowSM").value < 0 
              || document.getElementById("L").value < 0 || document.getElementById("L").value > 100)
                return false
            var xhr = new XMLHttpRequest();
            var DataSend = "/"+document.getElementById("upT").value +"/"+ document.getElementById("lowT").value +
                           "/"+document.getElementById("upH").value +"/"+ document.getElementById("lowH").value +
                           "/"+document.getElementById("upSM").value +"/"+ document.getElementById("lowSM").value +
                           "/"+document.getElementById("L").value+"/";
            xhr.open("POST", "/BackEndTolerance", false);
            xhr.send(DataSend);
            window.open(location.href, "_self", "");
            window.close();
        }
    </script>
</body>
</html>
)rawliteral";
#pragma endregion Parameters_html
#pragma endregion
#pragma region Common Function
unsigned long getTime() // Get Timestamp
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {return(0);} //Failed to obtain time
  time(&now);
  return now;
}
void Make_Day()//Counter Day
{
  if((unsigned long) (millis()-Time_Passed) >= A_Day_milis){
    Time_Passed = millis();
    Days ++;
  }
}
#pragma endregion
#pragma region Check Internet Connected from Wifi
void Ping()// Ping to host
{
  AsyncClient* ping_client = new AsyncClient();
  ping_client->onConnect([](void* arg, AsyncClient* client){ping_flag = true;});
  ping_client->onError([](void* arg, AsyncClient* client, int8_t error) {ping_flag = false;});
  ping_client->connect(PING, 80);
  delay(1250);
  ping_client->stop();
}
void Cycle_Ping()// Cycle Ping to Host
{
  if(Last_ping_time == 0)
    Last_ping_time = millis();
  if((unsigned long)(millis()- Last_ping_time) > time_delay_to_ping){
    Last_ping_time = millis();
    Ping();
  }
}
#pragma endregion
#pragma region MQTT Protocol
void connect_to_broker() // Connect to the broker
{
  while (!client.connected()) {
    String clientId = "ESP32";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      client.subscribe(MQTT_Pump_TOPIC);
      client.subscribe(MQTT_LED_TOPIC);
      MQTTStatus = true;
    } else {
      MQTTStatus = false;
      break;
    }
  }
}
void callback(char* topic, byte *payload, unsigned int length)// Receive Messange From Broker
{
    char status[20];
    for(i = 0; i<length; i++){
        status[i] = payload[i];
    }
    if(String(topic) == MQTT_Pump_TOPIC){ 
        if(Ig_Pump)
          Ig_Pump = false;
        else
        { 
        if(String(status).indexOf("ON") != -1)
            Command_Pump = 1;
        else if(String(status).indexOf("OFF") != -1)
            Command_Pump = 2;
        } 
    }
    if(String(topic) == MQTT_LED_TOPIC){
        if(Ig_Led)
            Ig_Led = false;
        else{
            if(String(status).indexOf("ON") != -1)
                Command_Light = 1;
            else if(String(status).indexOf("OFF") != -1)
                Command_Light = 2;
        }
    }
}
#pragma endregion
#pragma region WebSocket Protocol
void notifyClients(const String data) //Notify all local clients with a message
{
  ws.textAll(data);
}
void notifyClient(AsyncWebSocketClient *client ,const String data) //Notify only one local client
{
  ws.text(client->id(),data);
}
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) //Handle messange from local clients
{
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if(String((char*)data).indexOf("Username:") >= 0){
      sta_ssid = String((char*)data).substring(String((char*)data).indexOf(' ')+1,String((char*)data).length());
    }
    if(String((char*)data).indexOf("Password:") >= 0){
      sta_password = String((char*)data).substring(String((char*)data).indexOf(' ')+1,String((char*)data).length());
      sta_flag = true;
    }
    if(String((char*)data).indexOf("Disconnect") >= 0){
      WiFi.disconnect(true);
      ping_flag = false;
      notifyClients("Wifi OFF");
    }
    if(String((char*)data).indexOf("Pump") >= 0){
      if(PumpsStatus)
        Command_Pump = 2;
      else
        Command_Pump = 1;
      if(WiFi.status() == WL_CONNECTED)
        Ig_Pump = true;
    }
    if(String((char*)data).indexOf("LED") >= 0){
      if(LightStatus)
        Command_Light = 2;
      else
        Command_Light = 1;
      if(WiFi.status() == WL_CONNECTED)
        Ig_Led = true;
    }
  }
}//Handle messange from local clients
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,void *arg, uint8_t *data, size_t len) //Handle event WebSocket
{
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      Person += 1;
      if(WiFi.status() == WL_CONNECTED)
        notifyClient(client,"Wifi ON");
      else
        notifyClient(client,"Wifi OFF");
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      Person -= 1;
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}//Handle event WebSocket
void initWebSocket() //Initialize the WebSocket protocol
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}
void Package_LC()
{
  messanger.clear();
  if(Temp[0] != (int)DHT_Err)
  {
    messanger += Local_Path[0];
    messanger += " ";
    messanger += String(DHT_Err);
    messanger += "/";
  }
  if(Temp[1] != (int)LDR_Err)
  {
    messanger += Local_Path[1];
    messanger += " ";
    messanger += String(LDR_Err);
    messanger += "/";
  }
  if(Temp[2] != (int)Soil_Err)
  {
    messanger += Local_Path[2];
    messanger += " ";
    messanger += String(Soil_Err);
    messanger += "/";
  }
  if(Temp[4] != (int)Humidity)
  {
    messanger += Local_Path[4];
    messanger += " ";
    messanger += String(Humidity);
    messanger += "/";
  }
  if(Temp[5] != (int)Temperature)
  {
    messanger += Local_Path[5];
    messanger += " ";
    messanger += String(Temperature);
    messanger += "/";
  }
  if(Temp[6] != (int)lumen)
  {
    messanger += Local_Path[6];
    messanger += " ";
    messanger += String(lumen);
    messanger += "/";
  }
  if(Temp[7] != (int)soilMoist)
  {
    messanger += Local_Path[7];
    messanger += " ";
    messanger += String(soilMoist);
    messanger += "/";
  }
  if(Temp[8] != (int)LightStatus)
  {
    messanger += Local_Path[8];
    messanger += " ";
    messanger += String(LightStatus);
    messanger += "/";
  }
  if(Temp[9] != (int)PumpsStatus)
  {
    messanger += Local_Path[9];
    messanger += " ";
    messanger += String(PumpsStatus);
    messanger += "/";
  }
  if(Person>0)
    notifyClients(messanger);
}
#pragma endregion
#pragma region Hypertext Transfer Protocol
String Info()//Make a String Current Data
{
  String Send_Value ="";
  for(i =0; i< 11; i++){
    Send_Value += String(Temp[i]);
    Send_Value += String(" ");
  }
  return Send_Value;
}
String processorMain(const String& var)//Replace placeholders on the HTML
{
  if(var == "HUMIDITY")
    return String(Humidity);
  if(var == "MOISTURE")
    return String(soilMoist);
  if(var == "TEMPERATURE")
    return String(Temperature);
  if(var == "LIGHT")
    return String(lumen);
  if(var == "INFO")
    if(WiFi.status() == WL_CONNECTED)
      return "Status: Connected";
    else
      return "Status: Not connected";
  if(var == "dht11_S")
    if(DHT_Err)
      return "Error";
    else
      return "OK";
  if(var == "mois_S")
    if(Soil_Err)
      return "Error";
    else
      return "OK";
  if(var == "Ligh_S")
    if(LDR_Err)
      return "Error";
    else
      return "OK";
  if(var == "Pump")
    if(PumpsStatus)
      return "ON";
    else
      return "OFF";
  if(var == "LED")
    if(LightStatus)
      return "ON";
    else
      return "OFF";
}//Replace placeholders on the HTML
String processorTolerance(const String& var)
{
  if(var == "UPT")
    return String(Danger_Temp);
  if (var == "LOWT")
    return String(Save_Temp);
  if(var == "UPH")
    return String(Danger_Humi);
  if (var == "LOWH")
    return String(Save_Humi);
  if(var == "UPSM")
    return String(WET_SOIL);
  if (var == "LOWSM")
    return String(DRY_SOIL);  
  if (var == "L")
    return String(DARK_LIGHT);
}
#pragma endregion
#pragma region Cloud Database
void Setup_RTDB()//Initiate Realtime Database Firebase
{
  Firebase.begin(DATABASE_URL,Database_Secrets);
  Firebase.reconnectWiFi(true);
  firebaseData.setResponseSize(4096);
}
boolean Update_Package_Send()
{
  boolean ChangeValue = false;
  if(Temp[0] != (int)DHT_Err )
  {
    ChangeValue = true;
    json.set(Child_Path[0].c_str(),DHT_Err);
    Temp[0] = (int)DHT_Err;
  }
  if(Temp[1] != (int)LDR_Err )
  {
    ChangeValue = true;
    json.set(Child_Path[1].c_str(),LDR_Err);
    Temp[1] = (int)LDR_Err;
  }
  if(Temp[2] != (int)Soil_Err )
  {
    ChangeValue = true;
    json.set(Child_Path[2].c_str(),Soil_Err);
    Temp[2] = (int)Soil_Err;
  }
  if(Temp[3] != (int)Days )
  {
    ChangeValue = true;
    json.set(Child_Path[3].c_str(),Days);
    Temp[3] = (int)Days;
  }
  if(Temp[4] != (int)Humidity )
  {
    ChangeValue = true;
    Temp[4] = (int)Humidity;
    json.set(Child_Path[4].c_str(),String(Humidity));
  }
  if(Temp[5] != (int)Temperature )
  {
    ChangeValue = true;
    json.set(Child_Path[5].c_str(),String(Temperature));
    Temp[5] = (int)Temperature;
  }
  if(Temp[6] != (int)lumen )
  {
    ChangeValue = true;
    json.set(Child_Path[6].c_str(),lumen);
    Temp[6] = (int)lumen;
  }
  if(Temp[7] != (int)soilMoist )
  {
    ChangeValue = true;
    json.set(Child_Path[7].c_str(),soilMoist);
    Temp[7] = (int)soilMoist;
  }
  if(Temp[8] != (int)LightStatus )
  {
    ChangeValue = true;
    json.set(Child_Path[8].c_str(),LightStatus);
    Temp[8] = (int)LightStatus;
  }
  if(Temp[9] != (int)PumpsStatus )
  {
    ChangeValue = true;
    json.set(Child_Path[9].c_str(),PumpsStatus);
    Temp[9] = (int)PumpsStatus;
  }
  if(Temp[10] != (int)MQTTStatus )
  {
    ChangeValue = true;
    json.set(Child_Path[10].c_str(),MQTTStatus);
    Temp[10] = (int)MQTTStatus;
  }
  return ChangeValue;
}
void DataLogging()//Store a record to database
{
  if(WiFi.status() != WL_CONNECTED || !ping_flag || first_sta)
    return;
  if(Firebase.ready() && ((unsigned long)(millis()- Last_datalogging_time)>time_delay_send_datalogging)||Last_datalogging_time == 0){
    timestamp = getTime();
    if(timestamp == 0)
    {
      Last_datalogging_time = 0;
      return;
    }
    Last_datalogging_time = millis();
    Destinate = Parent_Path;
    Destinate += "DataLog/";
    Destinate += String(timestamp);
    Destinate += "/";
    Firebase.RTDB.setJSON(&firebaseData, Destinate.c_str(), &json);
    //json.clear();
  }
}
void DeleteOldData(int mode = 0)//Delete Old Record on database
{
  if(Delay_Delete_Data != 0)
  {
    if ((unsigned long)(millis() - Wait_Delete) >= (unsigned long)(Delay_Delete_Data * 1000))
    {
      Wait_Delete = 0;
      Delay_Delete_Data = 0;
    }
    else
      return;
  }
  if(WiFi.status() != WL_CONNECTED || !ping_flag || first_sta)
    return;
  timestamp = getTime();
  if(timestamp == 0)
    return;
  QueryFilter query;
  timestamp -= expired_data;
  if(mode == 0)
    query.orderBy("$key").startAt("0").endAt(String(timestamp)).limitToFirst(4);
  else
    query.orderBy("$key").limitToFirst(1);

  if(Firebase.ready())
  {
    Destinate = Parent_Path;
    Destinate += "DataLog";
    if(Firebase.RTDB.getJSON(&firebaseData,Destinate,&query))
    {
      if(firebaseData.dataType() == "null" || firebaseData.dataType() == "array")
      {
        query.clear();
        if (mode == 0)
          DeleteOldData(1);
        return;
      }
      if (firebaseData.dataType() == "json" && firebaseData.jsonString().length() > 4)
      {
        Djson = firebaseData.jsonObject();
        size_t len = Djson.iteratorBegin();
        String key, value;
        int otype = 0;
        for(size_t i = 0; i < len; i += total_key)
        {
          Djson.iteratorGet(i,otype,key,value);
          if (mode == 0)
          {
            String path = Destinate;
            path += "/";
            path += key;
            Firebase.RTDB.deleteNode(&firebaseData, path);
          }
          else
          {
            sscanf(key.c_str(),"%lu",&Delay_Delete_Data);
            Delay_Delete_Data -= timestamp;
            Wait_Delete = millis();
            break;
          }

        }
        Djson.iteratorEnd();
        Djson.clear();
      }
    }
  }
  query.clear();
}
void Package_NW()
{
  if(Update_Package_Send() && WiFi.status() == WL_CONNECTED && ping_flag && !first_sta && Firebase.ready())
    Firebase.RTDB.updateNodeSilentAsync(&firebaseData,Parent_Path.c_str() , &json);
}
#pragma endregion
#pragma region Network
void Setup_Server()//Initiate connection to the servers
{
  if(!first_sta)
    return;
  configTime(gmtOffset_sec, daylightOffset_sec, NTPserver1,NTPserver2,NTPserver3);
  Setup_RTDB();
  client.setServer(MQTT_SERVER, MQTT_PORT );
  client.setCallback(callback);
  connect_to_broker();
  first_sta = false;
}//Initiate connection to the servers
void Connect_Network()//Connect to Wifi Router
{
  if(sta_ssid == NULL || sta_password == NULL)
    return;
  WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
  long current = millis();
  while (WiFi.status() != WL_CONNECTED && (unsigned long) (millis()- current) < Network_TimeOut)
  {
    delay(500);
  }
  if(WiFi.status() != WL_CONNECTED)
  {
    if(Person > 0)
      notifyClients("Wifi OFF");
  }
  else
  {
    if(Person > 0)
      notifyClients("Wifi ON");
    Ping();
    if(ping_flag)
      Setup_Server();
  }
}
void Init_Server()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if(ON_STA_FILTER(request)) //Only for client from AP Mode
      return request->redirect("/NothingHereForYou");
    if(!request->authenticate(http_username.c_str(), http_password.c_str()))
      return request->requestAuthentication();
    request->send_P(200, "text/html", main_html, processorMain);
  });//Home Page Server
  server.on("/Test",HTTP_GET,[](AsyncWebServerRequest *request){
    WiFi.softAPdisconnect();
  });
  server.on("/Sercurity",HTTP_GET,[](AsyncWebServerRequest *request){
    if(ON_STA_FILTER(request)) //Only for client from AP Mode
      return request->redirect("/NothingHereForYou");
    if(!request->authenticate(http_username.c_str(), http_password.c_str()))
      return request->requestAuthentication();
    sercurity_backend_key = true;
    request->send_P(200,"text/html",Sercurity_html);
  });
  server.on("/BackEndSercure",HTTP_POST,[](AsyncWebServerRequest *request){
    if(ON_STA_FILTER(request)) //Only for client from AP Mode
      return request->redirect("/NothingHereForYou");
    if(!request->authenticate(http_username.c_str(), http_password.c_str()))
      return request->requestAuthentication();
    if(!sercurity_backend_key)
      return request->redirect("/WrongDoor");
    request->send(100);
  },NULL,[](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total){
    if(!sercurity_backend_key)
      return request->send(410);
    String TmpID = String((char*) data).substring(String((char*) data).indexOf(' ')+1,String((char*) data).indexOf('/'));
    String TmpPass = String((char*) data).substring(String((char*) data).indexOf('/')+1,String((char*) data).lastIndexOf('/'));
    if(TmpID.length() > 63 || TmpID == NULL || TmpPass.indexOf(' ') >= 0 || ( TmpPass.length() < 8 && TmpPass != NULL))
      return request->send(400);
     
    if(String((char*) data).indexOf("ADMIN: ")>=0)
    {
      http_username = TmpID;
      http_password = TmpPass;
      request->send(204);
    }
    else if(String((char*) data).indexOf("AP: ")>=0)
    {
      ap_ssid = TmpID;
      ap_password = TmpPass;
      request->send(511);
      Person = 0;
      WiFi.softAP(ap_ssid.c_str(),ap_password.c_str());
    }
    else
      request->send(444);
    sercurity_backend_key = false;
  });
  server.on("/Tolerance",HTTP_GET,[](AsyncWebServerRequest *request){
    if(ON_STA_FILTER(request)) //Only for client from AP Mode
      return request->redirect("/NothingHereForYou");
    if(!request->authenticate(http_username.c_str(), http_password.c_str()))
      return request->requestAuthentication();
    tolerance_backend_key = true;
    request->send_P(200,"text/html",Tolerance_html,processorTolerance);
  });
  server.on("/BackEndTolerance",HTTP_POST,[](AsyncWebServerRequest *request){
    if(ON_STA_FILTER(request)) //Only for client from AP Mode
      return request->redirect("/NothingHereForYou");
    if(!request->authenticate(http_username.c_str(), http_password.c_str()))
      return request->requestAuthentication();
    if(!sercurity_backend_key)
      return request->redirect("/WrongDoor");
    request->send(100);
  },NULL,[](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total){
    if(!tolerance_backend_key)
      return request->send(410);
    sscanf(strtok((char*)data,"/"),"%d",&Danger_Temp);
    sscanf(strtok(NULL,"/"),"%d",&Save_Temp);
    sscanf(strtok(NULL,"/"),"%d",&Danger_Humi);
    sscanf(strtok(NULL,"/"),"%d",&Save_Humi);
    sscanf(strtok(NULL,"/"),"%d",&WET_SOIL);
    sscanf(strtok(NULL,"/"),"%d",&DRY_SOIL);
    sscanf(strtok(NULL,"/"),"%d",&DARK_LIGHT);
    return request->send(200);
  });
  server.on("/logout",HTTP_GET,[](AsyncWebServerRequest *request){
    request->send(401);
  });
  server.onNotFound([](AsyncWebServerRequest *request){
    if(ON_STA_FILTER(request)) //Only for client from AP Mode
      return request->redirect("https://youtu.be/dQw4w9WgXcQ");
    request->send_P(404,"text/html",Error_html);
  });
  
  // Start server
  server.begin();
}
#pragma endregion
#pragma region Sensor Data Read
int Get_Sensor(int anaPin)// Get Data From Light Sensor & Soild Sensor
{
  int value = 0;
  value = analogRead(anaPin);
  if(value > 4095)
    value = 4095;
  if(value < 1500)
    value = 1500;
  value = map(value,4095,1500,0,100);
  return value;
} 
void Check()// Check error sensor
{
  if(isnan(Humidity) || isnan(Temperature) || Humidity > 90 || Temperature > 50 ){
      DHT_Err = true;
    }
  else{
      DHT_Err = false;
    }
  if(lumen < 0 || lumen > 100){
      LDR_Err = true;
    }
  else{
      LDR_Err = false;
    }
  if(soilMoist < 0 || soilMoist > 100){
      Soil_Err = true;
    }
  else{
      Soil_Err = false;
    }
  if(DHT_Err || LDR_Err || Soil_Err)
    Err = true;
  else if(Err)
    Err = false;
}
void Read_Sensor()//Get Data from All Sensors
{
  lumen = Get_Sensor(LDR);
  delay(500);
  soilMoist = Get_Sensor(Soil_Moisture);
  delay(500);
  Humidity = dht.readHumidity();
  Temperature = dht.readTemperature();
  Check();
}
#pragma endregion
#pragma region Controlled Things
void Condition_Pump()//Check watering conditions
{
  if(((soilMoist < DRY_SOIL && !Err ||(unsigned long)(millis()-Times_Pumps) >= Next_Pump) || Command_Pump == 1) && !PumpsStatus) //Đang tắt
  {
    Times_Pumps = millis();
    PumpsStatus = true;
    if(Command_Pump == 2)
      Command_Pump = 0;
    if(Command_Pump == 1)
      return;
  }
  if((((Temperature >= Danger_Temp || Temperature <= Save_Temp || soilMoist > WET_SOIL) && !Err || ((unsigned long)(millis()- Times_Pumps) >= Still_Pumps)) || Command_Pump == 2) && PumpsStatus) //Đang bật
  {
    Times_Pumps = millis(); 
    PumpsStatus = false;
    if(Command_Pump == 1)
      Command_Pump = 0;
    if(Command_Pump ==2)
      return;
  }
}
void Pump()//Pump Choice
{
  if(Err && Command_Pump == 0)
    PumpsStatus = false;
  else
    Condition_Pump();
  if(PumpsStatus)
    digitalWrite(Pumps,HIGH);
  else 
    digitalWrite(Pumps,LOW);
}
void Condition_Light()//Check lighting up conditions
{
  if((lumen <= DARK_LIGHT && !Err ||  Command_Light == 1 )&& !LightStatus) //Đang tắt
    {
      LightStatus = true;
      if(Command_Light == 2)
        Command_Light = 0;
      if(Command_Light ==1)
        return;
    }
  if((lumen > DARK_LIGHT && !Err || Command_Light == 2) && LightStatus) // Đang bật
    {
      LightStatus = false;
      if(Command_Light == 1)
        Command_Light = 0;
      if(Command_Light ==2)
        return;
    }
}
void Light_Up()//Light up choice
{
  if(Err && Command_Light == 0)
    LightStatus = false;
  else
    Condition_Light();
  if(LightStatus)
    digitalWrite(Light,HIGH);
  else 
    digitalWrite(Light,LOW);
}
#pragma endregion
#pragma region Main System
void Network()// Netword Part
{
  ws.cleanupClients();
  Package_LC();
  Package_NW();
  DataLogging();
  DeleteOldData();
  if(sta_flag)
  {
    Connect_Network();
    sta_flag = false;
  }  
  if(WiFi.status() != WL_CONNECTED){
    if(Last_ping_time != 0) Last_ping_time = 0;
    if(ping_flag) ping_flag = false;
    return;
  }
  Cycle_Ping();
  if(!ping_flag)
    return;
  Setup_Server();// If it doesn't setup
  client.loop();
  if (!client.connected()) {
    connect_to_broker();
  }
}
void Auto()//Auto Part
{
  Make_Day();
  Read_Sensor();
  Check();
  Pump();
  Light_Up();
}
#pragma endregion
#pragma region Arduino program structure
void setup() 
{
  Serial.begin(9600);
  WiFi.mode(WIFI_AP_STA);
  Connect_Network();
  WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
  initWebSocket();
  Init_Server();
  pinMode(Pumps,OUTPUT);
  pinMode(Light,OUTPUT);
  digitalWrite(Pumps,LOW);
  digitalWrite(Light,LOW);
  dht.begin();
  Time_Passed = millis();
}
void loop() 
{
  Auto();
  Network();
}
#pragma endregion
