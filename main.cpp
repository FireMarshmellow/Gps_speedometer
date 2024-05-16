#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED display settings
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Create an OLED display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// Use UART1 for the GPS module
HardwareSerial GPSModule(1);

// Button setup
#define BUTTON_PIN 7
int screenMode = 0; // 0 - All info, 1 - MPH only, 2 - KMH only, 3 - Blank screen
int lastButtonState = LOW;          // Last known state of the button
unsigned long lastDebounceTime = 0;  // Last time the button was toggled
unsigned long debounceDelay = 50;    // Debounce delay in milliseconds


// Define global variables to hold GPS data
String timeGPS, latitude, longitude, fixQuality, numSatellites;
float speedMph = 0.0;  // Default to 0.0 MPH if no speed data is received

void setup() {
  Serial.begin(115200);  // Start the serial communication with your computer
  GPSModule.begin(9600, SERIAL_8N1, 16, 17);  // Start GPS module communication; replace 16, 17 with actual pins
  
  pinMode(BUTTON_PIN, INPUT);  // Set up the button pin without pull-up

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(F("Mellow_labs"));
  display.display();
  delay(2000); // Pause for 2 seconds

  Serial.println("GPS Module Testing");
}

void loop() {
  bool dataUpdated = false;  // Flag to check if new data has been parsed

  // Read data from the GPS module
  if (GPSModule.available()) {
    String nmeaSentence = GPSModule.readStringUntil('\n');

    // Decode sentences and update data
    if (nmeaSentence.startsWith("$GPGGA,")) {
      decodeGPGGA(nmeaSentence);
      dataUpdated = true;
    } else if (nmeaSentence.startsWith("$GPVTG,")) {
      decodeGPVTG(nmeaSentence);
      dataUpdated = true;
    }
  }

  // If new data has been parsed, print it and update OLED
  if (dataUpdated) {
    printGPSData();
    updateOLED();
  }


  // Your existing GPS data processing code...

  // Handle button press with debouncing
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (reading == LOW) {  // Check for button press (LOW when pressed in pull-up configuration)
        screenMode = (screenMode + 1) % 5;  // Change screen mode
        updateOLED();  // Update the OLED display based on new mode
        Serial.println("Button pressed - Mode changed");
      }
      lastDebounceTime = millis(); // Reset the debounce timer
    }
  }
  lastButtonState = reading;  // Store the current reading to check against next loop
}

void decodeGPGGA(String sentence) {
  int startIndex = 0;
  int endIndex = 0;
  String data[15]; // GPGGA has 15 pieces of information
  int i = 0;

  while ((endIndex = sentence.indexOf(',', startIndex)) > 0 && i < 15) {
    data[i] = sentence.substring(startIndex, endIndex);
    startIndex = endIndex + 1;
    i++;
  }
  data[i] = sentence.substring(startIndex); // Catch any remaining data after the last comma

  timeGPS = data[1];
  latitude = data[2] + " " + data[3];
  longitude = data[4] + " " + data[5];
  fixQuality = data[6];
  numSatellites = data[7];
}

void decodeGPVTG(String sentence) {
  int startIndex = 0;
  int endIndex = 0;
  String data[10]; // GPVTG has up to 10 pieces of information
  int i = 0;

  while ((endIndex = sentence.indexOf(',', startIndex)) > 0 && i < 10) {
    data[i] = sentence.substring(startIndex, endIndex);
    startIndex = endIndex + 1;
    i++;
  }
  data[i] = sentence.substring(startIndex);

  float speedKnots = data[5].toFloat();
  speedMph = speedKnots * 1.15078; // Convert knots to MPH
}

void printGPSData() {
  Serial.println("Time: " + timeGPS);
  Serial.println("Latitude: " + latitude);
  Serial.println("Longitude: " + longitude);
  Serial.println("Fix quality: " + fixQuality);
  Serial.println("Number of satellites: " + numSatellites);
  Serial.println("Speed: " + String(speedMph, 2) + " MPH");
}

String formatGPSTime(String rawTime) {
  // GPS time comes in HHMMSS format
  if (rawTime.length() < 6) {
    return ""; // Return empty if the time string is too short
  }
  String formattedTime = rawTime.substring(0, 2) + ":" + // Hours
                         rawTime.substring(2, 4) + ":" + // Minutes
                         rawTime.substring(4, 6);        // Seconds
  return formattedTime;
}


void updateOLED() {
  display.clearDisplay();
  display.setCursor(0,0);

  switch(screenMode) {
    case 0: { // All information
      float speedKmh = speedMph * 1.60934;
      String formattedTime = formatGPSTime(timeGPS);
      display.println(String(speedMph, 2) + " MPH");
      display.println(String(speedKmh, 2) + " km/h");
      display.println("Sats: " + numSatellites);
      display.println(formattedTime);
      break;
    }
    case 1: { // MPH only, larger font
      display.setTextSize(3);
      display.println(String(speedMph, 2) + "Mph");
      display.setTextSize(1); // Reset font size for next usage
      break;
    }
    case 2: { // KMH only, larger font
      display.setTextSize(3);
      float speedKmhOnly = speedMph * 1.60934;
      display.println(String(speedKmhOnly, 2) + "Km");
      display.setTextSize(1);
      break;
    }
    case 3: { // KMH only, larger font
      display.setTextSize(2);
      display.println(String("Mellow_labs"));
      display.setTextSize(1);
      break;
    }
    case 4: // Blank screen
      // Do not draw anything
      break;
  }

  display.display();  // Actually display all of the above
}

