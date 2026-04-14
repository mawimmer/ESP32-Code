#include <Arduino.h>

#define CLK 2
#define DT 3
#define SW 4


unsigned long lastBlinkTime = 0;
bool ledState = LOW;
int counter = 0;
int lastStateCLK;
boolean buttoncheck = false;


void setup() {
    //Serial0 for reading content in VSC Terminal
  Serial0.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT); 

  //Rotary Encoder Block - Start

  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);
  
  // Get the starting position
  lastStateCLK = digitalRead(CLK);

    //Rotary Encoder Block - End
}

void loop() {

if (millis() - lastBlinkTime >= 500) {
    lastBlinkTime = millis();
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
    
    // Only print "alive" when the LED toggles, so the terminal stays clean
    if (ledState == HIGH) {
        Serial0.println("ESP32 is alive!");
    }
  }

    //Rotary Encoder Block - Start

  int currentStateCLK = digitalRead(CLK);

  // If the CLK state changed, the knob moved
  if (currentStateCLK != lastStateCLK) {
    
    // Check DT to see which direction it moved
  if (digitalRead(DT) != currentStateCLK) {
      counter++;
  } else {
      counter--;
  }

  // Keep the counter within your -21 to 21 range
  if (counter > 21)  counter = 21;
  if (counter < -21) counter = -21;
    
    Serial0.println(counter);
  }
  // Save the state for the next loop
  lastStateCLK = currentStateCLK;

  if(digitalRead(SW) == LOW && buttoncheck == false) {
    delay(50);
    Serial0.println("The Button was pressed!");
    buttoncheck = true;
  };
  if (digitalRead(SW) == HIGH && buttoncheck == true) {
    delay(50);
    buttoncheck = false;
  };
      //Rotary Encoder Block - End
}

