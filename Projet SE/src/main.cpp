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
 
static TaskHandle_t tasklect;
static TaskHandle_t taskprocess;
static TaskHandle_t tasklum;


bool bpState = 0;
bool lastState = 0;
int BP = 18;

const int ledbp = 4;
const int ledOnOff = 2;
const int ledLum = 15;
int lum;

bool ledbpState = LOW;
bool ledLumState = LOW;

unsigned long lastMsg;

const char* ssid = "VOO-006709";
const char* password = "JHPYEPQQ";

const char* mqtt_server = "192.168.0.30";
const int   mqtt_port = 1883;

const int LumPin = 32;

#define DHTPIN 12
#define DHTTYPE    DHT11
DHT dht(DHTPIN, DHTTYPE);

double t = 0.0;
double h = 0.0;
double l = 0.0;

char strT[9];
char strH[7];
char strL[8];

bool OnOff = 1;
bool lectState =0;
bool start = 1;
bool lumState = 0;
bool prosState = 0;
bool stop = 0;

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


void task_lectdata(void * arg){ 
  for(;;){
    Serial.println("Lect data ...");

    //take sema
    // take sema lum

    t = dht.readTemperature();
    h = dht.readHumidity();
    lum = analogRead((int)arg);
    l = lum*100;
    l = l/4096;
    vTaskDelay(500/portTICK_PERIOD_MS);

    if(isnan(h) or isnan(t)){h = 0;
    t= 0;
    Serial.println("error dht");}
   
    //lecture temp
    //lecture hum
    //Lecture lum

    sprintf(strT,"%2.1f",t);
    sprintf(strH,"%3.1f",h);
    sprintf(strL,"%3.1f",l);  

    //give sema 
    //give sema lum
    vTaskResume(taskprocess);     
    vTaskSuspend(tasklect);
   
  }

  }

void task_dataProcess(void * arg){

for(;;){
  //take sema
  
  
  Serial.print("Publish message: ");
  client.publish("esp1/humidite",strH);
  client.publish("esp1/temperature",strT);
  client.publish("esp1/luminosite",strL);
  

  Serial.print(" hum = ");
  Serial.print(strH);
  Serial.print(" temp = ");
  Serial.print(strT);

  Serial.print(" lum = ");
  Serial.print(strL);
  
  Serial.print(" ");
  Serial.println(" ");  

 
   vTaskResume(tasklum);
  Serial.println("stopped");
   vTaskSuspend(taskprocess);

//Affichage donnxes

}

}

void task_lum(void * arg){
  for(;;){
  if (l < 0.5){
    digitalWrite((int)arg,HIGH);
  }
  else{
    digitalWrite((int)arg,LOW);
  }
  vTaskSuspend(tasklum);


}}

void setup(){
 
  pinMode(ledbp, OUTPUT);
  pinMode(BP,INPUT);
  pinMode(LumPin,INPUT);
  pinMode(ledLum,OUTPUT);
  pinMode(ledOnOff,OUTPUT);
  digitalWrite(ledbp, ledbpState);
  digitalWrite(ledLum,LOW);

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server,mqtt_port);
  client.setCallback(callback);
  
  if (!client.connected()) {
    reconnect();
    client.loop(); 
  }

  xTaskCreatePinnedToCore(task_lectdata,"lectdata",8000,(void*)LumPin,1,&tasklect,NULL); 
  vTaskSuspend(tasklect);
  //Creation of ledA task 
  xTaskCreatePinnedToCore(task_dataProcess,"process",8000,NULL,1,&taskprocess,NULL);
  vTaskSuspend(taskprocess);

  xTaskCreatePinnedToCore(task_lum,"Lum",8000,(void*)ledLum,1,&tasklum,NULL);
  vTaskSuspend(tasklum);


  dht.begin();

}

void loop(){
  if (OnOff){

    while (!client.connected()) {
    reconnect();
    client.loop();}

     if(!digitalRead(ledOnOff)){
    digitalWrite(ledOnOff,HIGH);}

    bpState = digitalRead(BP);
      if(bpState != lastState){
        if(bpState){
            digitalWrite(ledbp,HIGH);
            lectState = 1;
        }

        else{
            digitalWrite(ledbp,LOW);
           
        }
        lastState = bpState;
        Serial.println("BP State");
        Serial.println(lectState);
      }
      

  unsigned long now = millis();
  if (now - lastMsg > 12000) {

    if(eTaskGetState(tasklect) == eSuspended){
    vTaskResume(tasklect); }
    
     lastMsg = now;   
    }
    
  if (lectState){
    if(eTaskGetState(tasklect) == eSuspended){
    vTaskResume(tasklect);
     lectState = 0;
    }
  }
  }
  else{
    vTaskSuspend(tasklect);
    vTaskSuspend(taskprocess);
    vTaskSuspend(tasklum);

    digitalWrite(ledbp,LOW);
    digitalWrite(ledLum,LOW);
    digitalWrite(ledOnOff,LOW);
  }


} 