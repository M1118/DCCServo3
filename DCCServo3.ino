
#include <NmraDcc.h>
#include <Servo.h>
#include <DCCServo.h>

/*
 * DCC Servo 3
 *
 * Control up to three servos with a combination
 * of the speed control knob, direction button and
 * fucntions F0 to F2.
 * Servo limits of travel and speed are set via
 * CV values. Also the decoder address is set via CV's
 * Each servo has a block of 3 CV's,
 *   CVx    Lower limit of travel (degrees)
 *   CVx+1  Upper limit of travel (degrees)
 *   CVx+2  Time in seconds between both limits at 100% throttle
 *   CVx+3  The configuration flags for the servo
 *
 * Servo0, pin 9, CV base is 30
 * Servo1, pin 10, CV base is 35
 * Servo2, pin 11, CV base is 40
 *
 * Digital output 4 is used to provide a delayed power-up facility
 * This pin is taken low after power has been applied for a fixed time
 */
 
#define DCCSERVO_VERSION_ID  2
 
/*
 * Power up control
 */
#define CONTROL  4      // Pin to take low after power-up delay
#define WAIT_MS  5000   // Power-up delay

#define LED 13

// CV Storage structure
struct CVPair
{
  uint16_t  CV;
  uint8_t   Value;
};
 
int MyAddress;

/*
 * The CV that are used for servos
 */
#define CV_S0LIMIT0    30
#define CV_S0LIMIT1    31
#define CV_S0TRAVEL    32
#define CV_S0FLAGS     33
#define CV_S1LIMIT0    35
#define CV_S1LIMIT1    36
#define CV_S1TRAVEL    37
#define CV_S1FLAGS     38
#define CV_S2LIMIT0    40
#define CV_S2LIMIT1    41
#define CV_S2TRAVEL    42
#define CV_S2FLAGS     43

/*
 * The factory default CV values
 */
CVPair FactoryDefaultCVs [] =
{
  {CV_MULTIFUNCTION_EXTENDED_ADDRESS_LSB, 3},
  {CV_MULTIFUNCTION_EXTENDED_ADDRESS_MSB, 0},
  {CV_MULTIFUNCTION_PRIMARY_ADDRESS, 3},
  {CV_VERSION_ID, DCCSERVO_VERSION_ID},
  {CV_MANUFACTURER_ID, MAN_ID_DIY},
  {CV_29_CONFIG, 0},
  {CV_S0LIMIT0, 0},
  {CV_S0LIMIT1, 180},
  {CV_S0TRAVEL, 5},
  {CV_S0FLAGS, SERVO_INITMID},
  {CV_S1LIMIT0, 0},
  {CV_S1LIMIT1, 180},
  {CV_S1TRAVEL, 5},
  {CV_S1FLAGS, SERVO_INITMID},
  {CV_S2LIMIT0, 0},
  {CV_S2LIMIT1, 180},
  {CV_S2TRAVEL, 5},
  {CV_S2FLAGS, SERVO_INITMID},
};

#define FACTORY_RESET_AT_STARTUP  0

#if FACTORY_RESET_AT_STARTUP
static uint8_t FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair);
#else
static uint8_t FactoryDefaultCVIndex = 0;
#endif


DCCServo  *servo1, *servo2, *servo3;
const int servoPin = 9;

NmraDcc  Dcc;
DCC_MSG  Packet;
const int DccAckPin = 3;
const int DccInPin = 2;

unsigned long startms = 0;
/*
 * A factory reset is required. Called on first run if the NVRAM does not contain
 * valid CV values
 */ 
void notifyCVResetFactoryDefault()
{
  // Make FactoryDefaultCVIndex non-zero and equal to num CV's to be reset 
  // to flag to the loop() function that a reset to Factory Defaults needs to be done
  FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs)/sizeof(CVPair);
};

// This function is called by the NmraDcc library 
// when a DCC ACK needs to be sent
// Calling this function should cause an increased 60ma current drain
// on the power supply for 6ms to ACK a CV Read 
void notifyCVAck(void)
{
  digitalWrite( DccAckPin, HIGH );
  delay( 6 );  
  digitalWrite( DccAckPin, LOW );
}

/*
 * Called to notify a CV value has been changed
 */
void notifyCVChange( uint16_t CV, uint8_t value)
{
  Serial.print("CV Write: ");
  Serial.print(CV);
  Serial.print(" = ");
  Serial.println(value);
  Dcc.setCV(CV, value);
  switch (CV)
  {
    case CV_S0LIMIT0:
      servo1->setStart(value);
      break;
    case CV_S0LIMIT1:
      servo1->setEnd(value);
      break;
    case CV_S0TRAVEL:
      servo1->setTravelTime(value);
      break;
    case CV_S1LIMIT0:
      servo2->setStart(value);
      break;
    case CV_S1LIMIT1:
      servo2->setEnd(value);
      break;
    case CV_S1TRAVEL:
      servo2->setTravelTime(value);
      break;
    case CV_S2LIMIT0:
      servo3->setStart(value);
      break;
    case CV_S2LIMIT1:
      servo3->setEnd(value);
      break;
    case CV_S2TRAVEL:
      servo3->setTravelTime(value);
      break;
    case CV_MULTIFUNCTION_PRIMARY_ADDRESS:
      MyAddress = value;
      break;
  }
}

/*
 * Periodically called with the speed, direction and number of speed steps
 *
 * Work out the current speed percentage and direction and update each of the
 * servos with this data
 */
void notifyDccSpeed( uint16_t Addr, uint8_t Speed, uint8_t ForwardDir, uint8_t SpeedSteps )
{
  int percentage = ((Speed - 1) * 100) / SpeedSteps;
  if (servo1->isAbsolute())
    servo1->setPosition(percentage);
  else
    servo1->setSpeed(percentage, ForwardDir != 0);
  if (servo2->isAbsolute())
    servo2->setPosition(percentage);
  else
    servo2->setSpeed(percentage, ForwardDir != 0);
  if (servo3->isAbsolute())
    servo3->setPosition(percentage);
  else
    servo3->setSpeed(percentage, ForwardDir != 0);
}

/*
 * Called regularly to report the state of the function keys
 */
void notifyDccFunc( uint16_t Addr, uint8_t FuncNum, uint8_t FuncState)
{
  if (FuncNum != 1)
    return;
    
  if (FuncState & 0x10)
    servo1->setActive(true);
  else
    servo1->setActive(false);   
  if (FuncState & 0x01)
    servo2->setActive(true);
  else
    servo2->setActive(false);   
  if (FuncState & 0x02)
    servo3->setActive(true);
  else
    servo3->setActive(false);

  }

/*
 * Setup routine called on startup of the decoder
 */
void setup()
{
  Serial.begin(9600);
  
  // Configure the DCC CV Programing ACK pin for an output
  pinMode(DccAckPin, OUTPUT);
  digitalWrite(DccAckPin, LOW);
  
  pinMode(CONTROL, OUTPUT);
  digitalWrite(CONTROL, HIGH);
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  
  // Setup which External Interrupt, the Pin it's associated with that we're using and enable the Pull-Up 
  Dcc.pin(0, DccInPin, 1);
  
  // Call the main DCC Init function to enable the DCC Receiver
  Dcc.init(MAN_ID_DIY, 10, FLAGS_MY_ADDRESS_ONLY|FLAGS_OUTPUT_ADDRESS_MODE, 0);
  
  // If the saved CV value for the decoder does not match this version of
  // the code force a factry reset of the CV values
  if (Dcc.getCV(CV_VERSION_ID) != DCCSERVO_VERSION_ID)
    FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs)/sizeof(CVPair);
  
  // Create the instances of the servos, intiialise the limits and travel
  // times from the CV values
  servo1 = new DCCServo(servoPin, Dcc.getCV(CV_S0LIMIT0),
                      Dcc.getCV(CV_S0LIMIT1), Dcc.getCV(CV_S0TRAVEL),
                      Dcc.getCV(CV_S0FLAGS));
  servo2 = new DCCServo(servoPin + 1, Dcc.getCV(CV_S1LIMIT0),
                      Dcc.getCV(CV_S1LIMIT1), Dcc.getCV(CV_S1TRAVEL),
                      Dcc.getCV(CV_S1FLAGS));
  servo3 = new DCCServo(servoPin + 2, Dcc.getCV(CV_S2LIMIT0),
                      Dcc.getCV(CV_S2LIMIT1), Dcc.getCV(CV_S2TRAVEL),
                      Dcc.getCV(CV_S2FLAGS));
}

/*
 * Loop function, this is the main body of the code and is called repeatedly
 * in order to do whatever processing is required.
 */
void loop()
{
  if (startms == 0)
    startms = millis();
  // After WAIT_MS turn the power on
  if (millis() > startms + WAIT_MS)
  {
    digitalWrite(CONTROL, LOW);
    digitalWrite(LED, HIGH);
  }
  // Execute the DCC process frequently in order to ensure
  // the DCC signal processing occurs
  Dcc.process();
  
  /* Check to see if the default CV values are required */
  if( FactoryDefaultCVIndex && Dcc.isSetCVReady())
  {
    FactoryDefaultCVIndex--; // Decrement first as initially it is the size of the array 
    Dcc.setCV( FactoryDefaultCVs[FactoryDefaultCVIndex].CV,
          FactoryDefaultCVs[FactoryDefaultCVIndex].Value);
  }
  
  // Now call the loop method for every DCCServo instance
  servo1->loop();
  servo2->loop();
  servo3->loop();
}
