/**
 * Vibration sensor
 * 
 * When the button is pressed, listen for vibrations for a 
 * predetermined amount of time. Send those vibration readings
 * to a server. An LED light is used to display feedback to user.
 */
#include <NTPClient.h>
#define MINUTE_IN_MILLIS 60000

const int INPUT_PIEZO = A0;
const int INPUT_BUTTON = D0;
const int OUTPUT_LED = D1;  // LED for feedback to user

const int STATE_WAITING_FOR_BUTTON_PRESS = 0;
const int STATE_READING_PIEZO = 1;

const int DELAY_BETWEEN_BUTTON_READINGS_MILLIS = 5 * MINUTE_IN_MILLIS;
const int DELAY_BETWEEN_PIEZO_READINGS_MILLIS = 2 * MINUTE_IN_MILLIS;

// Assumes that no cycle will take longer than 3 hours (soak + wash, etc.)
const int DURATION_READ_PIEZO_MILLIS = 180 * MINUTE_IN_MILLIS;

const long UTC_OFFSET_SECONDS = 0 * 60 * 60;  // Chicago UTC/GMT -6 hours

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

  // Setup NTP client
  timeClient.begin();

  // TODO: Setup Firebase
  // TODO: Switch to connecting to intranet

  // TODO: Setup WiFi
}

void loop() {
  switch (state) {
    case STATE_WAITING_FOR_BUTTON_PRESS:
      // Wait for button press
      buttonReading = digitalRead(INPUT_BUTTON);
      if (buttonReading == HIGH) {  // Button is pressed
        state = STATE_READING_PIEZO;

        // Calculate time to stop reading piezo
        timeClient.update();
        stopReadingPiezoTime = timeClient.getEpochTime() + DURATION_READ_PIEZO_MILLIS;
      } else {
        delay(DELAY_BETWEEN_BUTTON_READINGS_MILLIS);
      }
      break;
    case STATE_READING_PIEZO:
      piezoReading = analogRead(INPUT_PIEZO);
      
      // TODO: Setup WiFi
      
      // Check time
      timeClient.update();
      currentTime = timeClient.getEpochTime();

      currentTime != lastAlertTime && key_long > currentTime - 4
      
      // Determine whether reading piezo state is complete
      isReadingPiezoDone = currentTime > stopReadingPiezoTime;
      
      if (isReadingPiezoDone) {
        state = STATE_WAITING_FOR_BUTTON_PRESS;
      } else {
        delay(DELAY_BETWEEN_PIEZO_READINGS_MILLIS);
      }
      break;
    default:
      state = STATE_WAITING_FOR_BUTTON_PRESS;
  }
}
