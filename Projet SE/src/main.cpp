#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <analogWrite.h>

bool bpState = 0;
bool lastState = 0;
int BP = 18;

const int ledbp = 4;
const int ledOnOff = 2;
const int ledLum = 15;

bool ledbpState = LOW;
bool ledLumState = LOW;


const char* ssid = "NOURA";
const char* password = "N1999zar";


const char* mqtt_server = "192.168.231.182";
const int   mqtt_port = 1883;

#define DHTPIN 17
#define DHTTYPE    DHT11
DHT dht(DHTPIN, DHTTYPE);

float t = 0.0;
float h = 0.0;


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
      client.subscribe("LED");
      client.subscribe("Green");
      client.subscribe("Blue");
      client.subscribe("Red");
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
  
  if (String(topic) == "LED"){
    Serial.print("LED = ");
    if (String(message) == "true"){ 
      Serial.print("ON");
      analogWrite(Gpin, green,255);
      analogWrite(Rpin, red);
      analogWrite(Bpin, blue);
      }
    else if (String(message) == "false"){
      Serial.print("OFF");
      analogWrite(Gpin, 0);
      analogWrite(Rpin, 0);
      analogWrite(Bpin, 0);
      }
    else{
      Serial.print("Err");
      digitalWrite(ledPin,HIGH);
      delay(200);
      digitalWrite(ledPin,LOW);
      delay(200);
      digitalWrite(ledPin,HIGH);
      delay(100);
      digitalWrite(ledPin,LOW);
      delay(100);

    }
  }
  else if (String(topic) == "Green" or "Blue" or "Red"){
    
    int pin;
    int color = atoi(message);
    Serial.print("Brightness : ");
    Serial.print(message);
    Serial.println();

    if (String(topic) == "Green"){
      green = color;
      pin = Gpin;
    }
    else if (String(topic) == "Red"){
      red = color;
      pin = Rpin;
    }
    else if (String(topic) == "Blue"){
      blue = color;
      pin = Bpin;
    } 
    analogWrite(pin,color);
  }
  else{ Serial.println("Error topic");
  }
  Serial.println();
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
      client.subscribe("LED");
      client.subscribe("Green");
      client.subscribe("Blue");
      client.subscribe("Red");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void task_BP(){
//Lecture BP
bpState = digitalRead(BP);
if(bpState != lastState){
    if(bpState){
        //ON
    }
    else{
        //OFF
    }
    lastState = bpState;
}

void task_lectdata(){  
//lecture temp

//lecture hum

//Lecture lum
}

void task_dataProcess(){
if (!client.connected()) {
reconnect();
}

client.loop(); 

char strT[9];
char strH[7];
  

sprintf(strT,"%2.1f",t);
sprintf(strH,"%03.1f",h);
unsigned long now = millis();
  if (now - lastMsg > 15000) {
    t = dht.readTemperature();
    h = dht.readHumidity();
    
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.print(" BP = ");
    Serial.println(BPstate);
 
    client.publish("HUM",strH);
    client.publish("TEMP",strT);

    Serial.print(" hum = ");
    Serial.print(h);
    Serial.print(" temp = ");
    Serial.print(t);
    Serial.print(" ");
    Serial.println(" ");
  }
//Envoi temp

//Envoi hum

//Envoi lum

//Affichage donnxes

}

void task_lum(){
//if lum -> sombre
    //Allumer led
//else
    //xteindre led

}

void setup(){
  dht.begin();
  pinMode(ledPin, OUTPUT);
  pinMode(BPpin,INPUT);
  digitalWrite(ledPin, ledState);

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server,mqtt_port);
  client.setCallback(callback);

}

void loop(){

}