#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <analogWrite.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static SemaphoreHandle_t Token;
//static SemaphoreHandle_t ledBsem; 

bool bpState = 0;
bool lastState = 0;
int BP = 18;

const int ledbp = 4;
const int ledOnOff = 2;
const int ledLum = 15;

bool ledbpState = LOW;
bool ledLumState = LOW;

unsigned long lastMsg;

const char* ssid = "VOO-006709";
const char* password = "JHPYEPQQ";

const char* mqtt_server = "192.168.0.30";
const int   mqtt_port = 1883;

const int LumPin = 36;

#define DHTPIN 17
#define DHTTYPE    DHT11
DHT dht(DHTPIN, DHTTYPE);

float t = 0.0;
float h = 0.0;
float l = 0.0;

char strT[9];
char strH[7];
char strL[7];

bool OnOff = 0;
bool lectState =0;

WiFiClient espClient;
PubSubClient client(espClient);
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
long int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID

    String Id = "ID - ESP  32" ;
    Id += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(Id.c_str())) {             //Connection mqtt broker
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("OnOff");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println(length);
  Serial.println((int)payload[0]);
  char message[length] ="";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]_");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message[i] = (char)payload[i];
  }
    
  message[length] = '\0';
  Serial.println("_");
  Serial.print("___");
  Serial.print(message);
  Serial.println("___");

  Serial.println();
  
  if (String(topic) == "OnOff"){
    Serial.print("LED = ");
    if (String(message) == "ON"){ 
      Serial.print("ON");
      OnOff = 1;
      //Tout allumer
      }

    else if (String(message) == "OFF"){
      //Tout xteindre
      OnOff = 1;
      }

    else{
      Serial.print("Err");
      digitalWrite(ledbp,HIGH);
      delay(200);
      digitalWrite(ledbp,LOW);
      delay(200);
      digitalWrite(ledbp,HIGH);
      delay(100);
      digitalWrite(ledbp,LOW);
      delay(100);

    }
  }
  else{ Serial.println("Error topic");
  }

  Serial.println();
}

void task_BP(void * arg){
//Lecture BP
  for(;;){
    bpState = digitalRead((int)arg);
      if(bpState != lastState){
        if(bpState){
            //ON
            digitalWrite(BP,HIGH);
            lectState = 1;
        }

        else{
            //OFF
            digitalWrite(BP,LOW);
            lectState = 0;
        }
        lastState = bpState;
      }
  }

}

void task_lectdata(void * arg){ 
  BaseType_t rc;

  for(;;){
  rc = xSemaphoreTake(Token,portMAX_DELAY); 
  assert(rc == pdPASS);
    if (!client.connected()) {
    reconnect();
}

client.loop(); 

sprintf(strT,"%2.1f",t);
sprintf(strH,"%3.1f",h);
sprintf(strL,"%3.1f",l);

unsigned long now = millis();
  if (now - lastMsg > 15000 or lectState) {

    //take sema
    // take sema lum

    t = dht.readTemperature();
    h = dht.readHumidity();
    l = analogRead((int)arg)*100/4096;
    //lecture temp

    //lecture hum

    //Lecture lum
    
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);  

    //give sema 
    //give sema lum
    rc = xSemaphoreGive(Token);  
    assert(rc == pdPASS);

  }
  } 
}

void task_dataProcess(void * arg){
BaseType_t rc;

for(;;){
  //take sema
  rc = xSemaphoreTake(Token,portMAX_DELAY); 
  assert(rc == pdPASS);
  Serial.print("Publish message: ");
  client.publish("HUM",strH);
  client.publish("TEMP",strT);
  client.publish("LUM",strL);
  

  Serial.print(" hum = ");
  Serial.print(h);
  Serial.print(" temp = ");
  Serial.print(t);
  Serial.print(" lum = ");
  Serial.print(l);
  
  Serial.print(" ");
  Serial.println(" ");

//Affichage donnxes

//give sema
rc = xSemaphoreGive(Token);  
assert(rc == pdPASS);
}

}

void task_lum(void * arg){
  BaseType_t rc;
  for(;;){
  
  rc = xSemaphoreTake(Token,portMAX_DELAY); 
  assert(rc == pdPASS);
  if (l < 50){
    digitalWrite((int)arg,HIGH);
  }
  else{
    digitalWrite((int)arg,LOW);
  }
  }

  rc = xSemaphoreGive(Token);  
  assert(rc == pdPASS);
//if lum -> sombre
    //Allumer led
//else
    //xteindre led

}

void setup(){
  BaseType_t rc;

  dht.begin();
  pinMode(ledbp, OUTPUT);
  pinMode(BP,INPUT);
  digitalWrite(ledbp, ledbpState);

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server,mqtt_port);
  client.setCallback(callback);

  rc = xTaskCreate(task_dataProcess,"process",1000,NULL,2,&Token); 
  assert(rc == pdPASS); 

  rc = xTaskCreate(task_BP,"BP",1000,(void*)BP,1,&Token);
    assert(rc == pdPASS); 

  //Creation of ledA task 
  rc = xTaskCreate(task_lectdata,"lectdata",1000,(void*)LumPin,1,&Token);
  assert(rc == pdPASS); 

    rc = xTaskCreate(task_lum,"Lum",1000,(void*)ledLum,1,&Token);
  assert(rc == pdPASS); 

}

void loop(){

} 