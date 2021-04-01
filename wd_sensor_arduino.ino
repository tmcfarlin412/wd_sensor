/**
 * Vibration sensor
 * 
 * When the button is pressed, listen for vibrations for a 
 * predetermined amount of time. Send those vibration readings
 * to a server. An LED light is used to display feedback to user.
 */
include "Conf.h"
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include "pitches.h"

#define MINUTE_IN_MILLIS 60000
#define SECOND_IN_MILLIS 1000

#define LOGGING_DEBUG true

const int INPUT_PIEZO = A0;
const int INPUT_BUTTON = 9;
const int OUTPUT_LED = 12;  // LED for feedback to user
const int OUTPUT_BUZZER = 10;  // LED for feedback to user

const int STATE_WAITING_FOR_BUTTON_PRESS = 0;
const int STATE_READING_PIEZO = 1;

const int DELAY_BETWEEN_BUTTON_READINGS_MILLIS = 5 * SECOND_IN_MILLIS;
const int DELAY_BETWEEN_PIEZO_READINGS_MILLIS = 2 * MINUTE_IN_MILLIS;

// Assumes that no cycle will take longer than 3 hours (soak + wash, etc.)
const int DURATION_READ_PIEZO_MILLIS = 180 * MINUTE_IN_MILLIS;

const long UTC_OFFSET_SECONDS = 0 * 60 * 60;  // Chicago UTC/GMT -6 hours

int MELODY_START[] = {
  NOTE_A7, NOTE_C7, NOTE_C7, NOTE_A7, NOTE_C7, NOTE_C7
};

int MELODY_START_DURATIONS[] = {
  400, 400, 800, 400, 400, 800
};

int MELODY_START_DELAYS[] = {
  400, 400, 800, 400, 400, 800
};

const int MELODY_START_NUMBER_OF_NOTES = sizeof(MELODY_START) / sizeof(MELODY_START[0]);

int MELODY_DONE[] = {
  NOTE_A7, NOTE_A7, NOTE_A7, NOTE_A7, NOTE_A7, NOTE_A7, NOTE_A7, NOTE_A7
};

int MELODY_DONE_DURATIONS[] = {
  400, 1200, 400, 1200, 400, 1200, 400, 1200
};

int MELODY_DONE_DELAYS[] = {
  400, 1600, 400, 1600, 400, 1600, 400, 1600
};

const int MELODY_DONE_NUMBER_OF_NOTES = sizeof(MELODY_DONE) / sizeof(MELODY_DONE[0]);

String uid = "";
String base_path = "";
int piezoReading;
int buttonReading = LOW;
int state = STATE_WAITING_FOR_BUTTON_PRESS;
long currentTime;
long stopReadingPiezoTime;
bool isReadingPiezoDone;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

void setup() {
  pinMode(INPUT_BUTTON, INPUT);
  pinMode(OUTPUT_LED, OUTPUT);

  // Connect to wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  logd("Connecting...");
  while (WiFi.status() != WL_CONNECTED) { 
    logd(".");
    delay(500); 
  } 
  logd("Connected: ");
  logd(WiFi.localIP().toString());

  // Setup NTP client
  timeClient.begin();

  // Firebase setup
  logd("Connecting to Firebase...");
  
  // Define the FirebaseAuth data for authentication data
  FirebaseAuth auth;
  
  // Define the FirebaseConfig data for config data
  FirebaseConfig config;
  
  // Assign the project host and api key (required)
  config.host = FIREBASE_HOST;
  
  config.api_key = FIREBASE_API_KEY;
  
  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  
  auth.user.password = USER_PASSWORD;
  
  //Initialize the library with the Firebase authen and config.
  Firebase.begin(&config, &auth);

  uid = auth.token.uid.c_str();

  logd("User id is " + uid);
  base_path = "users/" + uid + "/" + WD_ID;
}

void loop() {
  switch (state) {
    case STATE_WAITING_FOR_BUTTON_PRESS:
      logd("Reading button");
      
      // Wait for button press
      buttonReading = digitalRead(INPUT_BUTTON);
      if (buttonReading == HIGH) {  // Button is pressed
        logd("Button is pressed, change state to read piezo");
        state = STATE_READING_PIEZO;

        // Calculate time to stop reading piezo
        timeClient.update();
        stopReadingPiezoTime = timeClient.getEpochTime() + DURATION_READ_PIEZO_MILLIS;

        FirebaseJson json;
        json.set(base_path + "/state", "START");

        playMelodyStart();
      } else {
        logd("Delay between button readings");
        delay(DELAY_BETWEEN_BUTTON_READINGS_MILLIS);
      }
      break;
    case STATE_READING_PIEZO:
      logd("Reading piezo");
      
      logd("Turning on LED");
      digitalWrite(OUTPUT_LED, HIGH);

      logd("Taking piezo reading");
      piezoReading = analogRead(INPUT_PIEZO);
      
      // Check time
      logd("Update time client");
      timeClient.update();
      currentTime = timeClient.getEpochTime();

      currentTime != lastAlertTime && key_long > currentTime - 4
      
      // Determine whether reading piezo state is complete
      isReadingPiezoDone = currentTime > stopReadingPiezoTime;
      
      if (isReadingPiezoDone) {
        logd("Piezo reading is done");

        logd("Set state to waiting for button press");
        state = STATE_WAITING_FOR_BUTTON_PRESS;

        playMelodyDone();

        logd("Write Firebase done");
        FirebaseJson json;
        json.set(base_path + "/state", "DONE");

        logd("Turn off LED")
        digitalWrite(OUTPUT_LED, LOW);
      } else {
        logd("Turn off LED")
        digitalWrite(OUTPUT_LED, LOW);
        
        logd("Delay between piezo readings");
        delay(DELAY_BETWEEN_PIEZO_READINGS_MILLIS);
      }     
      
      break;
    default:
      logd("Default, waiting for button press");
      state = STATE_WAITING_FOR_BUTTON_PRESS;
  }
}

void logd(String message) {
  if (LOGGING_DEBUG)
  {
    Serial.println(message);
  }
}
void playMelodyStart() {
  logd("Play melody start");
  playNote(MELODY_START, MELODY_START_DURATIONS, MELODY_START_DELAYS, MELODY_START_NUMBER_OF_NOTES);
}

void playMelodyDone() {
  logd("Play melody done");
  playNote(MELODY_DONE, MELODY_DONE_DURATIONS, MELODY_DONE_DELAYS, MELODY_DONE_NUMBER_OF_NOTES);
}
