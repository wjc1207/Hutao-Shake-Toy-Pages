/*    Author: Junchi Wang（王骏驰）
      Student ID: 20012100013
      Project Name: Hutao Shake Toy

      Voice CMDs
      0 Hutao Quick
      1 Hutao slow
      2 shake quick
      3 shake slow
      4 quit quick
      5 quit slow
      6 quit [机械音]
      7 bright quick
      8 bright slow
      9 dark quick
      10 dark slow

                                   "Hutao"
                           ---------------------->    "Shake"       "quit"
      State Machine  :   0                        1 ----------> 2 ---------> 0
                           <----------------------
                                   “quit”

      0||1 : not work
      2 : work
*/
//libraries
#include <SoftwareSerial.h>
#include <VoiceRecognitionV3.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"

/**
  Connection
  Arduino    VoiceRecognitionModule
   2   ------->     TX
   3   ------->     RX
*/
VR myVR(2, 3); // 2:RX 3:TX, you can choose your favourite pins.

uint8_t records[7]; // save record
uint8_t buf[64];

uint8_t generalState = 0;
uint8_t lightState = 0; // 1 light, 0 dark
uint8_t UILineState = 3; // chosen line on OLED screen
uint8_t counterOLED = 0; // counter for OLED refresh
uint8_t counterSwitchXY = 0; // counter for rocker CD


uint8_t hutaoRGBVal[3] = {121, 2, 2};
float currentTemperature;
float currentATemperature;
float currentHumidity;
//define constants
const PROGMEM int fluxHigherLimit = 400;
const PROGMEM int fluxLowerLimit = 360;
//define Voice CMDs index
#define hutao1Cmd (0)
#define hutao2Cmd (1)
#define shake1Cmd (2)
#define shake2Cmd (3)
#define quit1Cmd (4)
#define quit2Cmd (5)
#define quit3Cmd (6)
#define bright1Cmd (7)
#define bright2Cmd (8)
#define dark1Cmd (9)
#define dark2Cmd (10)

#define initialState (0)
#define intermediateState (1)
#define workState (2)

#define selfCheckSuccessMode (2)
#define selfCheckErrorMode (3)

//define PINs
#define LIGHTSENSOR 0
#define X (1)
#define Y (2)
#define SWITCH (3)
#define MOTOR 6
#define BLUEPIN 9
#define GREENPIN 10
#define REDPIN 11
#define DHTPIN 12
#define DHTTYPE    DHT22
#define I2C_ADDRESS 0x3C
#define RST_PIN -1
SSD1306AsciiAvrI2c oled;
DHT_Unified dht(DHTPIN, DHTTYPE);
/**
  @brief   Print signature, if the character is invisible,
           print hexible value instead.
  @param   buf     --> command length
           len     --> number of parameters
*/
void printSignature(uint8_t *buf, int len)
{
  int i;
  for (i = 0; i < len; i++)
  {
    if (buf[i] > 0x19 && buf[i] < 0x7F)
    {
      Serial.write(buf[i]);
    }
    else
    {
      Serial.print(F("["));
      Serial.print(buf[i], HEX);
      Serial.print(F("]"));
    }
  }
}

/**
  @brief   Print signature, if the character is invisible,
           print hexible value instead.
  @param   buf  -->  VR module return value when voice is recognized.
             buf[0]  -->  Group mode(FF: None Group, 0x8n: User, 0x0n:System
             buf[1]  -->  number of record which is recognized.
             buf[2]  -->  Recognizer index(position) value of the recognized record.
             buf[3]  -->  Signature length
             buf[4]~buf[n] --> Signature
*/
void printVR(uint8_t *buf)
{
  Serial.println(F("VR Index\tGroup\tRecordNum\tSignature"));

  Serial.print(buf[2], DEC);
  Serial.print(F("\t\t"));

  if (buf[0] == 0xFF)
  {
    Serial.print(F("NONE"));
  }
  else if (buf[0] & 0x80)
  {
    Serial.print(F("UG "));
    Serial.print(buf[0] & (~0x80), DEC);
  }
  else
  {
    Serial.print(F("SG "));
    Serial.print(buf[0], DEC);
  }
  Serial.print(F("\t"));

  Serial.print(buf[1], DEC);
  Serial.print(F("\t\t"));
  if (buf[3] > 0)
  {
    printSignature(buf + 4, buf[3]);
  }
  else
  {
    Serial.print(F("NONE"));
  }
  Serial.println(F("\r\n"));
}

void setup()
{
  /** initialize */
  myVR.begin(9600);

  Serial.begin(115200);
  Serial.println(F("HutaoShakeToy"));

  // put your setup code here, to run once:
  // Initialize device.
  dht.begin();
  oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
  initialDisplay();

  //define PINs function
  pinMode(MOTOR, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(REDPIN, OUTPUT);

  records[0] = hutao1Cmd;
  records[1] = hutao2Cmd;
  records[2] = bright1Cmd;
  records[3] = bright2Cmd;
  records[4] = dark1Cmd;
  records[5] = dark2Cmd;


  if (myVR.clear() == 0)
  {
    Serial.println(F("Recognizer cleared."));
  }
  else
  {
    Serial.println(F("Not find VoiceRecognitionModule."));
    RGBLightColorChange(selfCheckErrorMode);
    delay(1000);
    while (1)
      ;
  }

  if (myVR.load(records, 7) >= 0) //load records
  {
    Serial.println(F("CMDs loaded"));
    RGBLightColorChange(selfCheckSuccessMode);
    delay(1000);
  }

}

uint8_t* RGBLightFluxChange(uint8_t hutaoRGBVal[3], int delta)//Control the RGB light lightness
{
  switch (hutaoRGBVal[0]) //40 means low lightness, 121 means medium lightness, 201 means high lightness
  {

    case 121:
      if (delta > 0) //delta > 0 means make the RGB light brighter.
      {
        hutaoRGBVal[0] = 201;
        hutaoRGBVal[1] = 3;
        hutaoRGBVal[2] = 3;
      }
      else //delta <= 0 means make the RGB light darker.
      {
        hutaoRGBVal[0] = 40;
        hutaoRGBVal[1] = 1;
        hutaoRGBVal[2] = 1;
      }
      break;
    case 201:
      if (delta <= 0)
      {
        hutaoRGBVal[0] = 121;
        hutaoRGBVal[1] = 2;
        hutaoRGBVal[2] = 2;
      }
      break;
    case 40:
      if (delta > 0)
      {
        hutaoRGBVal[0] = 121;
        hutaoRGBVal[1] = 2;
        hutaoRGBVal[2] = 2;
      }
      break;

    default: //default is the medium light
      hutaoRGBVal[0] = 121;
      hutaoRGBVal[1] = 2;
      hutaoRGBVal[2] = 2;
      break;
  }
  return hutaoRGBVal;
}
void RGBLightColorChange(uint8_t mode)//Control the behavior and Color of RGB light, drive the RGB light
{
  uint8_t RGBVal[3];
  // Check if the light sensor reading is higher than the upper limit
  if (analogRead(LIGHTSENSOR) > fluxHigherLimit) {
    // Set the light state to 1 (bright)
    lightState = 1;
  }
  // If the light sensor reading is not higher than the upper limit,
  // check if it is lower than the lower limit
  else if (analogRead(LIGHTSENSOR) < fluxLowerLimit) {
    // Set the light state to 0 (dark)
    lightState = 0;
  }
  if (mode == 0)
  {
    if (lightState) // dark --> light on
    {
      RGBVal[2] = hutaoRGBVal[2];
      RGBVal[1] = hutaoRGBVal[1];
      RGBVal[0] = hutaoRGBVal[0];
    }
    else // bright --> light off
    {
      RGBVal[2] = 0;
      RGBVal[1] = 0;
      RGBVal[0] = 0;
    }
  }
  else if (mode == 1)
  {

    if (lightState) // dark --> blink
    {
      if (millis() / 200 % 2 == 0)
      {
        RGBVal[2] = hutaoRGBVal[2];
        RGBVal[1] = hutaoRGBVal[1];
        RGBVal[0] = hutaoRGBVal[0];
      }
      else
      {
        RGBVal[2] = 0;
        RGBVal[1] = 0;
        RGBVal[0] = 0;
      }
    }
    else // bright --> light off
    {
      RGBVal[2] = 0;
      RGBVal[1] = 0;
      RGBVal[0] = 0;
    }
  }
  else if (mode == 2)
  {
    RGBVal[2] = 0;
    RGBVal[1] = 255;
    RGBVal[0] = 0;
  }
  else if (mode == 3)
  {
    RGBVal[2] = 0;
    RGBVal[1] = 0;
    RGBVal[0] = 255;
  }
  //drive the RGB light                      E.X.
  analogWrite(BLUEPIN, 255 - RGBVal[2]);  // blue 255 - 26
  analogWrite(GREENPIN, 255 - RGBVal[1]); // green  255 - 26
  analogWrite(REDPIN, 255 - RGBVal[0]);   // red 255 - 151
}
void printState() //Send the state to the upper machine by UART
{
  Serial.print(F("MEG,"));
  Serial.print((String)generalState);
  Serial.print(F(","));
  Serial.print((String)hutaoRGBVal[0]);
  Serial.print(F(","));
  Serial.print((String)currentATemperature);
  Serial.print(F(","));
  Serial.print((String)currentHumidity);
  Serial.print(F("\0"));
}
void toIntermediateState() //transition to IntermediateState
{
  generalState = intermediateState;
  myVR.clear();
  records[0] = shake1Cmd;
  records[1] = shake2Cmd;
  records[2] = bright1Cmd;
  records[3] = bright2Cmd;
  records[4] = dark1Cmd;
  records[5] = dark2Cmd;
  myVR.load(records, 7);
}
void toWorkState() //transition to WorkState
{
  generalState = workState;
  myVR.clear();
  records[0] = quit1Cmd;
  records[1] = quit2Cmd;
  records[2] = quit3Cmd;
  records[3] = bright1Cmd;
  records[4] = bright2Cmd;
  records[5] = dark1Cmd;
  records[6] = dark2Cmd;
  myVR.load(records, 7);
}
void toInitialState()//transition to InitialState
{
  generalState = initialState;
  myVR.clear();
  records[0] = hutao1Cmd;
  records[1] = hutao2Cmd;
  records[2] = bright1Cmd;
  records[3] = bright2Cmd;
  records[4] = dark1Cmd;
  records[5] = dark2Cmd;
  myVR.load(records, 7);
}
void voiceRecognition() //Control by Voice CMDs
{
  int ret;
  uint8_t* hutaoRGBValCopy;
  ret = myVR.recognize(buf, 50); //Voice Recognizition
  if (ret > 0)
  {
    switch (generalState)
    { // buf[1]
      case (initialState):
        {
          if (buf[1] == hutao1Cmd or buf[1] == hutao2Cmd)
            //if the Voice CMD is "Hutao"
          {
            toIntermediateState();
          }
          break;
        }

      case (intermediateState):
        {
          if (buf[1] == shake1Cmd or buf[1] == shake2Cmd)
            //if the Voice CMD is "Shake"
          {
            toWorkState();
          }
          break;
        }

      case (workState):
        {
          if (buf[1] == quit1Cmd or buf[1] == quit2Cmd or buf[1] == quit3Cmd)
          { //if the Voice CMD is "quit"
            toInitialState();
          }
          break;
        }
    }

    if (buf[1] == bright1Cmd or buf[1] == bright2Cmd)
    {
      hutaoRGBValCopy = RGBLightFluxChange(hutaoRGBVal, 1);
      hutaoRGBVal[0] = hutaoRGBValCopy[0];
      hutaoRGBVal[1] = hutaoRGBValCopy[1];
      hutaoRGBVal[2] = hutaoRGBValCopy[2];
    }
    else if (buf[1] == dark1Cmd or buf[1] == dark2Cmd)
    {
      hutaoRGBValCopy = RGBLightFluxChange(hutaoRGBVal, -1);
      hutaoRGBVal[0] = hutaoRGBValCopy[0];
      hutaoRGBVal[1] = hutaoRGBValCopy[1];
      hutaoRGBVal[2] = hutaoRGBValCopy[2];
    }
    /** voice recognized */
    stateDisplay();
  }
}
void switchXY() //control by rocker
{
  uint8_t* hutaoRGBValCopy;
  if (counterSwitchXY == 5)
  {
    if (UILineState == 3)
    {
      if ((analogRead(X) > 900 or analogRead(X) < 100)) //if the rocker operation is up or down
      {
        UILineState = 4;
        stateDisplay();

      }
      else if (analogRead(Y) < 200) //if the rocker operation is right
      {
        hutaoRGBValCopy = RGBLightFluxChange(hutaoRGBVal, 1);
        hutaoRGBVal[0] = hutaoRGBValCopy[0];
        hutaoRGBVal[1] = hutaoRGBValCopy[1];
        hutaoRGBVal[2] = hutaoRGBValCopy[2];
        stateDisplay();

      }
      else if (analogRead(Y) > 800) //if the rocker operation is left
      {
        hutaoRGBValCopy = RGBLightFluxChange(hutaoRGBVal, -1);
        hutaoRGBVal[0] = hutaoRGBValCopy[0];
        hutaoRGBVal[1] = hutaoRGBValCopy[1];
        hutaoRGBVal[2] = hutaoRGBValCopy[2];
        stateDisplay();

      }
    }
    else if (UILineState == 4)
    {
      if (analogRead(X) > 900 or analogRead(X) < 100) //if the rocker operation is up or down
      {
        UILineState = 3;
        stateDisplay();
      }
      else
      {
        switch (generalState)
        { // buf[1]
          case (initialState):
            {
              if (analogRead(Y) < 200 or analogRead(Y) > 800) //if the rocker operation is right or left
              {
                toWorkState();
                stateDisplay();
              }
              break;
            }
          case (intermediateState):
            {
              if (analogRead(Y) < 200 or analogRead(Y) > 800) //if the rocker operation is right or left
              {
                toWorkState();
                stateDisplay();
              }
              break;
            }
          case (workState):
            {
              if (analogRead(Y) < 200 or analogRead(Y) > 800) //if the rocker operation is right or left
              { // quit
                toInitialState();
                stateDisplay();
              }
              break;
            }
        }


      }
    }
    counterSwitchXY = 0;
  }

}
void executeUpperCMD() //contorl by upper machine
{
  uint8_t* hutaoRGBValCopy;
  String message; //String is better than char*
  String CMD_arr[3];
  String tem = "";
  uint8_t j = 0;
  if (Serial.available())
  {
    message = Serial.readStringUntil('\n');
    message.trim(); // remove leading and trailing whitespace
    for (int i = 0; i < message.length(); i++)
    {
      if (message.charAt(i) == ',')
      {
        CMD_arr[j] = message.substring(0, i);
        message.remove(0, i + 1);
        j++;
        i = 0;
      }
    }
    CMD_arr[j] = message; // 添加最后一个子字符串
    if (CMD_arr[0] == "CMD")
    {
      if (CMD_arr[1] == "L")
      {
        if (CMD_arr[2] == "1")
        {
          hutaoRGBValCopy = RGBLightFluxChange(hutaoRGBVal, 1);
          hutaoRGBVal[0] = hutaoRGBValCopy[0];
          hutaoRGBVal[1] = hutaoRGBValCopy[1];
          hutaoRGBVal[2] = hutaoRGBValCopy[2];
          stateDisplay();

        }
        else if (CMD_arr[2] == "-1")
        {
          hutaoRGBValCopy = RGBLightFluxChange(hutaoRGBVal, -1);
          hutaoRGBVal[0] = hutaoRGBValCopy[0];
          hutaoRGBVal[1] = hutaoRGBValCopy[1];
          hutaoRGBVal[2] = hutaoRGBValCopy[2];
          stateDisplay();

        }
      }
      if (CMD_arr[1] == "M")
      {
        switch (generalState)
        { // buf[1]
          case (initialState):
            {
              if (CMD_arr[2] == "1")
              {
                toWorkState();
                stateDisplay();
              }
              break;
            }
          case (intermediateState):
            {
              if (CMD_arr[2] == "1")
              {
                toWorkState();
                stateDisplay();
              }
              break;
            }
          case (workState):
            {
              if (CMD_arr[2] == "0")
              { // quit
                toInitialState();
                stateDisplay();
              }
              break;
            }
        }
      }
    }
  }

}



void loop()
{
  switchXY(); //control by a physical rocker
  executeUpperCMD(); //control by upper machine CMD
  voiceRecognition(); //control by Voice
  switch (generalState) //Accoring to the state, drive the motor(Hutao Shake Toy)
  {
    case (initialState):
      {
        digitalWrite(MOTOR, LOW);
        RGBLightColorChange(0);
        break;
      }
    case (intermediateState):
      {
        digitalWrite(MOTOR, LOW); // maxCurrent = 20 mA
        RGBLightColorChange(0);
        break;
      }
    case (workState):
      {
        digitalWrite(MOTOR, HIGH);
        // RGB light
        RGBLightColorChange(1);
        break;
      }
    default:
      {
        digitalWrite(MOTOR, LOW);
        RGBLightColorChange(0);
        break;
      }
  }
  //OLED counter
  counterOLED = counterOLED + 1;
  if ((int)counterOLED == 5) //OLED refresh time = 5 * LoopTime
  {
    stateDisplay();
    counterOLED = 0;
  }
  //Switch counter
  if ((int)counterSwitchXY < 5) //rocker control CD = 5 * LoopTime
  {
    counterSwitchXY = counterSwitchXY + 1;
  }
  else
  {
    counterSwitchXY = 5;
  }


}

// Define a function to display the current state on the OLED screen and upper machine screen
void stateDisplay() {
  // Clear the screen and set the font to Callibri11
  oled.clear();
  oled.setFont(Callibri11);

  // Set the font color to white for better visibility and set the display to 1x
  oled.set1X();

  // Read the temperature and humidity using the DHT sensor
  sensors_event_t event;
  float e;
  dht.temperature().getEvent(&event);
  if (!isnan(event.temperature)) {
    currentTemperature = event.temperature;
  }
  dht.humidity().getEvent(&event);
  if (!isnan(event.relative_humidity)) {
    currentHumidity = event.relative_humidity;
  }
  e = (currentHumidity / 100) * 6.105 * exp((17.27 * currentTemperature) / (237.7 + currentTemperature));
  currentATemperature = 1.07 * currentTemperature + 0.2 * e - 2.7;

  // Display the temperature and humidity readings on the OLED screen
  oled.print(F("ATempera:   "));
  oled.print(currentATemperature);
  oled.println(F("C"));
  oled.print(F("Humidity:     "));
  oled.print(currentHumidity);
  oled.println(F("%"));

  // Display the brightness level on the OLED screen
  if (UILineState == 3)
    oled.setInvertMode(1);

  oled.print(F("Brightness:  "));
  if (hutaoRGBVal[0] == 201)
    oled.println(F("---------"));
  else if (hutaoRGBVal[0] == 121)
    oled.println(F("------"));
  else
    oled.println(F("---"));

  oled.println(F(""));
  oled.setInvertMode(0);

  // Display the current state of the toy on the OLED screen
  if (UILineState == 4)
    oled.setInvertMode(1);
  if (generalState == initialState)
    oled.println(F("ZZZ"));
  else if (generalState == intermediateState)
    oled.println(F("Hutao Heard you!"));
  else
    oled.println(F("Shake! Shake!"));
  oled.setInvertMode(0);

  // Print the current state to the upper machine screen
  printState();
}

// Define a function to display a welcome message on the OLED screen
void initialDisplay() {
  // Clear the screen
  oled.clear();

  // Set the font to Callibri14
  oled.setFont(Callibri14);


  // Print the welcome message on the screen using println()
  oled.println();
  oled.println(("      Hutao Shake Toy"));
  oled.println(("           Welcome!"));
}
