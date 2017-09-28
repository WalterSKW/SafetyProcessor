//
// ---------------------------------------------------------------
// Program  : Safetymonitor
// Hardware : Arduino Uno
// Author   : Whu
// ---------------------------------------------------------------
//  Version	Date		Description
// V0.1		20170813	Base
// V0.2   20170928  review LED behaviour
// ---------------------------------------------------------------
//
//#define DEBUG     			        // insert 2 slashes in front of the #define to disable
#define CYCLE 50		              // cycletime in ms = time between detector checks
#define LEDCYCLES 20			        // to control LED behaviour over a total cycle time of LEDCYCLES x CYCLE ms
/*
   NTC : Value decreases when temperature increases!!
   Value is independent of voltage variations in the 5V source since it is the ratio between the 2 resistors
   100K at 25°C
   10K @ ~80°C : with a 10K pull-up -> 2,5V (5V * 10k/(10k+10k)
   Max value = 1023@5V so limit will be a value of 512@2,5V
*/
// define limits
// NTC@25°C : 100k, NTC@80°C : 10k
// Pull up resistor to Vcc = 100k
// at room temp : voltage is around 2,5V = a value of 512 read from analog port
// If not connected the value will be around max voltage (~Vcc) =  a value of >1000 read from analog port
// At 80°C, NTC value =~10k with a voltage around 0,45V = a value of 93 read from analog port
#define SENSOR_PRESENT 950      	// Value above this value = no sensor (Pull up to max voltage = 1023)
#define TEMP_LIMIT 95             // below this limit -> Alarm
//
// Variables
// Digital ports
int SafetyRelay = 5;			        // Output : (de)activates relay
int OnOffSwitch = 12;			        // Input  : pushbutton switch for power on/off printer
int LedPower= 11;                 // Output : On when Power = On
int LedDetector1 = 8;			        // Output : LED in detector 1
int LedDetector2 = 10;			      // Output : LED in detector 2
int Detector1 = 7;				        // Input  : Smoke detctor in detector 1
int Detector2 = 9;				        // Input  : Smoke detctor in detector 2
//
// Analog ports
int NTCDetector1 = A0;			       // Input  : NTC in detector 1
int NTCDetector2 = A1;			       // Input  : NTC in detector 2
//
// 'measured' temperature values
int Temp1;						            // Value for NTC in detector 1
int Temp2;						            // Value for NTC in detector 2
//
// Alarms
boolean  Alarm;     	            // Alarm activated
boolean AlarmFlag;                // remembers an alarm. to keep persistent alarmstate until power off/reset
int  AlarmDet1;			              // Detector 1 : alarm Value
int  AlarmDet2;                   // Detector2 : alarm Value
//
boolean PrinterState;             // PrinterState false = power off, true = power on
//
// Flag true when detector found
boolean Detector1Present;
boolean Detector2Present;
//
int ActualLedCycle = 0;			      // used for signalling patterns on the LEDs
//
// Some arrays to store measured values
// 10 samples every 100ms means 1 second average.
int Temp1Array[10];
int Temp2Array[10];
int ArrayIndex;
boolean ArrayValid = false;       // Assure that the array has been completely filled at least once
//
int Average(int Array[]);
void TogglePower();
//
// Set up and initialisations
//
void setup()
{
  // Digital ports
  pinMode(LedDetector1, OUTPUT);
  pinMode(LedDetector2, OUTPUT);
  pinMode(SafetyRelay, OUTPUT);
  pinMode(LedPower, OUTPUT);
  pinMode(Detector1, INPUT);
  pinMode(Detector2, INPUT);
  pinMode(OnOffSwitch, INPUT_PULLUP); 	// Pushbutton

 // Serial.begin(9600);
  PrinterState = false;

  
  // initialisations
  PrinterState = false;                	// Printer power off
  TogglePower();
  AlarmFlag = false;     				        // alarm flag off. Will be set when a real alarm occurs. 
  ArrayIndex = 0;
  //
  // Search for sensors
  //
  Detector1Present = false;
  Detector2Present = false;
  Temp1 = analogRead(NTCDetector1);     //read value from the thermistor
  if (Temp1 < SENSOR_PRESENT)
  {
    Detector1Present = true;				    // Detector (NTC) found
  }
  Temp2 = analogRead(NTCDetector2);     //read value from the thermistor
  if (Temp2 < SENSOR_PRESENT)
  {
    Detector2Present = true;				    // Detector (NTC) found
  }
  ActualLedCycle = 0;
}
//
// ---------------------------------------------------------------
// loop()
// main loop
// Reads sensors only if there has been NO alarm before
// Reads power on switch and toggles power if NO outstanding alarm (set in EvaluateSensors)
// Switches power off on alarm and makes the alarm persistent (AlarmFlag)
// Shows state with LEDs
//
// ---------------------------------------------------------------
//
void loop()
{
  if(!AlarmFlag) EvaluateSensors();		  // If there has been NO alarm then read and process sensordata
  if (!Alarm)								            // if NO alarm then read on/off button. Alarm is set in EvaluateSensors()
  {
    // No active alarm
    if (digitalRead(OnOffSwitch) == LOW)
    {
      delay(200);                       // delay to debounce switch and read again
      if (digitalRead(OnOffSwitch) == LOW)  // Still button pressed ?
      {
        PrinterState = !PrinterState;   // toggle machine state
        TogglePower();
      }
    }
  }
  else
  {
    // Alarm condition
    PrinterState = false;							  // Machine = off
    AlarmFlag= true;                    // keeps state until complete power off/reset
    TogglePower();
  }

  ShowStatus();										      // show actual detection status on the LEDs

  delay(CYCLE);                         // delay 
}
//
// ---------------------------------------------------------------
// EvaluateSensors()
// Read smoke detection state and actual temperature value for installed detectors
// Store temperature value in an array to calculate an average value over the last 10 measurements
// Sets AlarmDet1 and AlarmDet2
// Possible values :
//  0 : no alarm
//  1 : temperature exceeded
//  2 : smoke detected
//  3 : temperature exceeded AND smoke detected
//
// ---------------------------------------------------------------
//
void EvaluateSensors()
{

  if (Detector1Present)
  {
    // Read inputs
    Temp1Array[ArrayIndex] = analogRead(NTCDetector1);	//read value from the thermistor
    AlarmDet1 = 2*!digitalRead(Detector1);     		        //read value from the smoke sensor : sets AlarmDet1 to 0 (no smoke) or 2 (smoke)
    // Check average temperature over last period (10 times x CYCLE)
    if (ArrayValid)									  // Array has been filled completely at least once
    {
      if (Average(Temp1Array) < TEMP_LIMIT)  AlarmDet1++; 
    }
  }
  if (Detector2Present)
  {
    // Read inputs
    Temp2Array[ArrayIndex] = analogRead(NTCDetector2);  //read value from the thermistor
    AlarmDet2 = 2*!digitalRead(Detector2);                //read value from the smoke sensor : sets AlarmDet2 to 0 (no smoke) or 2 (smoke)
    // Check average temperature over last period (10 times x CYCLE)
    if (ArrayValid)                  // Array has been filled completely at least once
    {
 
      if (Average(Temp2Array) < TEMP_LIMIT)  AlarmDet2++; 
    }
  }

  if ( (AlarmDet1 + AlarmDet2) > 0 )
    Alarm = true;
  else
    Alarm = false;

  if (++ArrayIndex > 9) 							// Increment and check index
  {
    ArrayIndex = 0;
    ArrayValid = true;								// at least filled once completely
  }

} // End EvaluateSensors
//
// ---------------------------------------------------------------
// ShowStatus()
// creates a blinking characteristic in function of the alarm state for the different detectors and the power LED indicator
// LEDs timing is based on CYCLE time (default 100ms) and LEDCYCLES (default 20 = 2 seconds)
// Power LED : 
//   controlled by PrinterState 
//      power    : constant ON with short occults (1 OFF, 19 ON)
//      no power : OFF with short flash (1 ON, 19 OFF)
// Detector LEDs : 
//   controlled by AlarmDet1 and AlarmDet2
//    - OFF when not detected (of course)
//    - OFF short flash when all safe : 1  ON / 19 OFF
//    - ON with 1 occult for temperature fault : 2 OFF, 18 ON
//    - ON with 2 occults when smoke detected. : 2 OFF, 2 ON, 2 OFF, 14 ON
// ---------------------------------------------------------------
//
void ShowStatus()
{
  if(ActualLedCycle++ >= LEDCYCLES) ActualLedCycle=0;

  // Power LED
  if(PrinterState)
  { // Printer ON
    if(ActualLedCycle == 0) digitalWrite(LedPower, LOW);
    else digitalWrite(LedPower, HIGH);
  }
  else
  { // Printer OFF
    if(ActualLedCycle == 0) digitalWrite(LedPower, HIGH);
    else digitalWrite(LedPower, LOW);
  }

  // Detector 1
  switch(AlarmDet1)
  {
    case 0 : // No alarm : short flash
      if(ActualLedCycle == 0) digitalWrite(LedDetector1, HIGH);
      else digitalWrite(LedDetector1, LOW);
      break;
    case 1 : // Temperature alarm : ON with 1 occult (2 cycles)
      if(ActualLedCycle < 2) digitalWrite(LedDetector1, LOW);
      else digitalWrite(LedDetector1, HIGH);
      break;
    case 2 : // Smoke alarm : ON with 2 occult (2 OFF, 2 ON, 2 OFF, 14 ON) 
      if(ActualLedCycle < 2) 
      { 
        digitalWrite(LedDetector1, LOW);  // 1st occult on cycles 0 and 1
      }
      else 
      {
        if((ActualLedCycle >= 4) && (ActualLedCycle < 6)) // 2nd occult on cycles 4 and 5
        {
          digitalWrite(LedDetector1, LOW);
        }
        else digitalWrite(LedDetector1, HIGH);
      }
      break;
  }

 // Detector 2
  switch(AlarmDet2)
  {
    case 0 : // No alarm : short flash
      if(ActualLedCycle == 0) digitalWrite(LedDetector2, HIGH);
      else digitalWrite(LedDetector2, LOW);
      break;
    case 1 : // Temperature alarm : ON with 1 occult (2 cycles)
      if(ActualLedCycle < 2) digitalWrite(LedDetector2, LOW); 
      else digitalWrite(LedDetector2, HIGH);
      break;
    case 2 : // Smoke alarm : ON with 2 occult (2 OFF, 2 ON, 2 OFF, 14 ON) 
      if(ActualLedCycle < 2) 
      { 
        digitalWrite(LedDetector2, LOW);  // 1st occult on cycles 0 and 1
      }
      else 
      {
        if((ActualLedCycle >= 4) && (ActualLedCycle < 6)) // 2nd occult on cycles 4 and 5
        {
          digitalWrite(LedDetector2, LOW);
        }
        else digitalWrite(LedDetector2, HIGH);
      }
      break;
  }
}
//
// ---------------------------------------------------------------
// Average()
// Calculate average over the array
// ---------------------------------------------------------------
//
int Average(int Array[])
{
  int Avg;
  int i;

  Avg = 0;
  for (i = 0; i < 10; i++)
  {
    Avg = Avg + Array[i];
  }
  Avg = Avg / 10;

  return Avg;
}
//
// ---------------------------------------------------------------
// TogglePower()
// Switches the SafetyRelay on or off depending on the actual printerstate
// ---------------------------------------------------------------
//
void TogglePower()
{ 
  switch(PrinterState)
  {
    case true :
      digitalWrite(SafetyRelay, HIGH);       // Relay On
      break;
    case false :
      digitalWrite(SafetyRelay, LOW);       // Relay Off
      break; 
  }
}
