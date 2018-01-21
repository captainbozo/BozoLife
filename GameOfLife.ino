/*****************************************
 * Conway's Game of Life: Pocket Edition
 *   by CaptainBozo
 *   
 * This project uses an Atmega168 and a
 * Nokia 5110 LCD to create a pocket-
 * sized Game of Life simulator. 
 * 
 * TODO: 
 *   Set up the SPIRAM! :D
 *   -- Might not be needed? Looks like I can 
 *      do comparisons of the Frame Buffer...
 *   Array comparison logic
 *   Game State logic
 *   Music
 *   Battery charging (Likely easy)
 *   LED Button on/off logic design
 *   Contrast Adjustment Pot
 *   Final Design
 *   Assembly   
 *****************************************/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include "LifeTitle.h"

// Hardware SPI (faster, but must use certain hardware pins):
// SCK is LCD serial clock (SCLK) - this is pin 13 on Arduino Uno
// MOSI is LCD DIN - this is pin 11 on an Arduino Uno
// pin 10 - Data/Command select (D/C)
// pin 9 - LCD chip select (CS)
// pin 8 - LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(10, 9, 8);

#define buttonsLatchPin 2
#define buttonsDataPin 4
#define buttonsClockPin 3

#define PIXEL_SIZE 1  // Big pixels
#define SPEED 20      // Delay in ms between generations

/*****************************************
 * BUTTONS PRESSED
 * --------------------
 * This will be an Int, storing
 * button presses as bits!*hu 
 * 
 * 00000000 00000000
 * |||||||| ||||||||
 * |||||||| |||||||\Start
 * |||||||| ||||||\Speed
 * |||||||| |||||\Place
 * |||||||| ||||\Erase
 * |||||||| |||\Step
 * |||||||| ||\Random
 * |||||||| |\LED Control
 * |||||||| \Not Used
 * |||||||\Up
 * ||||||\Down
 * |||||\Left
 * ||||\Right
 * |||\Not Used
 * ||\Not Used
 * |\Not Used
 * \Not Used
 ****************************************/
int16_t buttonsPressed = 0;
#define startButtonBit 0
#define speedButtonBit 1
#define placeButtonBit 2
#define eraseButtonBit 3
#define stepButtonBit 4
#define randomButtonBit 5
#define LEDControlButtonBit 6
#define upButtonBit 8
#define downButtonBit 9
#define leftButtonBit 10
#define rightButtonBit 11

/*****************************************
 * Game State:
 * --------------------
 * 0 = Title Screen
 * 1 = Running
 * 2 = Pause/Edit
 ****************************************/
int8_t gameState = 0;

int genSpeed = 4;
int currentFrame = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  // Initialize the screen.
  display.begin();
  // Set initial contrast.
  // TODO: Create Contrast Variable to be adjusted by potentiometer.
  display.setContrast(50);
  display.clearDisplay();

  // Initialize Buttons
  // TODO: 
  // - Get the Shift Register to output correctly!
  pinMode(buttonsLatchPin, OUTPUT);
  pinMode(buttonsClockPin, OUTPUT); 
  pinMode(buttonsDataPin, INPUT);
   
}

void loop() {
  // put your main code here, to run repeatedly:

  buttonRead();
  debug();
  
  // Main Game Loop
  // TODO: - Make this its own process
    switch(gameState) {
      // gameState 0 (default) is Title Screen)
      case 0:  display.drawBitmap(0, 0,  LifeTitle, 84, 48, 1);
               display.display();            
               if (bitRead(buttonsPressed, startButtonBit)) {
                gameState = 1;              
               }
               break;
      // gameState 1 is Running
      case 1:
               // Random Seed
               display.clearDisplay();
               randomise();
               display.display();
               delay(1000);

               // Begin
               int generations = 0;
               while(tick()) {
                 generations++;
                 display.display();
                 delay(SPEED);
               }

               // Finish
               display.setTextColor(BLACK, WHITE);
               display.setCursor(0,0);
               display.println(generations);
               display.display();

               // Restart
               delay(5000);
    }
}

void buttonRead() {
  // Read Button Input and Debounce
  // TODO: 
  //       - Mask the missing pins by setting them to 0, not by subtracting. This only 
  //         works if you assume that it will always be active, which MIGHT not always
  //         be true.
  //       - Debounce the buttons
  //       --- You can do an AND comparison for all the bits in the int16 to debounce...
  //       - Find a saner way to write this out
  //       --- Some kind of loop? Maybe make the button pins an array and do a For Each...

  // Have the Shift Register read the buttons by turning on and off the Latch Pin
  digitalWrite(buttonsLatchPin,1);
  delayMicroseconds(20);
  digitalWrite(buttonsLatchPin,0);

  buttonsPressed = shiftIn(buttonsDataPin, buttonsClockPin) - 192;
  buttonsPressed += (shiftIn(buttonsDataPin, buttonsClockPin) * 256) - 61440;
  
}

void debug() {
  if (bitRead(buttonsPressed, startButtonBit)) { Serial.print("Start Button, "); }
  if (bitRead(buttonsPressed, speedButtonBit)) { Serial.print("Speed Button, "); }
  if (bitRead(buttonsPressed, placeButtonBit)) { Serial.print("Place Button, "); }
  if (bitRead(buttonsPressed, eraseButtonBit)) { Serial.print("Erase Button, "); }
  if (bitRead(buttonsPressed, stepButtonBit)) { Serial.print("Step Button, "); }
  if (bitRead(buttonsPressed, randomButtonBit)) { Serial.print("Random Button, "); }
  if (bitRead(buttonsPressed, upButtonBit)) { Serial.print("Up Button, "); }
  if (bitRead(buttonsPressed, downButtonBit)) { Serial.print("Down Button, "); }
  if (bitRead(buttonsPressed, leftButtonBit)) { Serial.print("Left Button, "); }
  if (bitRead(buttonsPressed, rightButtonBit)) { Serial.print("Right Button, "); }   
  Serial.print(" --- Game State: ");
  Serial.print(gameState);
  Serial.print(" --- Current Frame: ");
  Serial.print(currentFrame);
  Serial.println();
}

// SHAMELESSLY STOLEN SHIFTIN PROCESS OMG
// TAKEN FROM: https://www.arduino.cc/en/Tutorial/ShiftIn
byte shiftIn(int myDataPin, int myClockPin) { 
  int i;
  int temp = 0;
  int pinState;
  byte myDataIn = 0;

  pinMode(myClockPin, OUTPUT);
  pinMode(myDataPin, INPUT);

//we will be holding the clock pin high 8 times (0,..,7) at the
//end of each time through the for loop

//at the begining of each loop when we set the clock low, it will
//be doing the necessary low to high drop to cause the shift
//register's DataPin to change state based on the value
//of the next bit in its serial information flow.
//The register transmits the information about the pins from pin 7 to pin 0
//so that is why our function counts down
  for (i=7; i>=0; i--)
  {
    digitalWrite(myClockPin, 0);
    delayMicroseconds(2);
    temp = digitalRead(myDataPin);
    if (temp) {
      pinState = 1;
      //set the bit to 0 no matter what
      myDataIn = myDataIn | (1 << i);
    }
    else {
      //turn it off -- only necessary for debuging
     //print statement since myDataIn starts as 0
      pinState = 0;
    }

    //Debuging print statements
    //Serial.print(pinState);
    //Serial.print("     ");
    //Serial.println (dataIn, BIN);

    digitalWrite(myClockPin, 1);

  }
  //debuging print statements whitespace
  //Serial.println();
  //Serial.println(myDataIn, BIN);
  return myDataIn;
}

void randomise() {
  for (uint8_t x = 0; x < LCDWIDTH; x = x + PIXEL_SIZE) {
    for (uint8_t y = 0; y < LCDHEIGHT; y = y + PIXEL_SIZE) {
      cell(x, y, random(0,2));  // 0 = White, 1 = Black
    }
  }
}

void cell(int x, int y, int color) {
  for (uint8_t i = 0; i < PIXEL_SIZE; i++) {
    for (uint8_t j = 0; j < PIXEL_SIZE; j++) {
      display.drawPixel(x + i, y + j, color);
    }
  }
}

bool tick() {
  bool something_happened = false;
  for (uint8_t x = 0; x < LCDWIDTH; x = x + PIXEL_SIZE) {
    for (uint8_t y = 0; y < LCDHEIGHT; y = y + PIXEL_SIZE) {

      // The current cell
      uint8_t alive = display.getPixel(x, y);

      // Count number of neighbours
      uint8_t neighbours = (
        display.getPixel(x - PIXEL_SIZE, y - PIXEL_SIZE) +
        display.getPixel(x, y - PIXEL_SIZE) +
        display.getPixel(x + PIXEL_SIZE, y - PIXEL_SIZE) +
        display.getPixel(x - PIXEL_SIZE, y) +
        display.getPixel(x + PIXEL_SIZE, y) +
        display.getPixel(x + PIXEL_SIZE, y + PIXEL_SIZE) +
        display.getPixel(x, y + PIXEL_SIZE) +
        display.getPixel(x - PIXEL_SIZE, y + PIXEL_SIZE)
      );

      // Apply the rules of life
      if (alive && (neighbours < 2 || neighbours > 3)) {
        cell(x, y, WHITE); // death
        if (!something_happened) something_happened = true;
      }
      else if (!alive && neighbours == 3) {
        cell(x, y, BLACK); // birth
        if (!something_happened) something_happened = true;
      }
    }
  }

  return something_happened;
}
