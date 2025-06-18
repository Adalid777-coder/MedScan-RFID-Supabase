#include <WiFi.h>
#include <HTTPClient.h>
#include <MFRC522.h>
#include <SPI.h>

// Configuración de pines RFID
#define SS_PIN 17    // GPIO17 (conectado a SDA/SS del MFRC522)
#define RST_PIN 22   // GPIO22 (conectado a RST del MFRC522)

// Credenciales de red y Supabase (actualizadas)
const char* ssid = "MC0L";
const char* password = "j0su2262";
const String supabaseUrl = "https://wtyxrzmwpzpgteohlssv.supabase.co";
const String apiKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Ind0eXhyem13cHpwZ3Rlb2hsc3N2Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDk1OTQ3MzksImV4cCI6MjA2NTE3MDczOX0.r1Krnw4vQIjmZa7KS_Ka3QRrgC9CCZoP_Xly6R1Yk9s";

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

void setup() {
  Serial.begin(115200);
  
  // Inicializar RFID con configuración SPI actualizada
  SPI.begin(18, 19, 23, SS_PIN); // SCK=18, MISO=19, MOSI=23, SS=17
  SPI.setFrequency(1000000); // Frecuencia a 1MHz
  rfid.PCD_Init();
  
  // Inicializa la clave por defecto (0xFF)
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  
  // Conectar a WiFi
  conectarWiFi();
}

void loop() {
  // Verificar conexión WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado. Reconectando...");
    conectarWiFi();
    delay(1000);
    return;
  }

  // Leer RFID
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String tagID = obtenerIDRFID();
    Serial.println("Tag RFID escaneado: " + tagID);
    
    // Consultar Supabase
    consultarSupabase(tagID);
    
    // Pausa para evitar múltiples lecturas
    delay(2000);
  }
}

String obtenerIDRFID() {
  String tagID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    // Asegurar formato de 2 dígitos hexadecimales
    if (rfid.uid.uidByte[i] < 0x10) tagID += "0";
    tagID += String(rfid.uid.uidByte[i], HEX);
  }
  tagID.toUpperCase(); // Convertir a mayúsculas para consistencia
  rfid.PICC_HaltA(); // Detener comunicación con el tag
  rfid.PCD_StopCrypto1(); // Detener cifrado
  return tagID;
}

void conectarWiFi() {
  Serial.print("Conectando a ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 15) {
    delay(500);
    Serial.print(".");
    intentos++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nError al conectar WiFi");
    Serial.println("Por favor verifica:");
    Serial.println("1. Que el nombre de la red (SSID) sea correcto");
    Serial.println("2. Que la contraseña sea correcta");
    Serial.println("3. Que el router esté encendido y funcionando");
    Serial.println("4. Que el ESP32 esté dentro del alcance de la red WiFi");
  }
}

void consultarSupabase(String tagID) {
  HTTPClient http;
  
  // Construir URL con parámetro codificado
  String url = supabaseUrl + "/rest/v1/pacientes?select=nombre,edad&rfid_id=eq." + tagID;
  
  Serial.println("Consultando: " + url);
  
  http.begin(url);
  http.addHeader("apikey", apiKey);
  http.addHeader("Authorization", "Bearer " + apiKey);
  http.addHeader("Content-Type", "application/json");
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Respuesta:");
    Serial.println(payload);
    
    // Aquí podrías procesar la respuesta JSON si necesitas datos específicos
    
  } else if (httpCode == HTTP_CODE_NOT_FOUND) {
    Serial.println("Paciente no encontrado");
  } else {
    Serial.print("Error en la consulta. Código: ");
    Serial.println(httpCode);
    Serial.println("Respuesta: " + http.getString());
  }
  
  http.end();
}
