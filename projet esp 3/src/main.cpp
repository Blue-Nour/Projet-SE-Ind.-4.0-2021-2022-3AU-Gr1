
#include <Arduino.h>
#include <Tone32.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <SPI.h>

#include <Adafruit_SSD1306.h>

#include <PubSubClient.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <WiFi.h>

 





 //

#define nombreDePixelsEnLargeur 128         // Taille de l'écran OLED, en pixel, au niveau de sa largeur
#define nombreDePixelsEnHauteur 64          // Taille de l'écran OLED, en pixel, au niveau de sa hauteur
#define brocheResetOLED         -1          // Reset de l'OLED partagé avec l'Arduino (d'où la valeur à -1, et non un numéro de pin)
#define adresseI2CecranOLED     0x3C        // Adresse de "mon" écran OLED sur le bus i2c (généralement égal à 0x3C ou 0x3D)

Adafruit_SSD1306 ecranOLED(nombreDePixelsEnLargeur, nombreDePixelsEnHauteur, &Wire, brocheResetOLED);

int bp_1_Pin  = 18 ; // stop buzzer
int bp_2_Pin  = 19 ; // on/off
int bp1State = 0 ;
int bp2State = 0 ; 
int bp1Old = 0; 
int bp2Old = 0;
int bp2value = 0 ; //toggle  I/O
int bp1value = 0 ; 
boolean bCouleurInverse = false;


int buzzer_Pin  = 23;
int oled_slc_Pin = 22; 
int oled_sda_Pin = 21;
int led_1_Pin = 15;
int led_2_Pin = 2;

TaskHandle_t task1_handle  = NULL ; 
TaskHandle_t task2_handle  = NULL ; 
TaskHandle_t task3_handle  = NULL ; 


//var  esp 1/2 
int temperature1 = 0 ;int humidite1 = 0 ; int luminosite1 = 0 ;
int temperature2 = 0 ; int humidite2 = 0 ; int luminosite2 = 0 ;
int temperature1old = 0 ;int humidite1old = 0 ; int luminosite1old = 0 ;
int temperature2old = 0 ; int humidite2old = 0 ; int luminosite2old = 0 ;

//setup connection mqtt

const char* ssid = "NOURA";
const char* password = "N1999zaru";

const char* mqtt_server = "192.168.231.73"; //ou 1

int R_led, G_led, B_led, currentTime, previousTime;

WiFiClient espClient;
PubSubClient client(espClient);

//192.168.0.199
void wificonfig(){
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}



//bp 2 toggle ON/OFF
void task1_BP2_OnOff(void *parameters){
    for(;;){
        //Serial.println("Task 1 btn ");
        //Serial.print(bp2value);
        //Serial.print(digitalRead(bp_2_Pin));
        if (digitalRead(bp_2_Pin)==HIGH){
            if (bp2Old==0){ // détéction de front montant 
                bp2Old = 1 ; 
                Serial.println("on ");
                bp2value = (bp2value+1)%2; // toggle
                digitalWrite(led_2_Pin,bp2value);
                //Serial.println(bp2value);
               }
        }
        else{bp2Old = 0;}
        
        vTaskDelay(50/portTICK_PERIOD_MS);
         
         
        if (bp2value == 0) {
            vTaskSuspend(task2_handle);
            //Serial.println("Task 1 suspend");
            tone(buzzer_Pin,1,1,0);
            //Serial.println("Task 1 tone");
            digitalWrite(led_1_Pin,LOW);
            //Serial.println("Task 1 led 1");

        }
        //Serial.print("fin task 1");
    }
}

// mode urgence buzzer et led  1Hz
void task2_buzzer(void *parameters){
    for(;;){
        //Serial.println("Task 2  ");
        vTaskDelay(100/portTICK_PERIOD_MS); 
        digitalWrite(led_1_Pin,HIGH);
        
        //tone(buzzer_Pin,523,1000,0);
        unsigned int frequency = 440 ;
        unsigned long duration = 100;
        int channel = 1 ;
         if (ledcRead(channel)) {
        log_e("Tone channel %d is already in use", ledcRead(channel));
        return;
        }
        ledcAttachPin(buzzer_Pin, channel);
        ledcWriteTone(channel, frequency);
        if (duration) {
            vTaskDelay(duration/portTICK_PERIOD_MS); 
            noTone(buzzer_Pin, channel);
        }  
        digitalWrite(led_1_Pin,LOW);
        //Serial.println("Task 2 fin  ");

}
}


//bp stop alarm
void task3_bp_buzzer(void *parameters){
    for(;;){
        //Serial.println("Task 3 btn ");
        //Serial.print(bp1value);
        //Serial.print(digitalRead(bp_1_Pin));
        if (digitalRead(bp_1_Pin)==HIGH){
            if (bp1Old==0){ // détéction de front montant 
                bp1Old = 1 ; 
                Serial.println("on ");
                bp1value = (bp1value+1)%2; // toggle
                
                if (bp1value ==1){
                    vTaskResume(task2_handle);
                    }
                else {vTaskSuspend(task2_handle);
                tone(buzzer_Pin,0,0,0);
                digitalWrite(led_1_Pin,LOW);
                }
                
                //Serial.println(bp1value);
               }
        } 
        else{bp1Old = 0;}
        vTaskDelay(50/portTICK_PERIOD_MS);
        //Serial.println("Task 3 fin ");
    }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp1_temperature");
      client.subscribe("esp1/humidite");
      client.subscribe("esp1/luminosite");
      client.subscribe("esp2/temperature");
      client.subscribe("esp2/humidite");
      client.subscribe("esp2/luminosite");
      Serial.println("lkjldkjqd");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// reception 
void callback(char* topic, byte* message, unsigned int length) {
    
    Serial.print("Message arrived on topic: ");
    ///////////////////////////////////////////////////////////////////
    if (bp2value == 1 ){////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            Serial.print("Message arrived on topic: ");
            Serial.print(topic);
            Serial.print(". Message: ");
            String messageTemp;
            for (int i = 0; i < length; i++) {
                Serial.print((char)message[i]);
                messageTemp += (char)message[i];
            }
            Serial.println(" for temp  ");
            Serial.println();
            if (String(topic) == "esp1_temperature") {
                Serial.print("Changing output to ");
                temperature1 = messageTemp.toInt();  

            }
            if (String(topic) == "esp1/humidite") {
                Serial.print("Changing output to ");
                humidite1 = messageTemp.toInt();
            }
            if (String(topic) == "esp1/luminosite") {
                Serial.print("Changing output to ");
                luminosite1 = messageTemp.toInt();
            }
            if (String(topic) == "esp2/temperature") {
                Serial.print("Changing output to ");
                temperature2 = messageTemp.toInt();  
            }
            if (String(topic) == "esp2/humidite") {
                Serial.print("Changing output to ");
                humidite2 = messageTemp.toInt();
            }
            if (String(topic) == "esp2/luminosite") {
                Serial.print("Changing output to ");
                luminosite2 = messageTemp.toInt();
            }
            int tempbelow = 0 ; 
            int tempup = 30 ; 
            int lumibelow = 0;
            int lumiup =30 ;
            int humidbelow = 0 ;
            int humidup = 30 ;
            Serial.println(" for temp 111111 ");
            if ( temperature1 != temperature1old || luminosite1 != luminosite1old || humidite1 !=  humidite1old || temperature2 != temperature2old || humidite2 != humidite2old || luminosite2 != luminosite2old ){
                // verification urgence et alarme

                if (temperature1 < tempbelow ||  temperature2 < tempbelow || temperature1 >tempup || temperature2 > tempup ){
                vTaskResume(task2_handle);
                }  
                if (humidite1 < humidbelow ||  humidite2 < humidbelow || humidite1 >humidup || humidite2 > humidup ){
                vTaskResume(task2_handle);
                }  
                if (luminosite1 < lumibelow ||  luminosite2 < lumibelow || luminosite1 >lumiup || luminosite2 > lumiup ){
                vTaskResume(task2_handle);
                }  
                //128x64
                //display.drawRoundRect(10, 10, 30, 50, 2, WHITE);
                //display.drawRoundRect(10, 10, 30, 50, 2, WHITE);
                //affichage oled
               
                ecranOLED.setTextColor(SSD1306_WHITE, SSD1306_BLACK); 
                // 10 caract par ligne en size 2 
                 /*
                ecranOLED.print("app1 |");
                ecranOLED.println("app2");
                ecranOLED.print("21*C |");
                ecranOLED.println("10*C");
                ecranOLED.print("50%  |");
                ecranOLED.println("100%");
                ecranOLED.print("100W |");
                ecranOLED.println("400W");
                */
                ecranOLED.clearDisplay();
                ecranOLED.setTextSize(2.5);                   // Taille des caractères (1:1, puis 2:1, puis 3:1)
                ecranOLED.setCursor(0, 0);  
                ecranOLED.print("app1 |");
                ecranOLED.println("app2");
                char txt[12];
                char txt2[12] ;
                char txt3[12] ;
               
                sprintf(txt,"%4dC|%3dC", temperature1, temperature2);
                sprintf(txt2,"%4d |%3d" ,humidite1,humidite2);
                sprintf(txt3,"%4d |%3d", luminosite1, luminosite2);
                Serial.print(txt);
                ecranOLED.println(txt);
                
                ecranOLED.setCursor(0,32);  
                ecranOLED.println(txt2);
                ecranOLED.setCursor(0,48);  
                ecranOLED.println(txt3);
                ecranOLED.setCursor(50,32);  
                ecranOLED.print("%");
                ecranOLED.setCursor(110,48);  
                ecranOLED.print("%");
                ecranOLED.setCursor(110,32);  
                ecranOLED.print("%");
                ecranOLED.setCursor(50,48);  
                ecranOLED.print("%");
                //lux en % car pas de place
                
                ecranOLED.display(); 
               
                delay(100);
                Serial.println(" for temp2222222");
            }      
}  
    }
 

void setup() {


  Serial.begin(9600);
  pinMode(led_1_Pin, OUTPUT);
  pinMode(led_2_Pin,OUTPUT);
  pinMode(bp_1_Pin,INPUT);
  pinMode(bp_2_Pin,INPUT);
  

// wifi 
 wificonfig();
 client.setServer(mqtt_server, 1883);
 client.setCallback(callback);
 reconnect();

     // Initialisation de l'écran OLED
    if(!ecranOLED.begin(SSD1306_SWITCHCAPVCC, adresseI2CecranOLED))
        while(1);                               // Arrêt du programme (boucle infinie) si échec d'initialisation

    ecranOLED.clearDisplay();                                   // Effaçage de l'intégralité du buffer
    ecranOLED.setTextSize(2.5);                   // Taille des caractères (1:1, puis 2:1, puis 3:1)
    ecranOLED.setCursor(0, 0);  

  xTaskCreate(
          task1_BP2_OnOff,//nom de la fonction
          "task 1 on off",// nom de la tache 
          1000,//taille aloue a notre tache 
          NULL,//parameters task
          2,//priorité de la tache 
          &task1_handle // gestion de tache 
      );
  xTaskCreate(
        task2_buzzer,//nom de la fonction
        "task 2 buzzer",// nom de la tache 
        1000,//taille aloue a notre tache 
        NULL,//parameters task
        2,//priorité de la tache 
        &task2_handle // gestion de tache 
    );
xTaskCreate(
        task3_bp_buzzer,//nom de la fonction
        "task 3 bp 1 urg",// nom de la tache 
        1000,//taille aloue a notre tache 
        NULL,//parameters task
        2,//priorité de la tache 
        &task3_handle // gestion de tache 
    );

Serial.print("fin de setup");
}



void loop() {
client.loop();
if (!client.connected()) {
    reconnect();
    }




} 