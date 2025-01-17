#include <sstream>

class SerialControl: public Loggable {
   uint8_t position = 0;
   char inputString[124] = "";

   public:
      std::function<void(char*)> *onParseString;
      
      SerialControl(): Loggable("Serial") {}

   void run() {
      if (!Serial.available() || onParseString == nullptr) return;

      while (Serial.available()) {
         char c = Serial.read();

         if (c == '\n' || c == '\r') {
            (*onParseString)(inputString);

            //! clear the character one by one, just printing an emptyString wont do it
            for (int i = 0; i < 100; i++) {
               Serial.print(' ');
               Serial.print('\b');
            }
            strcpy(inputString, "");
            position = 0;
            Serial.print("");
            Serial.print("\r");
         }

         // //! handle backspace
         // else if (c == '\b' && position > 0) {
         //    // Handle backspace to remove the previous character
         //    position--;
         //    inputString[position] = '\0'; // Overwrite with a null character
         //    Serial.print("\b \b");  // Clear the character on the console
         // } 

         //! handle other keystrokke
         else if (position < sizeof(inputString) - 1) {
               // Insert the new character at the current position
               inputString[position++] = c;

               // // Move cursor to the beginning of the line and print the input
               // Serial.print('\r');
               // Serial.print("cmd: ");
               // Serial.print(inputString);
         }
      }
   }
};