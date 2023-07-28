/*
 *  Project     CSync basic detector
 *  @author     KabukiFlux
 *
 *  Name:      15khzDetectArduino
 *  Description:  Detects the sync frequency
 *  for a connected CRT input signal.
 *
 *  License: MIT KabukiFlux 2023
 *  
 *  Pinout:
 *    pin 2 -> CSync
 *    pin 4 -> Status LED 15kHz signal
 *    pin 5 -> Status LED 24kHz signal
 *    pin 6 -> Status LED 31kHz signal
 *
 */
// Pro micro 0, 1, 2, 3, 7 INT pins.
const int csyncPin = 3;

// LEDS to show detection.
const int detected15Pin = 4;
const int detected24Pin = 5;
const int detected31Pin = 6;

// Number of readings to take to calculate the most readed line value.
const int numReadings = 20;

// Debug to serial
const bool debug = true;

volatile unsigned long csyncStart = micros();
volatile unsigned long csyncEnd = micros();
volatile unsigned long pulsePeriod = micros();
volatile bool interruptStart = true;
volatile bool csyncNewLineValue = false;

class DetectLeds {
  public:
    DetectLeds() {
      pinMode(detected15Pin, OUTPUT);
      pinMode(detected24Pin, OUTPUT);
      pinMode(detected31Pin, OUTPUT);
      digitalWrite(detected15Pin, LOW);
      digitalWrite(detected24Pin, LOW);
      digitalWrite(detected31Pin, LOW);
    }

    void set15() {
      digitalWrite(detected15Pin, HIGH);
      digitalWrite(detected24Pin, LOW);
      digitalWrite(detected31Pin, LOW);
    }

    void set24() {
      digitalWrite(detected15Pin, LOW);
      digitalWrite(detected24Pin, HIGH);
      digitalWrite(detected31Pin, LOW);
    }

    void set31() {
      digitalWrite(detected15Pin, LOW);
      digitalWrite(detected24Pin, LOW);
      digitalWrite(detected31Pin, HIGH);
    }

    void noSync() {
      digitalWrite(detected15Pin, LOW);
      digitalWrite(detected24Pin, LOW);
      digitalWrite(detected31Pin, LOW);
    }

};

class DebugMessages {
  public:
    DebugMessages() {
      if (debug) {
        Serial.begin(9600);
      }
    }

    void showFreq(float frequency) {
      if (debug) {
        Serial.print(frequency);
        Serial.println(" Hz");
      }
    }

    void messageCommon() {
      Serial.print("Csync freq: ");
    }

    void message15(float frequency) {
      if (debug) {
        this->messageCommon();
        Serial.print(frequency);
        Serial.println(" Hz (15kHz signal)");
      }
    }

    void message24(float frequency) {
      if (debug) {
        this->messageCommon();
        Serial.print(frequency);
        Serial.println(" Hz (24kHz signal)");
      }
    }

    void message31(float frequency) {
      if (debug) {
        this->messageCommon();
        Serial.print(frequency);
        Serial.println(" Hz (31kHz signal)");
      }
    }

    void outOfRange(float frequency) {
      if (debug) {
        this->messageCommon();
        Serial.print(frequency);
        Serial.println(" Hz - out of ranges.");
      }
    }

    void noSync() {
      if (debug) {
        Serial.println("No csync signal detected.");
      }
    }

    void lineWidth(float period) {
      if (debug) {
        Serial.print(period);
        Serial.print("uS ");
      }
    }

    void lineEnd() {
      if (debug) {
        Serial.println(" ");
      }
    }

};

class CsyncCalculations {
  public:
    float mostRegularWidth(float arr[], int size) {
      float regular = 0.0;
      int maxCount = 0;
      for (int i = 0; i < size; i++) {
        int count = 0;
        for (int j = 0; j < size; j++) {
          if (arr[j] == arr[i]) {
            count++;
          }
        }
        if (count > maxCount) {
          maxCount = count;
          regular = arr[i];
        }
      }
      return regular;
    }

    float periodToFrequency(unsigned long period) {
      // Calculate frequency in Hz (1/period in microseconds)
      return 1000000.0 / pulsePeriod;
    }

};

DetectLeds leds;
DebugMessages messages;
CsyncCalculations calculations;

void csyncInterrupt()
{
  if (interruptStart) {
    csyncStart = micros();
    // First rising interrupt
    interruptStart = false;
  }
  else {
    // Second rising interrupt (interval)
    csyncEnd = micros();
    csyncNewLineValue = (csyncEnd > csyncStart);
    pulsePeriod = csyncEnd - csyncStart;
    interruptStart = true;
  }
}

void setup() {
  pinMode(csyncPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(csyncPin),csyncInterrupt,RISING);
}

void loop() {

  float validReadings[numReadings]; // Array to store the valid readings
  int validReadingsCount = 0; // Number of valid readings obtained
  float sum = 0;

  bool regenerateInterrupt = csyncNewLineValue;

  for (int i = 0; i < numReadings; i++) {
    if (csyncNewLineValue) {
      csyncNewLineValue = false;
      messages.lineWidth(pulsePeriod);
      // If a pulse was detected and is not tooo long 64uS usually
      if (1000 > pulsePeriod > 0) {
        float frequency = calculations.periodToFrequency(pulsePeriod);
        // Check if the frequency is within any of the specified ranges
        if ((frequency >= 15000 && frequency <= 18000) ||
            (frequency >= 24000 && frequency <= 26000) ||
            (frequency >= 29000 && frequency <= 33000) ||
            (frequency >= 35000 && frequency <= 47000)) {
          validReadings[validReadingsCount] = frequency;
          validReadingsCount++;
        }
      }
      delay(4); // Wait XmS for the next result (not required)
    }
  }

  // Stop Int after taking the measures
  if (regenerateInterrupt) {
    detachInterrupt(digitalPinToInterrupt(csyncPin));
  }

  if (validReadingsCount > 0) {
    messages.lineEnd();
    float mostDetectedFrequency = calculations.mostRegularWidth(validReadings, validReadingsCount);
    if (mostDetectedFrequency >= 15000 && mostDetectedFrequency <= 18000) {
      leds.set15();
      messages.message15(mostDetectedFrequency);
    } else if (mostDetectedFrequency >= 24000 && mostDetectedFrequency <= 26000) {
      leds.set24();
      messages.message24(mostDetectedFrequency);
    } else if (mostDetectedFrequency >= 29000 && mostDetectedFrequency <= 33000) {
      leds.set31();
      messages.message31(mostDetectedFrequency);
    } else {
      leds.noSync();
      messages.outOfRange(mostDetectedFrequency);
    }

  } else {
    messages.lineEnd();
    leds.noSync();
    messages.noSync();
  }
  
  delay(1000); // Wait before checking again

  // Restart Int for the next iteration
  if (regenerateInterrupt) {
    attachInterrupt(digitalPinToInterrupt(csyncPin),csyncInterrupt,RISING);
  }

}
