#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include <DHT.h>
#include <I2S.h>

// Replace with your WiFi credentials
const char* ssid = "sunbird";
const char* password = "ltd@sunbird.ai";

// ThingSpeak settings
unsigned long myChannelNumber = 2777390; // Replace with your channel ID
const char* myWriteAPIKey = "G8506CVEV46OW4ST"; // Replace with your Write API Key

WiFiClient client;

// DHT settings
#define DHTPIN D4 // GPIO2
#define DHTTYPE DHT11 // DHT 11 sensor
DHT dht(DHTPIN, DHTTYPE);

// LED pins
#define RED_LED D1   // GPIO5
#define GREEN_LED D2 // GPIO4
#define BLUE_LED D3  // GPIO0

// I2S settings
#define I2S_WS D1    // GPIO5
#define I2S_SCK D2   // GPIO4
#define I2S_SD D7    // GPIO13

void setup() {
  Serial.begin(115200);
  delay(10);

  // Initialize DHT sensor
  dht.begin();

  // Initialize LEDs as outputs
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  // Turn off all LEDs initially
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BLUE_LED, LOW);

  // Initialize I2S
  // I2S.setPins(I2S_SCK, I2S_WS, I2S_SD);
  
  I2S.begin(I2S_PHILIPS_MODE, 16000, 32); // Sampling rate: 16 kHz, Bits: 32

  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // Initialize ThingSpeak
  ThingSpeak.begin(client);
}

float readSoundLevel() {
  int sampleCount = 0;
  long sum = 0;

  while (sampleCount < 1000) { // Collect 1000 samples
    int32_t sample = I2S.read(); // Read I2S sample
    sum += abs(sample); // Accumulate absolute values
    sampleCount++;
  }

  // Calculate average sound level
  return (float)sum / sampleCount;
}

void loop() {
  // Read temperature and humidity from DHT11
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Read sound level from I2S sensor
  float soundLevel = readSoundLevel();

  // Check if readings are valid
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Print values to Serial Monitor
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
  Serial.print("Sound Level: ");
  Serial.print(soundLevel);
  Serial.println(" dB");

  // LED Control
  if (temperature > 25) {
    // High temperature
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
  } else if (temperature < 20) {
    // Low temperature
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BLUE_LED, HIGH);
  } else {
    // Moderate temperature
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(BLUE_LED, LOW);
  }

  // Send data to ThingSpeak
  ThingSpeak.setField(1, temperature); // Field 1: Temperature
  ThingSpeak.setField(2, humidity);    // Field 2: Humidity
  ThingSpeak.setField(3, soundLevel);  // Field 3: Sound Level
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  if (x == 200) {
    Serial.println("Data successfully sent to ThingSpeak!");
  } else {
    Serial.println("Error sending data to ThingSpeak");
  }

  // Wait 20 seconds before sending next data (as per ThingSpeak rate limit)
  delay(20000);
}
