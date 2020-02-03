/*
================================================================================
   Based on arduino Leonardo (Atmega32u4) USB converter for
   Jissen Mahjong controller (aka Capcom, Famicom mahjong controller)
   https://wiki.nesdev.com/w/index.php/Jissen_Mahjong_controller
================================================================================
- The controller contains one 8-bit 4021 shift register and one transistor 
  in switch mode for selecting buttons lines.
- This shift register takes parallel inputs from buttons and converts them 
  into a serial output.
- There is three lines with buttons [A]-[H], [I]-[N], [SEL]-[RON]  
- This code first selects a buttons line then latches the data and then shifts 
  in the first bit on the data line. 
  Then it clocks and shifts in on the data line until all bits are received.
  Then selects next buttons line and repeat procedure.
- What is debugged are the button states of the mahjong controller.
- A logical "1" means the button is not pressed. A logical "0" means the button is
  pressed.
- This code shifts the first bit of data into the LSB.
- The order of shifting for the buttons is shown in the table below:

        Bit# | FMC MJ Buttons   
        -----------------------
          0  | H | - | -      |   
        -----------------------
          1  | G | - | RON    |   
        -----------------------
          2  | F | N | RIICHI |    
        -----------------------
          3  | E | M | CHI    |   
        -----------------------
          4  | D | L | PON    |   
        -----------------------
          5  | C | K | KAN    |   
        -----------------------
          6  | B | J | ST     |    
        -----------------------
          7  | A | I | SEL    |    
        -----------------------
        
- The mahjong controller pinout is shown below (looking into controller's
  connector end):
    __________
   /          |
  /       O 8 | 8  - No Connection
  | 15 O      | 15 - +5V also command buttons row (PUR wire)
  |       O 7 | 7  - Data Out (RED wire)
  | 14 O      | 14 - No Connection
  |       O 6 | 6  - No Connection
  | 13 O      | 13 - No Connection
  |       O 5 | 5  - No Connection
  | 12 O      | 12 - Latch (YEL wire)
  |       O 4 | 4  - No Connection
  | 11 O      | 11 - A-H buttons row (GREE wire)
  |       O 3 | 3  - No Connection
  | 10 O      | 10 - I-N buttons row (BLU wire)
  |       O 2 | 2  - No Connection
  | 9  O      | 9  - Clock(ORA wire)
  \       O 1 | 1  - Ground (BRO wire)
   \__________|

- For the Jissen Mahjong controller, data must be read three times. Each time, 
  the logical state of the two wires - A-H (pin 11, green wire) and I-N (pin 10, blue wire)
  selects which row is currently being read. If they both in HIGH state - command buttons
  row being read. If A-H is HIGH and I-N is LOW - I-N buttons row being read.
  If A-H is LOW and I-N is HIGH - A-H buttons row being read.
- Based on NES gamepad arduino code from here:
  https://www.allaboutcircuits.com/projects/nes-controller-interface-with-an-arduino-uno/
*/
//===============================================================================
//  Header Files
//===============================================================================
#include "Keyboard.h"

//===============================================================================
//  Constants
//===============================================================================
// Here we have a bunch of constants that will become clearer when we look at the
// readAHrow(), readINrow(), readCommandRow() functions. Basically, we will use 
// these contents to clear a bit. These are chosen according to the table above.

//Buttons in commands row
const int RON_BUTTON       = 1;
const int RIICHI_BUTTON    = 2;
const int CHI_BUTTON       = 3;
const int PON_BUTTON       = 4;
const int KAN_BUTTON       = 5;
const int ST_BUTTON        = 6;
const int SEL_BUTTON       = 7;
//Buttons in I - N row
const int N_BUTTON         = 2;
const int M_BUTTON         = 3;
const int L_BUTTON         = 4;
const int K_BUTTON         = 5;
const int J_BUTTON         = 6;
const int I_BUTTON         = 7;
//Buttons in A - H row
const int H_BUTTON         = 0;
const int G_BUTTON         = 1;
const int F_BUTTON         = 2;
const int E_BUTTON         = 3;
const int D_BUTTON         = 4;
const int C_BUTTON         = 5;
const int B_BUTTON         = 6;
const int A_BUTTON         = 7;

//===============================================================================
//  Variables
//===============================================================================
byte nesRegister  = 0;    // We will use this to hold current button states

//===============================================================================
//  Pin Declarations
//===============================================================================
//Arduino inputs:
int nesData       = 4;    // The data pin (red) for the NES MJ controller

//Arduino outputs:
int nesClock      = 5;    // The clock pin (ora) for the NES MJ controller
int nesLatch      = 6;    // The latch pin (yel) for the NES MJ controller
//Row selection outputs
int nesAH         = 7;    // The A-H butttons pin (gree)
int nesIN         = 8;    // The I-N buttons pin (blu)

//Keyboard buttons control
char lastPressedKey  = 0x0;   // We will use this to hold last pressed button
bool isButtonPressed = false; // General information about current buttons
                              // pressed / not pressed  

//===============================================================================
//  Initialization
//===============================================================================
void setup() 
{
  // Set appropriate pins to inputs
  pinMode(nesData, INPUT);
  
  // Set appropriate pins to outputs
  pinMode(nesClock, OUTPUT);
  pinMode(nesLatch, OUTPUT);
  pinMode(nesAH, OUTPUT);
  pinMode(nesIN, OUTPUT);
  
  // Set initial states
  digitalWrite(nesClock, LOW);
  digitalWrite(nesLatch, LOW);
  
  // Initialize control over the keyboard:
  Keyboard.begin();
}

//===============================================================================
//  Main
//===============================================================================
void loop() 
{
  // This function call will return the states of all MJ controller's register
  // in a nice 8 bit variable format. Remember to refer to the table and
  // constants above for which button maps where!
  // Reading first row - [A] to [H] buttons
  nesRegister = readAHrow();
  
  // To give you an idea on how to use this data to control things for your
  // next project, look through the serial terminal code below. Basically,
  // just choose a bit to look at and decide what to do whether HIGH (not pushed)
  // or LOW (pushed). What is nice about this test code is that we mapped all
  // of the bits to the actual button name so no useless memorizing!
  if (bitRead(nesRegister, H_BUTTON) == 0)
    keyboardButtonPress('h');
  if (bitRead(nesRegister, G_BUTTON) == 0)
    keyboardButtonPress('g');
  if (bitRead(nesRegister, F_BUTTON) == 0)
    keyboardButtonPress('f');
  if (bitRead(nesRegister, E_BUTTON) == 0)
    keyboardButtonPress('e');
  if (bitRead(nesRegister, D_BUTTON) == 0)
    keyboardButtonPress('d');
  if (bitRead(nesRegister, C_BUTTON) == 0)
    keyboardButtonPress('c');
  if (bitRead(nesRegister, B_BUTTON) == 0)
    keyboardButtonPress('b');
  if (bitRead(nesRegister, A_BUTTON) == 0)
    keyboardButtonPress('a');

  // Reading second row - [I] to [N] buttons
  nesRegister = readINrow();
  
  if (bitRead(nesRegister, N_BUTTON) == 0)
    keyboardButtonPress('n');
  if (bitRead(nesRegister, M_BUTTON) == 0)
    keyboardButtonPress('m');
  if (bitRead(nesRegister, L_BUTTON) == 0)
    keyboardButtonPress('l');
  if (bitRead(nesRegister, K_BUTTON) == 0)
    keyboardButtonPress('k');
  if (bitRead(nesRegister, J_BUTTON) == 0)
    keyboardButtonPress('j');
  if (bitRead(nesRegister, I_BUTTON) == 0)
    keyboardButtonPress('i');

  // Reading third row - command buttons
  nesRegister = readCommandRow();
  
  if (bitRead(nesRegister, RON_BUTTON) == 0)
    keyboardButtonPress('z');
  if (bitRead(nesRegister, RIICHI_BUTTON) == 0)
    keyboardButtonPress(KEY_LEFT_SHIFT);        // [Left SHIFT] for Riichi
  if (bitRead(nesRegister, CHI_BUTTON) == 0)
    keyboardButtonPress(' ');                   // [SPACE] for Chi
  if (bitRead(nesRegister, PON_BUTTON) == 0)
    keyboardButtonPress(KEY_LEFT_ALT);          // [Left ALT] for Pon
  if (bitRead(nesRegister, KAN_BUTTON) == 0)
    keyboardButtonPress(KEY_LEFT_CTRL);         // [Left CTRL] for Kan
  if (bitRead(nesRegister, ST_BUTTON) == 0)
    keyboardButtonPress('1');
  if (bitRead(nesRegister, SEL_BUTTON) == 0)
    keyboardButtonPress('5');

  // If some button was pressed
  if  (isButtonPressed)
    delay(80);   // Slight delay for preventing contact bounce.
  else  //if the button was released
  {
    if (lastPressedKey != 0x0)
    {
      Keyboard.releaseAll ();  //release all pressed keys
      //Keyboard.release(lastPressedKey);  //release the last pressed key
      lastPressedKey = 0x0;
    }
  }

  //for next iteration if no pressed buttons it is indicates this state
  isButtonPressed = false;
}

//===============================================================================
//  Functions
//===============================================================================
///////////////
// readAHrow //
///////////////
byte readAHrow() 
{  
  // Pre-load a variable with all 1's which assumes all buttons are not
  // pressed. But while we cycle through the bits, if we detect a LOW, which is
  // a 0, we clear that bit. In the end, we find all the buttons states at once.
  int tempData = 255;

  //Set outputs for checking A - H row
  digitalWrite(nesAH, LOW);
  digitalWrite(nesIN, HIGH);
    
  // Quickly pulse the nesLatch pin so that the register grab what it see on
  // its parallel data pins.
  digitalWrite(nesLatch, HIGH);
  digitalWrite(nesLatch, LOW);
 
  // Upon latching, the first bit is available to look at, which is the state
  // of the [H] button. We see if it is low, and if it is, we clear out variable's
  // first bit to indicate this is so.
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, H_BUTTON);
    
  // Clock the next bit which is the [G] button and determine its state just like
  // we did above.
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, G_BUTTON);
  
  // Now do this for the rest of them!
  
  // [F] button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, F_BUTTON);

  // [E] button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, E_BUTTON);

  // [D] button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, D_BUTTON);
    
  // [C] button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, C_BUTTON);

  // [B] button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, B_BUTTON);  
    
  // [A] button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, A_BUTTON);
    
  // After all of this, we now have our variable all bundled up
  // with all of the A-H buttons states.
  return tempData;
}

///////////////
// readINrow //
///////////////
byte readINrow() 
{  
  // Pre-load a variable with all 1's which assumes all buttons are not
  // pressed. But while we cycle through the bits, if we detect a LOW, which is
  // a 0, we clear that bit. In the end, we find all the buttons states at once.
  int tempData = 255;

  //Set outputs for checking I - N row
  digitalWrite(nesAH, HIGH);
  digitalWrite(nesIN, LOW);
    
  // Quickly pulse the nesLatch pin
  digitalWrite(nesLatch, HIGH);
  digitalWrite(nesLatch, LOW);
 
  // In I-N row first two bits does not used so go to third bit
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  
  // [N] button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, N_BUTTON);

  // [M] button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, M_BUTTON);

  // [L] button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, L_BUTTON);
    
  // [K] button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, K_BUTTON);

  // [J] button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, J_BUTTON);  
    
  // [I] button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, I_BUTTON);
    
  // After all of this, we now have our variable all bundled up
  // with all of the I-N buttons states.
  return tempData;
}

////////////////////
// readCommandRow //
////////////////////
byte readCommandRow() 
{  
  // Pre-load a variable with all 1's which assumes all buttons are not
  // pressed. But while we cycle through the bits, if we detect a LOW, which is
  // a 0, we clear that bit. In the end, we find all the buttons states at once.
  int tempData = 255;

  //Set outputs for checking command butons row
  digitalWrite(nesAH, HIGH);
  digitalWrite(nesIN, HIGH);
    
  // Quickly pulse the nesLatch pin
  digitalWrite(nesLatch, HIGH);
  digitalWrite(nesLatch, LOW);
 
  // In command buttons row first bit does not used so go to second bit
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, RON_BUTTON);
  
  // [RIICHI] button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, RIICHI_BUTTON);

  // [CHI] button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, CHI_BUTTON);

  // [PON] button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, PON_BUTTON);
    
  // [KAN] button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, KAN_BUTTON);

  // [ST] button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, ST_BUTTON);  
    
  // [SEL] button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, SEL_BUTTON);
    
  // After all of this, we now have our variable all bundled up
  // with all of the command buttons states.
  return tempData;
}

///////////////////////////////
// USB Keyboard button press //
///////////////////////////////
void keyboardButtonPress(char key) 
{  
  //Indicate that the button was pressed
  isButtonPressed = true;
  
  //Press the button without release
  Keyboard.press(key);

  //Set info about last pressed button
  lastPressedKey = key;
}

