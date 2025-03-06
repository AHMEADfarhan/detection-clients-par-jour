#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>


#define ENTRY_SENSOR_PIN D4  // Capteur d'entrée (GPIO2)
#define EXIT_SENSOR_PIN D5   // Capteur de sortie (GPIO14)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1


// Informations Wi-Fi
const char* ssid = "FairFix.fr";
const char* password = "fairfixfairfixfairfix";


// Informations du Broker MQTT
const char* mqttServer = "192.168.1.124";
const int mqttPort = 1883;
const char* mqttUser = "mqtt";
const char* mqttPassword = "Azerty93";
const char* mqttTopicTotal = "client/total";  // Topic du nombre total de clients


WiFiClient espClient;
PubSubClient client(espClient);


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


int clientCount = 0;         // Nombre total de clients dans le magasin
int lastEntryState = LOW;    // Dernier état du capteur d'entrée
int lastExitState = LOW;     // Dernier état du capteur de sortie
unsigned long debounceDelay = 500;
unsigned long lastEntryTime = 0;
unsigned long lastExitTime = 0;


void setup() {
  Serial.begin(115200);


  pinMode(ENTRY_SENSOR_PIN, INPUT);
  pinMode(EXIT_SENSOR_PIN, INPUT);


  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Échec de l'initialisation de l'écran OLED"));
    while (true);
  }


  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Connexion Wi-Fi...");
  display.display();


  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connecté au Wi-Fi!");


  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Wi-Fi Connecte!");
  display.setCursor(0, 10);
  display.print("IP: ");
  display.print(WiFi.localIP());
  display.display();
  delay(2000);


  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);


  while (!client.connected()) {
    Serial.print("Connexion au serveur MQTT...");
    if (client.connect("ESP8266Client", mqttUser, mqttPassword)) {
      Serial.println("connecte");
    } else {
      Serial.print("Échec, état=");
      Serial.println(client.state());
      delay(2000);
    }
  }


  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Pret a detecter...");
  display.display();
}


void loop() {
  int entryState = digitalRead(ENTRY_SENSOR_PIN);
  int exitState = digitalRead(EXIT_SENSOR_PIN);


  // Détection des entrées
  if (entryState == HIGH && lastEntryState == LOW && (millis() - lastEntryTime > debounceDelay)) {
    clientCount++;  // Incrémentation du compteur
    lastEntryTime = millis();
    lastEntryState = HIGH;


    // Mise à jour de l'affichage et publication du total
    updateDisplay();
    publishTotal();
  } else if (entryState == LOW && lastEntryState == HIGH) {
    lastEntryState = LOW;
  }


  // Détection des sorties
  if (exitState == HIGH && lastExitState == LOW && (millis() - lastExitTime > debounceDelay)) {
    clientCount--;  // Décrémentation du compteur
    lastExitTime = millis();
    lastExitState = HIGH;


    // Mise à jour de l'affichage et publication du total
    updateDisplay();
    publishTotal();
  } else if (exitState == LOW && lastExitState == HIGH) {
    lastExitState = LOW;
  }


  client.loop();
  delay(50);
}


void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Clients dans le magasin: ");
  display.print(clientCount);
  display.display();
}


void publishTotal() {
  String total = String(clientCount);
  client.publish(mqttTopicTotal, total.c_str());  // Publication uniquement du nombre total de clients
}


void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message recu: ");
  Serial.print(topic);
  Serial.print(" Message: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
