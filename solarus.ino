ignore pls 
#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>
#include <ESP32Servo.h>
#include <time.h>

// Configuración de WiFi y Firebase
#define WIFI_SSID "ESP32"
#define WIFI_PASSWORD "1234567890"
#define DATABASE_URL "https://nuevoiot-e7f97-default-rtdb.firebaseio.com"
#define DATABASE_SECRET "AIzaSyB1iskFoDhNmdFPig6Eje1DE_IcH9K0dyo"

// Pines de las LDR y servos
const int LDR1 = 34; // naranja
const int LDR2 = 35; // morado
const int LDR3 = 32; // azul
const int LDR4 = 33; // amarillo
const int servoPin = 26; // Servo principal (dirección horizontal)
const int servoPin2 = 27; // Servo secundario (inclinación)

Servo myServo;
Servo myServo2;
int servoPos = 0; // Posición inicial del servo principal
int servoTilt = 90; // Posición inicial del servo secundario (neutral)
int lastServoPos = -1; 
int lastServoTilt = -1;

// Posiciones angulares de cada LDR
const int positions[] = {0, 45, 90, 135};

// Firebase setup
WiFiClientSecure ssl;
DefaultNetwork network;
AsyncClientClass client(ssl, getNetwork(network));
FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result;
LegacyToken dbSecret(DATABASE_SECRET);

void printError(int code, const String &msg) {
    Serial.printf("Error, msg: %s, code: %d\n", msg.c_str(), code);
}

void setup() {
    Serial.begin(115200);

    // Inicializar servos
    myServo.attach(servoPin);
    myServo2.attach(servoPin2);
    myServo.write(servoPos); 
    myServo2.write(servoTilt);
    delay(1000);

    // Configuración de los pines de las LDR
    pinMode(LDR1, INPUT);
    pinMode(LDR2, INPUT);
    pinMode(LDR3, INPUT);
    pinMode(LDR4, INPUT);

    // Configuración de red WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    // Configuración de Firebase
    Serial.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    ssl.setInsecure();

    // Inicializar Firebase
    initializeApp(client, app, getAuth(dbSecret));
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);
    client.setAsyncResult(result);
}

void loop() {
    // Leer los valores de las LDR
    int ldrValues[4] = {
        analogRead(LDR1),
        analogRead(LDR2),
        analogRead(LDR3),
        analogRead(LDR4)
    };

    // Identificar las dos LDR con los valores más altos
    int max1Index = 0, max2Index = -1;
    for (int i = 1; i < 4; i++) {
        if (ldrValues[i] > ldrValues[max1Index]) {
            max2Index = max1Index;
            max1Index = i;
        } else if (max2Index == -1 || ldrValues[i] > ldrValues[max2Index]) {
            max2Index = i;
        }
    }

    // Calcular la posición del servo principal
    if (max2Index != -1 && abs(ldrValues[max1Index] - ldrValues[max2Index]) < 50) {
        servoPos = (positions[max1Index] + positions[max2Index]) / 2;
    } else {
        servoPos = positions[max1Index];
    }

    // Calcular la inclinación del segundo servo
    int maxDifference = abs(ldrValues[max1Index] - (max2Index != -1 ? ldrValues[max2Index] : 0));
    servoTilt = 90 + (maxDifference > 100 ? 30 : (maxDifference > 50 ? 15 : 0));
    if (ldrValues[max2Index] > ldrValues[max1Index]) {
        servoTilt = 90 - (servoTilt - 90); // Invertir inclinación
    }

    // Mover los servos si hay cambios
    if (servoPos != lastServoPos) {
        myServo.write(servoPos);
        lastServoPos = servoPos;
        Serial.printf("Servo (horizontal) moved to position: %d degrees\n", servoPos);
    }

    if (servoTilt != lastServoTilt) {
        myServo2.write(servoTilt);
        lastServoTilt = servoTilt;
        Serial.printf("Servo (tilt) moved to position: %d degrees\n", servoTilt);
    }

    // Mostrar los valores de las LDR en el monitor serie
    Serial.printf("LDR1: %d, LDR2: %d, LDR3: %d, LDR4: %d\n", ldrValues[0], ldrValues[1], ldrValues[2], ldrValues[3]);

    // Enviar datos a Firebase
    time_t now = time(nullptr);
    String jsonStr = "{\"timestamp\":" + String(now) +
                     ",\"ldr1\":" + String(ldrValues[0]) +
                     ",\"ldr2\":" + String(ldrValues[1]) +
                     ",\"ldr3\":" + String(ldrValues[2]) +
                     ",\"ldr4\":" + String(ldrValues[3]) + "}";

    Serial.print("Pushing data... ");
    String name = Database.push<object_t>(client, "/sensor_readings", object_t(jsonStr.c_str()));
    
    if (client.lastError().code() == 0) {
        Serial.printf("Success! Push name: %s\n", name.c_str());
        Serial.println(jsonStr);
    } else {
        printError(client.lastError().code(), client.lastError().message());
    }

    delay(500);  // Retardo antes de la siguiente lectura
}