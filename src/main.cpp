#include <Arduino_GFX_Library.h>
#include <Arduino.h>

//Parameters for Screen
#define TFT_CS 5
#define TFT_DC 27
#define TFT_RST 33
#define TFT_SCK 18
#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_LED 22
#define E1_OUT_A 26
#define E1_OUT_B 25
#define E1_PUSH 32
#define E2_OUT_A 12
#define E2_OUT_B 14
#define E2_PUSH 21
#define MENU_BUTTON 13
#define CS_SD 15
#define MOST_SD 5
#define MISO_SD 2
#define CLK_SD 35


/* More dev device declaration: https://github.com/moononournation/Arduino_GFX/wiki/Dev-Device-Declaration */
#if defined(DISPLAY_DEV_KIT)
Arduino_GFX *gfx = create_default_Arduino_GFX();
#else /* !defined(DISPLAY_DEV_KIT) */

/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
//Arduino_DataBus *bus = create_default_Arduino_DataBus();
Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO, VSPI /* spi_num */);

/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */
Arduino_GFX *gfx = new Arduino_ILI9341(bus, TFT_RST, 1 /* rotation */, false /* IPS */);
//Arduino_GC9A01 *gfx = new Arduino_GC9A01(bus, TFT_RST, 0 /* rotation */, true /* IPS */);

#endif /* !defined(DISPLAY_DEV_KIT) */
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

//Rotary Encoder Settings
unsigned long _lastIncReadTime = micros();
unsigned long _lastDecReadTime = micros();
int _pauseLength = 25000;
int _fastIncrement = 10;


char gradeLevelName[18] = {'C', 'h', 'o', 'o', 's', 'e', 0x20, 'G', 'r', 'a', 'd', 'e', 0x20, 'L', 'e', 'v', 'e', 'l'};
//bounds depending on grade level
//defualt upper bounds (5th Grade)
int addUpperBound = 99;
int divUpperBound = 15;
int subUpperBound = 99;
int multUpperBound = 10;

int counter = 0;
int counter_2 = 0;

bool push = 0;
bool lastPush = false;

//encoder controls 
void read_encoder();
void read_encoder_2();
void read_push();

//Mode Settings
short mode = 0;
//0 is Splash Screen, 1 is Game Selection, 2 is Addition Mode, 3 is Subtraction Mode, 4 is Multiplicaiotn, 5 is Division, 6 is Quiz Mode

//Text Game Selection and initial values 
char addName[3] = {'A', 'd', 'd'};
short sum = 0;
short addNum1 = 0;
short addNum2 = 0;

char subName[8] = {'S', 'u', 'b', 't', 'r', 'a', 'c', 't'};
short difference = 0;
short subNum1 = 0;
short subNum2 = 0;

char multiName[8] = {'M', 'u', 'l', 't', 'i', 'p', 'l', 'y'};
short product = 0;
short multNum1 = 0;
short multNum2 = 0;

char divName[8] = {'D', 'i', 'v', 'i', 's', 'i', 'o', 'n'};
char quizName[4] = {'Q', 'u', 'i', 'z'};
short quotient = 0;
short divNum1 = 0;
short divNum2 =0;

short menu1 = 0;

short quizMode = 0;

short currentScore = 0;

bool lastMenu = 0;

//Cursor Variable
short x = 0;
short y = 0;


//Funciton Defintiions
void A_CHANGE_1(); 
void A_CHANGE_2();
void Push_1();
void Push_2();
void updateScreen(bool forward1, bool backwards1, bool forward2, bool backward2, bool push, bool back);
uint16_t randomColor();
void squareTransition(uint16_t color);
void chooseGradeLevel(int push, int back);


//Reset Numbers
void resetAdd();
void resetSub();
void resetMult();
void resetDiv();
void resetQuiz();

//Mode function defintions
void addMode(int push, int back);
void subtractMode(int push, int back);
void multiplyMode(int push, int back);
void divideMode(int push, int pubacksh2);

void setup() {
pinMode(TFT_LED, OUTPUT);
digitalWrite(TFT_LED, HIGH);

//Setup Screen
gfx->begin();
gfx->fillScreen(WHITE);
Serial.begin(9600);

#ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
#endif
  //Finalize screen setup and do opening menu screen
    gfx->setTextSize(5, 5);
    gfx->setCursor(20, gfx->height()/2-5);
    char Title[10] = {'M', 'a', 't', 'h', 0x20, 'Q', 'u', 'e', 's', 't'};
    for (int i = 0; i < 10; i++) {
      gfx->setTextColor(randomColor());
      gfx->print(Title[i]);
      delay(100);
    }

    delay(1000);

// Display opening screen menu
gfx->fillScreen(WHITE);
gfx->setCursor(20, gfx->height()/2-5);
gfx->print("Math Quest");
gfx->setCursor(20, gfx->height()/2+50);
gfx->setTextSize(2, 2);
delay(2000);
updateScreen(0, 0, 0, 0, 1, 0);

// Set encoder pins and attach interrupts
pinMode(E1_OUT_A, INPUT_PULLUP);
pinMode(E1_OUT_B, INPUT_PULLUP);
pinMode(E2_OUT_A, INPUT_PULLUP);
pinMode(E2_OUT_B, INPUT_PULLUP);
pinMode(E1_PUSH, INPUT_PULLUP);
pinMode(E2_PUSH, INPUT_PULLUP);
pinMode(MENU_BUTTON, INPUT_PULLUP);
attachInterrupt(digitalPinToInterrupt(E1_OUT_A), read_encoder, CHANGE);
attachInterrupt(digitalPinToInterrupt(E1_OUT_B), read_encoder, CHANGE);
attachInterrupt(digitalPinToInterrupt(E2_OUT_A), read_encoder_2, CHANGE);
attachInterrupt(digitalPinToInterrupt(E2_OUT_B), read_encoder_2, CHANGE);
}

void loop()
{
  static int lastCounter = 0;
  static int lastCounter_2 = 0;
  // If count has changed print the new value to serial
  if(counter != lastCounter){
    if (counter < 0) counter = 0;
    if (counter > 99) counter = 99;
    if (counter < lastCounter) updateScreen(1, 0, 0, 0, 0, 0);
    if (counter > lastCounter) updateScreen(0, 1, 0, 0, 0, 0);
    lastCounter = counter;
  }

  if(counter_2 != lastCounter_2){
    if (counter_2 < 0) counter_2 = 0;
    if (counter_2 > 99) counter_2 = 99;
    if (counter_2 < lastCounter_2) updateScreen(0, 0, 1, 0, 0, 0);
    if (counter_2 > lastCounter_2) updateScreen(0, 0, 0, 1, 0, 0);
    lastCounter_2 = counter_2;
  }

  bool push = !digitalRead(E1_PUSH) | !digitalRead(E2_PUSH);
  bool menu_button = !digitalRead(MENU_BUTTON);
  Serial.print(push);
  Serial.print("     ");
  Serial.println(menu_button);

  if (push && !lastPush) {
    updateScreen(0, 0, 0, 0, 1, 0);
    lastPush = true;
  }
  else if (!push && lastPush) {
    lastPush = false;
  }

  if (menu_button) {
    updateScreen(0, 0, 0, 0, 0, 1);
  }
}

void updateScreen(bool forward1, bool backwards1, bool forward2, bool backward2, bool push, bool back) {
  switch(mode) {
    //Splash Screen
    case 0:
      if (push == 1 || back == 1) {
        gfx->fillScreen(WHITE);
        mode = 1;
      }
      break;
  //Main Menu
   case 1:
    if (forward1 || forward2) {
      menu1++;
      if (menu1 > 6) menu1 = 0;
    }
    else if(backwards1 || backward2) {
      menu1--;
      if (menu1 < 0) menu1 = 6;
    } 
      switch(menu1) {
        case 0:
          gfx->fillScreen(RED);
          gfx->setTextColor(WHITE);
          gfx->setCursor(gfx->width()/2 - 30, gfx->height()/2+5);
          gfx->print("Add");
          if (push) {
            gfx->fillScreen(RED);
            gfx->setTextColor(WHITE);
            resetAdd();
            mode = 2;
          }
          break;
        case 1:
          gfx->fillScreen(BLUE);
          gfx->setTextColor(WHITE);
          gfx->setCursor(gfx->width()/2 - 50, gfx->height()/2+5);
          gfx->print("Subtract");
          if (push) {
            gfx->fillScreen(BLUE);
            gfx->setTextColor(WHITE);
            resetSub();
            mode = 3;
          }
          break;
        case 2:
          gfx->fillScreen(GREEN);
          gfx->setTextColor(BLACK);
          gfx->setCursor(gfx->width()/2 - 50, gfx->height()/2+5);
          gfx->print("Multiply");
          if (push) {
            gfx->fillScreen(GREEN);
            gfx->setTextColor(BLACK);
            resetMult();
            mode = 4;
          }
          break;
        case 3:
          gfx->fillScreen(PURPLE);
          gfx->setTextColor(WHITE);
          gfx->setCursor(gfx->width()/2 - 50, gfx->height()/2+5);
          gfx->print("Division");
          if (push) {
            gfx->fillScreen(PURPLE);
            gfx->setTextColor(WHITE);
            resetDiv();
            mode = 5;
          }
          break;
        case 4:
          gfx->fillScreen(BLACK);
          gfx->setTextColor(WHITE);
          gfx->setCursor(gfx->width()/2 - 50, gfx->height()/2+5);
          gfx->print("Quiz");
          if (push) {
            gfx->fillScreen(BLACK);
            gfx->setTextColor(WHITE);
            resetQuiz();
            mode = 6;
          }
          break;
          case 5:
          gfx->fillScreen(RED);
          gfx->setTextColor(WHITE);
          gfx->setCursor(gfx->width()/2 - 50, gfx->height()/2+5);
          gfx->print(gradeLevelName);
          if (push) {
            gfx->fillScreen(RED);
            gfx->setTextColor(WHITE);
            
            mode = 7;
          }
          break;
      
      }
      break;
    //Additon
    case 2:
      gfx->fillScreen(RED);
      gfx->setTextColor(WHITE);
      addMode(push, back);
      break;
    //Subtraction
    case 3:
      gfx->fillScreen(BLUE);
      gfx->setTextColor(WHITE);
      subtractMode(push, back);
      break;
    //Multiplication
    case 4:
      gfx->fillScreen(GREEN);
      gfx->setTextColor(BLACK);
      multiplyMode(push, back);
      break;
    //Division
    case 5:
      gfx->fillScreen(PURPLE);
      gfx->setTextColor(WHITE);
      divideMode(push, back);
      break;
    //Quiz Mode
    case 6:
      gfx->fillScreen(BLACK);
      gfx->setTextColor(WHITE);
      switch(quizMode) {
        //switch between types of questions 
        case 0:
          addMode(push, back);
          break;
        case 1:
          subtractMode(push, back);
          break;
        case 2:
          multiplyMode(push, back);
          break;
        case 3:
          divideMode(push, back);
          break;
      }

      break;
      case 7:
        gfx->fillScreen(RED);
        gfx->setTextColor(WHITE);
        chooseGradeLevel(push, back);
      break;
  
  }
}

uint16_t randomColor() {
  short red = random(0, 255);
  short green = random(0, 255);
  short blue = random(0, 255);
  
  return ( ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3) );
}

void squareTransition(uint16_t color) {
  return; //may or may not be implemented
}
//reset functions 
void resetAdd() {
  //set current score 
  if (mode != 2) {
    currentScore = 0;
  }
  //calcualte the total sum, then use as upper limit for the addends
  sum = random(0, addUpperBound);
  addNum1 = random(0, sum);
  addNum2 = sum - addNum1;
  //reset encoders
  counter = 0;
  counter_2 = 0;
}

void resetSub() {
  //set current score 
  if (mode != 3) {
    currentScore = 0;
  }
  //determine the subtrahend and minuend 
  subNum1 = random(0, subUpperBound);
  subNum2 = random(0, subNum1);
  //calculate the correct answer
  difference = subNum1 - subNum2;
  //reset encoders
  counter = 0;
  counter_2 = 0;
}

void resetMult() {
  //set current score 
  if (mode != 4) {
    currentScore = 0;
  }
  //determine multipliers with grade level upper bound
  multNum1 = random(0, multUpperBound);
  multNum2 = random(0, multUpperBound);
  //determine the correct answer
  product = multNum1 * multNum2;
  //reset encoders
  counter = 0;
  counter_2 = 0;
}

void resetDiv() {
  //set current score 
  if (mode != 5) {
    currentScore = 0;
  }
  //determine the quotient and divisor from grade level upper bound 
  divNum2 = random(0, divUpperBound);
  quotient = random(0, divUpperBound);
  //use known values to calculate the dividend 
  divNum1 = divNum2 * quotient;
  //reset encoders 
  counter = 0;
  counter_2 = 0;
}

void resetQuiz() {
  //set current score 
  if (mode != 6) {
    currentScore = 0;
  }
  //confetti if question is correct 
  if (currentScore == 1){
    confetti();
  
}
//randomly switch between mode questions
  quizMode = random(0, 3);
  switch(quizMode) {
    case 0:
      resetAdd();
      break;
    case 1:
      resetSub();
      break;
    case 2:
      resetMult();
      break;
    case 3:
      resetDiv();
  }
}

void addMode(int push, int back) {
  gfx->setTextSize(3,3);
  gfx->setCursor(0, 50);
  gfx->print("Score: ");
  gfx->print(currentScore);
  gfx->setCursor(0, gfx->height()/2);
  gfx->print(addNum1); gfx->print(" + "); gfx->print(addNum2); gfx->print(" = ");
  if (counter != 0) gfx->print(counter % 10);
  else gfx->print(" ");
  gfx->print(counter_2 % 10);
  if(push) {
    short calculatedSum = (counter%10)*10 + counter_2%10;
    if (sum == calculatedSum) {
      gfx->setCursor(10, 10);
      gfx->setTextSize(3, 3);
      gfx->print("CORRECT!");
      confetti();
      gfx->setTextSize(5,5);
      currentScore++;
      if (mode == 6) resetQuiz();
      else resetAdd();
      delay(1000);
      updateScreen(0,0,0,0,0,0);
    } else {
      gfx->setCursor(10, 10);
      gfx->setTextSize(3, 3);
      gfx->print("WRONG!");
      gfx->setTextSize(5,5);
      delay(1000);
      updateScreen(0,0,0,0,0,0);
    }
  }
  if (back) {
    mode = 1;
    currentScore = 0;
    updateScreen(0,0,0,0,0,0);
  }
}
void subtractMode(int push, int back) {
  gfx->setTextSize(3,3);
  gfx->setCursor(0, 50);
  gfx->print("Score: ");
  gfx->print(currentScore);
  gfx->setCursor(0, gfx->height()/2);
  gfx->print(subNum1); gfx->print(" - "); gfx->print(subNum2); gfx->print(" = ");
  if (counter != 0) gfx->print(counter % 10);
  else gfx->print(" ");
  gfx->print(counter_2 % 10);
  if(push) {
    short calculatedDifference = (counter%10)*10 + counter_2%10;
    if (difference == calculatedDifference) {
      gfx->setCursor(10, 10);
      gfx->setTextSize(3, 3);
      gfx->print("CORRECT!");
      confetti();
      gfx->setTextSize(5,5);
      currentScore++;
      if (mode == 6) resetQuiz();
      else resetSub();
      delay(1000);
      updateScreen(0,0,0,0,0,0);
    } else {
      gfx->setCursor(10, 10);
      gfx->setTextSize(3, 3);
      gfx->print("WRONG!");
      gfx->setTextSize(5,5);
      delay(1000);
      updateScreen(0,0,0,0,0,0);
    }
  }
  if (back) {
    mode = 1;
    currentScore = 0;
    updateScreen(0,0,0,0,0,0);
  }
}
void multiplyMode(int push, int back) {
  gfx->setTextSize(3,3);
  gfx->setCursor(0, 50);
  gfx->print("Score: ");
  gfx->print(currentScore);
  gfx->setCursor(0, gfx->height()/2);
  gfx->print(multNum1); gfx->print(" * "); gfx->print(multNum2); gfx->print(" = ");
  if (counter != 0) gfx->print(counter % 10);
  else gfx->print(" ");
  gfx->print(counter_2 % 10);
  if(push) {
    short calculatedProduct = (counter%10)*10 + counter_2%10;
    if (product == calculatedProduct) {
      gfx->setCursor(10, 10);
      gfx->setTextSize(3, 3);
      gfx->print("CORRECT!");
      confetti();
      gfx->setTextSize(5,5);
      currentScore++;
      if (mode == 6) resetQuiz();
      else resetMult();
      delay(1000);
      updateScreen(0,0,0,0,0,0);
    } else {
      gfx->setCursor(10, 10);
      gfx->setTextSize(3, 3);
      gfx->print("WRONG!");
      gfx->setTextSize(5,5);
      delay(1000);
      updateScreen(0,0,0,0,0,0);
    }
  }
  if (back) {
    mode = 1;
    currentScore = 0;
    updateScreen(0,0,0,0,0,0);
  }
}
void divideMode(int push, int back) {
  gfx->setTextSize(3,3);
  gfx->setCursor(0, 50);
  gfx->print("Score: ");
  gfx->print(currentScore);
  gfx->setCursor(0, gfx->height()/2);
  gfx->print(divNum1); gfx->print(" ÷ "); gfx->print(divNum2); gfx->print(" = ");
  if (counter != 0) gfx->print(counter % 10);
  else gfx->print(" ");
  gfx->print(counter_2 % 10);
  if(push) {
    short calculatedQuotient = (counter%10)*10 + counter_2%10;
    if (quotient == calculatedQuotient) {
      gfx->setCursor(10, 10);
      gfx->setTextSize(3, 3);
      gfx->print("CORRECT!");
      confetti();
      gfx->setTextSize(5,5);
      currentScore++;
      if (mode == 6) resetQuiz();
      else resetDiv();
      delay(1000);
      updateScreen(0,0,0,0,0,0);
    } else {
      gfx->setCursor(10, 10);
      gfx->setTextSize(3, 3);
      gfx->print("WRONG!");
      gfx->setTextSize(5,5);
      delay(1000);
      updateScreen(0,0,0,0,0,0);
    }
  }
  if (back) {
    mode = 1;
    currentScore = 0;
    updateScreen(0,0,0,0,0,0);
  }
}

void read_encoder() {
  // Encoder interrupt routine for both pins. Updates counter
  // if they are valid and have rotated a full indent
 
  static uint8_t old_AB = 3;  // Lookup table index
  static int8_t encval = 0;   // Encoder value  
  static const int8_t enc_states[]  = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0}; // Lookup table

  old_AB <<=2;  // Remember previous state

  if (digitalRead(E1_OUT_A)) old_AB |= 0x02; // Add current state of pin A
  if (digitalRead(E1_OUT_B)) old_AB |= 0x01; // Add current state of pin B
 
  encval += enc_states[( old_AB & 0x0f )];

  // Update counter if encoder has rotated a full indent, that is at least 4 steps
  if( encval > 3 ) {        // Four steps forward
    int changevalue = 1;
    if((micros() - _lastIncReadTime) < _pauseLength) {
      changevalue = _fastIncrement * changevalue;
    }
    _lastIncReadTime = micros();
    counter = counter + changevalue;              // Update counter
    encval = 0;
  }
  else if( encval < -3 ) {        // Four steps backward
    int changevalue = -1;
    if((micros() - _lastDecReadTime) < _pauseLength) {
      changevalue = _fastIncrement * changevalue;
    }
    _lastDecReadTime = micros();
    counter = counter + changevalue;              // Update counter
    encval = 0;
  }
}

void read_encoder_2() {
  // Encoder interrupt routine for both pins. Updates counter
  // if they are valid and have rotated a full indent
 
  static uint8_t old_AB = 3;  // Lookup table index
  static int8_t encval = 0;   // Encoder value  
  static const int8_t enc_states[]  = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0}; // Lookup table

  old_AB <<=2;  // Remember previous state

  if (digitalRead(E2_OUT_A)) old_AB |= 0x02; // Add current state of pin A
  if (digitalRead(E2_OUT_B)) old_AB |= 0x01; // Add current state of pin B
 
  encval += enc_states[( old_AB & 0x0f )];

  // Update counter if encoder has rotated a full indent, that is at least 4 steps
  if( encval > 3 ) {        // Four steps forward
    int changevalue = 1;
    if((micros() - _lastIncReadTime) < _pauseLength) {
      changevalue = _fastIncrement * changevalue;
    }
    _lastIncReadTime = micros();
    counter_2 = counter_2 + changevalue;              // Update counter
    encval = 0;
  }
  else if( encval < -3 ) {        // Four steps backward
    int changevalue = -1;
    if((micros() - _lastDecReadTime) < _pauseLength) {
      changevalue = _fastIncrement * changevalue;
    }
    _lastDecReadTime = micros();
    counter_2 = counter_2 + changevalue;              // Update counter
    encval = 0;
  }
}
void chooseGradeLevel(int push, int back) {
  //setting the bounds for each problem 
  gfx->setCursor(0, gfx->height()/2);
  gfx->print("Input grade Level 1-5: ");
  if (counter != 0) gfx->print(counter % 5 + 1);
  else gfx->print(" ");
  if(push) {
    short gradeLevel = counter % 5 + 1;
    switch(gradeLevel) {
      case 1: 
      //first grade limits
        addUpperBound = 20;
        subUpperBound = 20;
        break;
      case 2: 
        addUpperBound = 40;
        subUpperBound = 40;
        break;
      case 3: 
        addUpperBound = 60;
        subUpperBound = 60;
        multUpperBound = 12;
        divUpperBound = 12;
        break;
      case 4: 
        addUpperBound = 80;
        subUpperBound = 80;
        multUpperBound = 13;
        divUpperBound = 13;
        break; 
      case 5:
        addUpperBound = 99;
        subUpperBound = 99;
        multUpperBound = 15;
        divUpperBound = 15;
        break;
    }

    mode = 1;
    updateScreen(0,0,0,0,0,0);
  
  }
  else if(back){
    mode = 1;
    updateScreen(0,0,0,0,0,0);
  }
}
void confetti(){
  //display randomized confetti then reset screen
  for (int i = 0; i < 100; i++) { // draw 100 confetti pieces
    int x = random(0, gfx->width());
    int y = random(0, gfx->height()); 
    int radius = random(2, 5);
    uint16_t color = random(0xFFFF);
    
    gfx->fillCircle(x, y, radius, color);
    delay(10); 
  }
  delay(1000);  //hold them for one second 
  gfx->fillScreen(BLACK); //clear the screen for the next "explosion"
}