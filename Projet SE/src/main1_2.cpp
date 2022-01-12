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

//Paramxtres OLED
#define nombreDePixelsEnLargeur 128        
#define nombreDePixelsEnHauteur 64       
#define brocheResetOLED         -1       
#define adresseI2CecranOLED     0x3C        
Adafruit_SSD1306 ecranOLED(nombreDePixelsEnLargeur, nombreDePixelsEnHauteur, &Wire, brocheResetOLED,100000UL, 100000UL);


 //Handles  des taches
static TaskHandle_t tasklect;
static TaskHandle_t taskprocess;
static TaskHandle_t tasklum;

//Variables BP
bool bpState = 0;
bool lastState = 0;
int BP = 18;

//Varables LEDs
const int ledbp = 4;
const int ledOnOff = 2;
const int ledLum = 15;

bool ledbpState = LOW;
bool ledLumState = LOW;

//Variables 'timer'
unsigned long lastMsg;

//Paramxtres Wifi
const char* ssid = "VOO-006709";
const char* password = "JHPYEPQQ";

//Variables wifi client
WiFiClient espClient;
PubSubClient client(espClient);
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
long int value = 0;

//Paramxtres mqtt
const char* mqtt_server = "192.168.0.30";
const int   mqtt_port = 1883;

//Variables LDR
int lum;
const int LumPin = 36;

//Variables DHT
#define DHTPIN 12
#define DHTTYPE    DHT11
DHT dht(DHTPIN, DHTTYPE);

//Variables donnxes capteurs
double t = 0.0;
double h = 0.0;
double l = 0.0;

char strT[9];
char strH[7];
char strL[8];

//Variables gestion taches
bool OnOff = 1;
bool lectState =0;
bool start = 1;
bool lumState = 0;
bool prosState = 0;
bool stop = 0;

void setup_wifi() {
/*
Fonction qui xtablit la connexion de l'esp au wifi
*/
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  //Connexion au wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  
  while (WiFi.status() != WL_CONNECTED) {//Attends que l'esp se connecte
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());// Affichage de l'adresse IP sur le port sxrie
}

void reconnect(){
  /*
Fonction qui (re)connecte l'esp au serveur mqtt
*/
    ecranOLED.clearDisplay();
    ecranOLED.setTextSize(3);
    ecranOLED.setCursor(30, 0); 
    ecranOLED.setTextColor(SSD1306_BLACK,SSD1306_WHITE);  
    ecranOLED.println("Connecting...");
    

  while (!client.connected()) { //Attends une connexion au serveur
    
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID

    String Id = "ID - ESP  32" ;
    Id += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(Id.c_str())) { //Si esp connectx au mqtt
      Serial.println("connected");
      
      client.subscribe("OnOff"); //Subscribe au topic voulu
    } 
    else { //Si connexion xchouxe
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000); //Delay de 5s sec avant de rxessayer
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
/*
Fonction qui sera appelxe x chaque fois que l'esp recois un msg provenant du mqtt
afin d'en traiter le message recu
*/

  Serial.println(length); //Affichage message
  Serial.println((int)payload[0]);
  char message[length] ="";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]_");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message[i] = (char)payload[i]; //Enregistrement du message dans une chaine de caractxres
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
      digitalWrite(ledOnOff,HIGH);
      delay(400);
      digitalWrite(ledOnOff,LOW);
      OnOff = 1; //Tout allumer
      
      }

    else if (String(message) == "OFF"){
      OnOff = 0; //Tout xteindre
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
  /*
  Tache qui lis les donnxes xmises par les capteurs (DHT11 et LDR)
  et qui formate ces donnxes dans des strings
  */
  for(;;){
    Serial.println("Lect data ...");

    //Lecture des donnxes
    t = dht.readTemperature(); 
    h = dht.readHumidity();
    lum = analogRead((int)arg);
    l = lum*100;
    l = l/4096;
    vTaskDelay(500/portTICK_PERIOD_MS);

    if(isnan(h) or isnan(t)){h = 0;
    t= 0;
    Serial.println("error dht");}
   
    //Formatage des donnxes en strings
    sprintf(strT,"% 3i",int(t));
    sprintf(strH,"% 3i",int(h));
    sprintf(strL,"% 3i",int(l));  
    
    
    vTaskResume(taskprocess); //Appel tache data process   
    vTaskSuspend(tasklect);   //Fin tache
  }
  }

void task_dataProcess(void * arg){
/*
Tache qui envoie les donnxes au mqtt et qui les affiche sur l'OLED
*/
for(;;){

  Serial.print("Publish message: ");

  //Envoi des donnxes au mqtt
  client.publish("esp2/humidite",strH);
  client.publish("esp2/temperature",strT);
  client.publish("esp2/luminosite",strL);

  //Afichage des donnxes envoyxes sur le port serie
  Serial.print(" hum = ");
  Serial.print(strH);
  Serial.print(" temp = ");
  Serial.print(strT);
  Serial.print(" lum = ");
  Serial.print(strL);
  Serial.print(" ");
  Serial.println(" ");  

  
  //Affichage OLED
  ecranOLED.clearDisplay();
  ecranOLED.setTextSize(2);         // Taille des caractères (1:1, puis 2:1, puis 3:1)
  ecranOLED.setCursor(0, 0); 
  ecranOLED.setTextColor(SSD1306_BLACK,SSD1306_WHITE); 
  ecranOLED.println("   ESP2   "); 
  ecranOLED.setTextColor(SSD1306_WHITE, SSD1306_BLACK); 
  ecranOLED.print("Temp:"); 
  ecranOLED.print(strT);
  ecranOLED.println("*C"); 
  ecranOLED.print("Hum :"); 
  ecranOLED.print(strH);
  ecranOLED.println("%");
  ecranOLED.print("Lum :");  
  ecranOLED.print(strL);
  ecranOLED.println("%");
  ecranOLED.display(); 

  //Appel tache lum
  vTaskResume(tasklum);

  //Fin tache
  vTaskSuspend(taskprocess);
}

}

void task_lum(void * arg){
  /*
  Tache qui allume l'eclairage connecte a l'esp si la luminosite exterieure est inssufisante 
  */
  for(;;){
  if (l < 10){
    digitalWrite((int)arg,HIGH);
  }
  else{
    digitalWrite((int)arg,LOW);
  }

  //Fin de tache
  vTaskSuspend(tasklum);
}}

void setup(){
  //Reglage modes des pins utilises
  pinMode(ledbp, OUTPUT);
  pinMode(BP,INPUT);
  pinMode(LumPin,INPUT);
  pinMode(ledLum,OUTPUT);
  pinMode(ledOnOff,OUTPUT);

  digitalWrite(ledbp, ledbpState);
  digitalWrite(ledLum,LOW);

  //Connexion et demmarage ecran OLED
  if(!ecranOLED.begin(SSD1306_SWITCHCAPVCC, adresseI2CecranOLED))
  while(1);  

  Serial.begin(115200); //Demmarage port serie

  setup_wifi();  //Connexion Wifi

  //Connexion serveur mqtt
  client.setServer(mqtt_server,mqtt_port);
  client.setCallback(callback);
  
  if (!client.connected()) { //Attends que la connexion au mqtt soit etablie
    reconnect();
    client.loop(); 
  }

  //Creation et susprension des taches
  xTaskCreatePinnedToCore(task_lectdata,"lectdata",8000,(void*)LumPin,1,&tasklect,NULL); 
  vTaskSuspend(tasklect);
 
  xTaskCreatePinnedToCore(task_dataProcess,"process",8000,NULL,1,&taskprocess,1);
  vTaskSuspend(taskprocess);

  xTaskCreatePinnedToCore(task_lum,"Lum",8000,(void*)ledLum,1,&tasklum,NULL);
  vTaskSuspend(tasklum);

  //Demmarage dht
  dht.begin();

}

void loop(){

  if (OnOff){
    if (!client.connected()) { //Reconnexion au mqtt si deconnexion
    reconnect();
    client.loop();}

    if(!digitalRead(ledOnOff)){
    digitalWrite(ledOnOff,HIGH);}

    bpState = digitalRead(BP); //Lecture etat bouton BP
      if(bpState != lastState){
        if(bpState){
            digitalWrite(ledbp,HIGH);
            lectState = 1; //Flag tache lect active
        }
        else{
            digitalWrite(ledbp,LOW);  
        }
        lastState = bpState;
        Serial.println("BP State");
        Serial.println(lectState);
      }
      

  unsigned long now = millis();
  if (now - lastMsg > 12000) { //Si 12 sec sont passees depuis la derniere lecture de donnees ou le demmarage

    if(eTaskGetState(tasklect) == eSuspended){
      vTaskResume(tasklect); }  //Appel de la tache lect
     lastMsg = now;   
    }
    
  if (lectState){ //Si un appui de BP a ete detecte
    if(eTaskGetState(tasklect) == eSuspended){
    vTaskResume(tasklect); //Appel de la tache lect
     lectState = 0;
    }
  }
  }
  else{
    //Si OFF

    //Suspention de toutes les taches
    vTaskSuspend(tasklect);
    vTaskSuspend(taskprocess);
    vTaskSuspend(tasklum);

    //Extrinction de toutes les leds
    digitalWrite(ledbp,LOW);
    digitalWrite(ledLum,LOW);
    digitalWrite(ledOnOff,LOW);
  }
} 