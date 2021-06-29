/**
 * Vibration sensor
 * 
 * When the button is pressed, listen for vibrations for a 
 * predetermined amount of time. Send those vibration readings
 * to a server. An LED light is used to display feedback to user.
 */
#include "Conf.h"

#include <AutoPower.h>

#include <ESP8266WiFi.h>

#include <FirebaseESP8266.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include "pitches.h"

#define MINUTE_IN_MILLIS 60000
#define SECOND_IN_MILLIS 1000

#define LOGGING_DEBUG true

#define AUTOPOWER_HOLD 16  // Connects to HOLD pin on AutoPower module
#define AUTOPOWER_BUTTON 5  // Connects to BTN pin on AutoPower module

#define INPUT_PIEZO = A0;
#define INPUT_BUTTON_UTIL = 14;
#define OUTPUT_LED = 15;  // LED for feedback to user
#define OUTPUT_BUZZER = 10;  // Buzzer for feedback to user

AutoPower power(AUTOPOWER_HOLD, AUTOPOWER_BUTTON, OUTPUT_LED, 2); // Hold button 2 seconds to power off

const int STATE_READING_PIEZO = 0;

const int DELAY_BETWEEN_PIEZO_READINGS_MILLIS = 5 * SECOND_IN_MILLIS;  // TODO: increase for prod.

const int MAX_READ_PIEZO_DURATION = 120 * MINUTE_IN_MILLIS;  // 120 min cycle max.
const int MAX_PIEZO_READINGS = MAX_READ_PIEZO_DURATION / DELAY_BETWEEN_PIEZO_READINGS_MILLIS;

// Assumes that no cycle will take longer than 3 hours (soak + wash, etc.)
const int DURATION_READ_PIEZO_MILLIS = 180 * MINUTE_IN_MILLIS;

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
int state = STATE_READING_PIEZO;
long stopReadingPiezoTime;
bool isReadingPiezoDone;

const long utcOffsetInSeconds = 0 * 60 * 60;  // Chicago UTC/GMT -6 hours

FirebaseData fbdo;

// Define the FirebaseAuth data for authentication data
FirebaseAuth auth;
  
// Define the FirebaseConfig data for config data
FirebaseConfig config;

/* The calback function to print the token generation status */
void tokenStatusCallback(TokenInfo info);

/* The function to print the operating results */
void printResult(FirebaseData &data);

void setup() {
  Serial.begin(9600);

  power.setTimeout(120);  // 2 min power off timeout
  
  pinMode(INPUT_BUTTON, INPUT);
  pinMode(INPUT_BUTTON, INPUT_PULLUP);  
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

  // Firebase setup
  logd("Connecting to Firebase...");
  
  // Assign the project host and api key (required)
  config.host = FIREBASE_HOST;
  
  config.api_key = FIREBASE_API_KEY;
  
  // Assign the user sign in credentials
  auth.user.email = FIREBASE_USER_EMAIL;
  
  auth.user.password = FIREBASE_USER_PASSWORD;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  config.token_status_callback = tokenStatusCallback;
  
  /** Assign the maximum retry of token generation */
  config.max_token_generation_retry = 5;

  /* Initialize the library with the Firebase authen and config */
  Firebase.begin(&config, &auth);

  uid = auth.token.uid.c_str();

  logd("User id is " + uid);
  base_path = "users/" + uid + "/" + WD_ID;
  logd("Base path is: " + base_path);

  // Play the start melody.
  playMelodyStart();
  
  // Update the online state.
  FirebaseJson json;
  json.set(base_path + "/state", "START");
}


int piezoReadingFake = 0;
unsigned long dataMillis = 0;
int count = 0;

void loop() {
  power.updatePower();  // Prevent power down if the loop continues.
  switch (state) {
    case STATE_READING_PIEZO:
      count++;
      
      logd("Taking piezo reading");
      piezoReading = analogRead(INPUT_PIEZO);
      
      // Determine whether reading piezo state is complete
      isReadingPiezoDone = count > MAX_PIEZO_READINGS;
      
      if (isReadingPiezoDone) {
        logd("Piezo reading is done");
        playMelodyDone();

        logd("Write Firebase done");
        FirebaseJson json;
        json.set(base_path + "/state", "DONE");
        
        power.powerOff();
      } else {
        logd("Blink to indicate that reading is about to be taken");
        ledOn();
        delay(500);
        ledOff();
        
        // Write to Firebase the piezo reading
        piezoReadingFake = (piezoReadingFake + 1) % 5;

        FirebaseJson json;
        json.set(base_path + "/state", piezoReadingFake);
        
        logd("Delay between piezo readings");
        delay(DELAY_BETWEEN_PIEZO_READINGS_MILLIS);
      }     
      
      break;
    default:
      logd("Default.");
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

void playNote(int melody[], int durations[], int delays[], int nNotes) { 
  for (int i = 0; i < nNotes; i++) {
    tone(OUTPUT_BUZZER, melody[i], durations[i]); 
    delay(delays[i]);
  }
}

void tokenStatusCallback(TokenInfo info)
{
    /** fb_esp_auth_token_status enum
     * token_status_uninitialized,
     * token_status_on_initialize,
     * token_status_on_signing,
     * token_status_on_request,
     * token_status_on_refresh,
     * token_status_ready,
     * token_status_error
    */
    if (info.status == token_status_error)
    {
        logd("Error");
        Serial.printf("Token info: type = %s, status = %s\n", getTokenType(info).c_str(), getTokenStatus(info).c_str());
        Serial.printf("Token error: %s\n", getTokenError(info).c_str());
    }
    else
    {
      logd("Info");
        Serial.printf("Token info: type = %s, status = %s\n", getTokenType(info).c_str(), getTokenStatus(info).c_str());
    }
}

void printResult(FirebaseData &data)
{

    if (data.dataType() == "int")
        Serial.println(data.intData());
    else if (data.dataType() == "float")
        Serial.println(data.floatData(), 5);
    else if (data.dataType() == "double")
        printf("%.9lf\n", data.doubleData());
    else if (data.dataType() == "boolean")
        Serial.println(data.boolData() == 1 ? "true" : "false");
    else if (data.dataType() == "string")
        Serial.println(data.stringData());
    else if (data.dataType() == "json")
    {
        Serial.println();
        FirebaseJson &json = data.jsonObject();
        //Print all object data
        Serial.println("Pretty printed JSON data:");
        String jsonStr;
        json.toString(jsonStr, true);
        Serial.println(jsonStr);
        Serial.println();
        Serial.println("Iterate JSON data:");
        Serial.println();
        size_t len = json.iteratorBegin();
        String key, value = "";
        int type = 0;
        for (size_t i = 0; i < len; i++)
        {
            json.iteratorGet(i, type, key, value);
            Serial.print(i);
            Serial.print(", ");
            Serial.print("Type: ");
            Serial.print(type == FirebaseJson::JSON_OBJECT ? "object" : "array");
            if (type == FirebaseJson::JSON_OBJECT)
            {
                Serial.print(", Key: ");
                Serial.print(key);
            }
            Serial.print(", Value: ");
            Serial.println(value);
        }
        json.iteratorEnd();
    }
    else if (data.dataType() == "array")
    {
        Serial.println();
        //get array data from FirebaseData using FirebaseJsonArray object
        FirebaseJsonArray &arr = data.jsonArray();
        //Print all array values
        Serial.println("Pretty printed Array:");
        String arrStr;
        arr.toString(arrStr, true);
        Serial.println(arrStr);
        Serial.println();
        Serial.println("Iterate array values:");
        Serial.println();
        for (size_t i = 0; i < arr.size(); i++)
        {
            Serial.print(i);
            Serial.print(", Value: ");

            FirebaseJsonData &jsonData = data.jsonData();
            //Get the result data from FirebaseJsonArray object
            arr.get(jsonData, i);
            if (jsonData.typeNum == FirebaseJson::JSON_BOOL)
                Serial.println(jsonData.boolValue ? "true" : "false");
            else if (jsonData.typeNum == FirebaseJson::JSON_INT)
                Serial.println(jsonData.intValue);
            else if (jsonData.typeNum == FirebaseJson::JSON_FLOAT)
                Serial.println(jsonData.floatValue);
            else if (jsonData.typeNum == FirebaseJson::JSON_DOUBLE)
                printf("%.9lf\n", jsonData.doubleValue);
            else if (jsonData.typeNum == FirebaseJson::JSON_STRING ||
                     jsonData.typeNum == FirebaseJson::JSON_NULL ||
                     jsonData.typeNum == FirebaseJson::JSON_OBJECT ||
                     jsonData.typeNum == FirebaseJson::JSON_ARRAY)
                Serial.println(jsonData.stringValue);
        }
    }
    else if (data.dataType() == "blob")
    {

        Serial.println();

        for (size_t i = 0; i < data.blobData().size(); i++)
        {
            if (i > 0 && i % 16 == 0)
                Serial.println();

            if (i < 16)
                Serial.print("0");

            Serial.print(data.blobData()[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
    else if (data.dataType() == "file")
    {

        Serial.println();

        File file = data.fileStream();
        int i = 0;

        while (file.available())
        {
            if (i > 0 && i % 16 == 0)
                Serial.println();

            int v = file.read();

            if (v < 16)
                Serial.print("0");

            Serial.print(v, HEX);
            Serial.print(" ");
            i++;
        }
        Serial.println();
        file.close();
    }
    else
    {
        Serial.println(data.payload());
    }
}

/* The helper function to get the token type string */
String getTokenType(struct token_info_t info)
{
    switch (info.type)
    {
    case token_type_undefined:
        return "undefined";

    case token_type_legacy_token:
        return "legacy token";

    case token_type_id_token:
        return "id token";

    case token_type_custom_token:
        return "custom token";

    case token_type_oauth2_access_token:
        return "OAuth2.0 access token";

    default:
        break;
    }
    return "undefined";
}

/* The helper function to get the token status string */
String getTokenStatus(struct token_info_t info)
{
    switch (info.status)
    {
    case token_status_uninitialized:
        return "uninitialized";

    case token_status_on_initialize:
        return "on initializing";

    case token_status_on_signing:
        return "on signing";

    case token_status_on_request:
        return "on request";

    case token_status_on_refresh:
        return "on refreshing";

    case token_status_ready:
        return "ready";

    case token_status_error:
        return "error";

    default:
        break;
    }
    return "uninitialized";
}

/* The helper function to get the token error string */
String getTokenError(struct token_info_t info)
{
    String s = "code: ";
    s += String(info.error.code);
    s += ", message: ";
    s += info.error.message.c_str();
    return s;
}

void ledOn() {
  logd("Turning on LED");
  digitalWrite(OUTPUT_LED, HIGH);
}

void ledOff() {
  logd("Turning off LED");
  digitalWrite(OUTPUT_LED, LOW);
}
