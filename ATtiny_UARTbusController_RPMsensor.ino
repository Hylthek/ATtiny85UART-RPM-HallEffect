//global space=================================================================================================
//header files-------------------------------------------------------------------------------------------------
#include <SoftwareSerial.h>
//notes--------------------------------------------------------------------------------------------------------
//note: to program a new ATtiny, you must set "Clock:" to "8 MHz (internal)" and then press "Burn Bootloader"
//IMPORTANT: DO NOT BURN BOOTLOADER WHILE CLOCK IS SET TO "<#>MHz (external)" THIS WILL BRICK THE CHIP
//consts and objects-------------------------------------------------------------------------------------------
const int pinTransmitFlag = 0; //bool output pin (LOW = read, HIGH = transmit) (also LED pin)
const int pinTxD = 1; //serial output pin
const int pinRxD = 2; //serial input pin
const int pinSensor1 = 3;
const int pinSensor2 = 4;
SoftwareSerial mySerial(pinRxD, pinTxD, false);
const char EOT = 0; //"EndOfTransmission"
//changable consts---------------------------------------------------------------------------------------------
const int ID = 1; //first 16ish ID bytes (0x1-0x10) are reserved so that data is not interpreted as ID bytes
const int MAX_ARRAY_SIZE = 2;
const int stuffingCutoff = 1000; //cutoff time until code starts stuffing zeros (mSec)
const int nodesPerRotation = 12; //the number of times the hall effect sensor triggers in one revolution
//global variables---------------------------------------------------------------------------------------------
int debugCounter = 1;
bool prevSensorState = 0; //previous state of the rpm sensor
unsigned long prevMilliseconds = 0; //time since last sample (mSec)
unsigned long prevMicroseconds = 0; //time since last sample (uSec)
//setup function===============================================================================================
void setup() { // put your setup code here, to run once:
  pinMode (pinRxD, INPUT_PULLUP);
  pinMode (pinTxD, OUTPUT);
  pinMode (pinTransmitFlag, OUTPUT);
  pinMode (pinSensor1, INPUT_PULLUP);
  pinMode (pinSensor2, INPUT_PULLUP);
  digitalWrite(pinTransmitFlag, LOW);
  mySerial.begin(9600);
  // ^^(baud rate)^^ for this number to be accurate, ATtiny MUST be set at "8 MHz(internal)" clock speed
  mySerial.listen(); //marks this serial's reciever as the active one in the chip
}
//loop function================================================================================================
void loop() { // put your main code here, to run repeatedly:
  //set "curr" variables---------------------------------------------------------------------------------------
  unsigned long currMilliseconds = millis();
  unsigned long currMicroseconds = micros();
  bool currSensorState = digitalRead(pinSensor1);
  //calculate RPM----------------------------------------------------------------------------------------------
  if (currSensorState != prevSensorState) {
    prevSensorState = digitalRead(pinSensor1);
    if(currSensorState == LOW) {
      //calculate- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      unsigned long x = currMicroseconds - prevMicroseconds;
      unsigned long RPM = (60000000 / (x * nodesPerRotation)); // RpM = 60,000,000/x/NpR
      //create send array- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      unsigned long RPM_Array[MAX_ARRAY_SIZE];
      RPM_Array[0] = currMilliseconds; //total milliseconds
      RPM_Array[1] = RPM;
      dataSend(RPM_Array);
      //update "prev" variables - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      prevMilliseconds = millis();
      prevMicroseconds = micros();
      return;
    }
  }
  //zero stuffing----------------------------------------------------------------------------------------------
  if (currMilliseconds - prevMilliseconds > stuffingCutoff) {
    //create send data array- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    unsigned long dataToSend[MAX_ARRAY_SIZE];
    dataToSend[0] = currMilliseconds;
    dataToSend[1] = 0;
    dataSend(dataToSend);
    //update prevMilliseconds - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    prevMilliseconds = millis();
    return;
  }
}
//UART data sending function===================================================================================
void dataSend(const unsigned long valueArray[MAX_ARRAY_SIZE]) {
  // FORMAT: teensy (master device)        : <myID>------------...-------------<myID+1>-----<myID+2>-ect.
  // FORMAT: ATtiny (child device aka. me!): -------<data><data>...<data><EOT>-----------------------ect.
  //wait for correct ID byte-----------------------------------------------------------------------------------
  while (true) {
    while (mySerial.available() > 0)  //clear data queue
      mySerial.read();
    while (mySerial.peek() == -1) {}  //wait for ID byte
    char IdByte = mySerial.read();    //read ID byte
    if (IdByte == ID) {               //exit loop if ID is ours
      break;
    }
  }
  //send out data---------------------------------------------------------------------------------------------
  digitalWrite(pinTransmitFlag, HIGH);
  delayMicroseconds(100); //100us delay allows bus driver to enable bus driving in time for the start bit
  for (int i = 0; i < MAX_ARRAY_SIZE; i++) {
    mySerial.print(valueArray[i], DEC);  //sends the data out as ascii numbers (ie. '1' '2' '3')
    if (i < MAX_ARRAY_SIZE - 1) {
      mySerial.write(',');
    }
  }
  mySerial.write(EOT); //notifies end of the transmission
  digitalWrite(pinTransmitFlag, LOW);
  return;
}
