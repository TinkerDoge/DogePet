// Example usage of DogePet Sectors library
// This shows how the modular sectors can be used in a minimal sketch

#include <Sectors.h>

void setup() {
  Serial.begin(115200);
  
  // Initialize hardware (display, I2C, audio, etc.)
  // ... your hardware setup code ...
  
  // Initialize all sectors with optional Gemini API key
  const char* apiKey = "your_api_key_here"; // or nullptr to disable AI
  Sectors::beginAll(apiKey);
  
  Serial.println("DogePet Sectors initialized!");
}

void loop() {
  // Update all sectors (handles motion, AI chatter, etc.)
  Sectors::updateAll();
  
  // Your main application logic
  // ... display rendering, input handling, etc. ...
  
  // Example: Send a message to the AI brain
  // Sectors::Brain::sendUserMessage("Hello DogePet!");
  
  // Example: Set robot mood
  // Sectors::Animations::setMood(MS_HAPPY);
  
  // Example: Show a toast message
  // Sectors::UI::showToast("Hello World!", 2000);
  
  delay(10); // Small delay to prevent overwhelming the loop
}
