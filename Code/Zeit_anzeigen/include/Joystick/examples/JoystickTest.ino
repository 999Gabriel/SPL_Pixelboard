/*
  Joystick Library Test Sketch
  
  This sketch tests the Joystick library functionality including:
  - Button press detection (short and long press)
  - X and Y axis analog reading
  - Normalized axis values
  
  Pin Configuration:
  - Button Pin: GPIO pin connected to joystick button
  - X Pin: Analog input pin for X-axis (e.g., A0)
  - Y Pin: Analog input pin for Y-axis (e.g., A1)
*/

#include "Joystick.h"

// Pin configuration
const int BUTTON_PIN = 2;   // GPIO pin for button
const int X_PIN = A0;       // Analog pin for X-axis
const int Y_PIN = A1;       // Analog pin for Y-axis
const unsigned long DEBOUNCE_INTERVAL = 50; // 50ms debounce

// Create Joystick instance
Joystick joystick(BUTTON_PIN, X_PIN, Y_PIN, DEBOUNCE_INTERVAL);

void setup() {
  Serial.begin(115200);
  delay(1000); // Wait for Serial to initialize
  
  Serial.println("=================================");
  Serial.println("Joystick Library Test Started");
  Serial.println("=================================");
  Serial.println("Testing button and axis functionality...\n");
}

void loop() {
  // Update joystick state (must be called regularly)
  joystick.update();
  
  // Test button functionality
  testButtonFunctionality();
  
  // Test axis functionality
  testAxisFunctionality();
  
  delay(100); // Small delay to avoid overwhelming serial output
}

void testButtonFunctionality() {
  // Check if button is currently pressed
  if (joystick.isPressed()) {
    Serial.println("Button: PRESSED");
  }
  
  // Check if button was just pressed (rising edge)
  if (joystick.wasPressed()) {
    Serial.println(">>> Button PRESSED (rising edge detected)");
    Serial.print("    Short Press Count: ");
    Serial.println(joystick.getShortPressCount());
  }
  
  // Check if button was just released
  if (joystick.getFallingFlag()) {
    Serial.println(">>> Button RELEASED (falling edge detected)");
  }
  
  // Check if long press was detected
  if (joystick.wasLongPressed()) {
    Serial.println(">>> LONG PRESS DETECTED!");
    Serial.print("    Long Press Count: ");
    Serial.println(joystick.getLongPressCount());
  }
}

void testAxisFunctionality() {
  // Get raw axis values (0-4095)
  int xRaw = joystick.getXValue();
  int yRaw = joystick.getYValue();
  
  // Get normalized axis values (0.0-1.0)
  float xNorm = joystick.getXNormalized();
  float yNorm = joystick.getYNormalized();
  
  // Print axis values
  Serial.print("X-Axis: Raw=");
  Serial.print(xRaw);
  Serial.print(" | Normalized=");
  Serial.print(xNorm, 3); // 3 decimal places
  Serial.print("  |  Y-Axis: Raw=");
  Serial.print(yRaw);
  Serial.print(" | Normalized=");
  Serial.println(yNorm, 3); // 3 decimal places
}

/*
  Alternative: Advanced Test with Threshold Detection
  Uncomment the code below and replace testAxisFunctionality() call
  to test axis movement with thresholds
  
void testAxisFunctionalityAdvanced() {
  int xRaw = joystick.getXValue();
  int yRaw = joystick.getYValue();
  
  float xNorm = joystick.getXNormalized();
  float yNorm = joystick.getYNormalized();
  
  // Deadzone threshold (adjust as needed)
  const float DEADZONE = 0.1; // 10% deadzone
  
  // Detect direction with deadzone
  if (xNorm < 0.5 - DEADZONE) {
    Serial.print("LEFT ");
  } else if (xNorm > 0.5 + DEADZONE) {
    Serial.print("RIGHT ");
  }
  
  if (yNorm < 0.5 - DEADZONE) {
    Serial.print("UP ");
  } else if (yNorm > 0.5 + DEADZONE) {
    Serial.print("DOWN ");
  } else {
    Serial.print("CENTER ");
  }
  
  Serial.print(" | X=");
  Serial.print(xRaw);
  Serial.print(" | Y=");
  Serial.println(yRaw);
}
*/
