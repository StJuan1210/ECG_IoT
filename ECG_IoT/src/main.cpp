#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Ticker.h>
#include <Wire.h> 
#include "BluetoothSerial.h"
LiquidCrystal_I2C lcd(0x27, 16, 2);
#define LED_BUILTIN 2
int t = 0;
BluetoothSerial ESP_BT;
boolean BT_cnx = false;
Ticker timer;
bool get_data = false;
int check1=0,check2=0;

// Connecting to the Internet
char * ssid = "Mattackal 2.4";
char * password = "burka123";

float ECGval() {
  
  float p = analogRead(A0);
  Serial.println(p);
  return p;
  delay(10);
    
}
void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param){
  if(event == ESP_SPP_SRV_OPEN_EVT){
    Serial.println("Client Connected");
    digitalWrite(LED_BUILTIN, HIGH);
    BT_cnx = true;
  }
 
  if(event == ESP_SPP_CLOSE_EVT ){
    Serial.println("Client disconnected");
    digitalWrite(LED_BUILTIN, LOW);
    BT_cnx = false;
    ESP.restart();
  }
}
// Running a web server
WebServer server;
WebSocketsServer webSocket = WebSocketsServer(81);

char webpage[] PROGMEM = R"=====(
        <html>
        <head>
          <script src='https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.5.0/Chart.min.js'></script>
        </head>
        <body onload="javascript:init()">
        <div>
          <input type="range" min="1" max="10" value="5" id="dataRateSlider" oninput="sendDataRate()" />
          <label for="dataRateSlider" id="dataRateLabel">Rate: 0.2Hz</label>
        </div>
        <hr />
        <div>
          <canvas id="line-chart"></canvas>
        </div>
        <style>
            @import url('https://fonts.googleapis.com/css2?family=Space+Mono&display=swap');
            html {font-family: 'Space Mono',monospace; 
                  display: inline-block; 
                  text-align: center; 
                  background: #000000;
            }
        .chart-container {
            width: 600px;
            height:400px
        }
        </style>
        <script>
          var webSocket, dataPlot;
          var maxDataPoints = 1000;
          function removeData(){
            dataPlot.data.labels.shift();
            dataPlot.data.datasets[0].data.shift();
          }
          function addData(label, data) {
            if(dataPlot.data.labels.length > maxDataPoints) removeData();
            dataPlot.data.labels.push(label);
            dataPlot.data.datasets[0].data.push(data);
            dataPlot.update();
          }
          function init() {
            webSocket = new WebSocket('ws://' + window.location.hostname + ':81/');
            dataPlot = new Chart(document.getElementById("line-chart"), {
              type: 'line',
              data: {
                labels: [],
                datasets: [{
                  data: [],
                  label: "ECG",
                  borderColor: "#3e95cd",
                  fill:false,
                }]
              }
            });
            webSocket.onmessage = function(event) {
              var data = JSON.parse(event.data);
              var today = new Date();
              var t = today.getHours() + ":" + today.getMinutes() + ":" + today.getSeconds();
              addData(t, data.value);
            }
          }
          function sendDataRate(){
            var dataRate = document.getElementById("dataRateSlider").value;
            webSocket.send(dataRate);
            dataRate = 1.0/dataRate;
            document.getElementById("dataRateLabel").innerHTML = "Rate: " + dataRate.toFixed(2) + "Hz";
          }
        </script>
        </body>
        </html>
)=====";
int check3=0;
void setup() {
  lcd.begin();
  lcd.backlight();

  check1 = digitalRead(5);
  if(check1==1){
 
  WiFi.begin(ssid, password);
  Serial.begin(115200);
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Wifi Mode");
  while(WiFi.status()!=WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/",[](){
    server.send_P(200, "text/html", webpage);
  });
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  timer.attach_ms(5, getData);
  check2 = 1;
  
  }
else {check2 = 0;
  Serial.begin(115200);
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Bluetooth Mode");
  ESP_BT.register_callback(callback);
  if(!ESP_BT.begin("ESP32_ECG")){
    Serial.println("An error occurred initializing Bluetooth");
  }else{
    Serial.println("Bluetooth initialized... Bluetooth Device is Ready to Pair...");
  }
}

}

void loop() {
  if(check2==1){
  wifiloop();
  }
  else if(check2==0){
    btloop();
  } 
  check3 = digitalRead(5);
  if(check3 != check1){
    ESP.restart();
  }
}

void getData() {
  get_data = true;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  // Do something with the data from the client
  if(type == WStype_TEXT){
    float dataRate = (float) atof((const char *) &payload[0]);
    timer.detach();
    timer.attach(dataRate, getData);
  }
}
void wifiloop(){
  webSocket.loop();
  server.handleClient();
  if(get_data){
    Serial.println(ECGval());
    String json = "{\"value\":";
    json += ECGval();
    json += "}";
    webSocket.broadcastTXT(json.c_str(), json.length());
    get_data = false;
    t++;
  }
   
}
void btloop(){
  if(BT_cnx){
      ESP_BT.print('E'); 
      ESP_BT.println(analogRead(A0));
  }
  delay(1); 
}
