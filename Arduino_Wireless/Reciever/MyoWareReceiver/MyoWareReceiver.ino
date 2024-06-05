/*
  MyoWare BLE Central Example Code
  Advancer Technologies, LLC
  Brian Kaminski
  8/01/2023

  This example sets up a BLE Central device, Then, it connects
  to up to four MyoWare 2.0 Wireless Shields that are reading the ENV and RAW
  outputs of a MyoWare Muscle sensor. It then streams the selected output data
  (ENVELOPE or RAW) from all sensors on the Serial Terminal.

  Note, in BLE, you have services, characteristics and values.
  Read more about it here:
  https://www.arduino.cc/reference/en/libraries/arduinoble/

  Note, before it begins checking the data and printing it,
  It first sets up some BLE stuff:
    1. sets up as a central
    2. scans for and connects to any MyoWare 2.0 Wireless Shields for 10 seconds

  In order for this example to work, you will need a MyoWare 2.0 Wireless Shield,
  and it will need to be programmed with the MyoWare BLEnPeripheral code,
  and advertizing with the unique and correlating characteristic UUID.

  Note, both the service and the characteristic need unique UUIDs and each
  MyoWare 2.0 Wireless Shield needs a unique name (e.g. MyoWareSensor1, MyoWareSensor2)

  This "BLE Central", will read each MyoWare 2.0 Wireless Sensor,
  aka the "BLE Peripheral", charactieristic, parse it for the ENV and RAW values,
  and print them to the serial terminal.

  Hardware:
  BLE device (e.g. ESP32) 
  USB from BLE device to Computer.

  ** For consistent BT connection follow these steps:
  ** 1. Reset Peripheral
  ** 2. Wait 5 seconds
  ** 3. Reset Central
  ** 4. Enjoy BT connection
  **
  ** ArduinoBLE does not support RE-connecting two devices.
  ** If you loose connection, you must follow this hardware reset sequence again.
  **
  ** ArduinoBLE does not support connecting more than four peripheral devices.

  This example code is in the public domain.
*/

#include <ArduinoBLE.h>
#include <Arduino.h>
#include <MyoWare.h>
#include <vector>
#include <Servo.h>
#include "esp_task_wdt.h"

#define THRESHOLD 250

// Pin defines
static const int THUMB_PIN    = 18;
static const int RING_PIN     = 2;
static const int MIDDLE_PIN   = 16;
static const int POINTER_PIN  = 5;
static const int PINKY_PIN    = 15;

// GPIO defines
static const int BUTTON_PIN = 23;

// Boolean to determine which mode to use
// 1: MyoWare sensor
// 0: Glove
bool mode = 1;
// Boolean to determine to print message or not
bool output = 0;

// Servo class objects
Servo servoThumb;
Servo servoRing;
Servo servoMiddle;
Servo servoPointer;
Servo servoPinky;


// debug parameters
const bool debugLogging = false; // set to true for verbose logging to serial

// The MyoWare devices
std::vector<BLEDevice> vecMyoWareShields;

// MyoWare class object
MyoWare myoware;

// ADC Definitions:
//Daumen: 
const int DAUMEN = 35;
static int DaumenVal;
static int DaumenDegree=20;

//Zeigefinger:
const int ZEIGEFINGER = 26; 
static int ZeigefingerVal;
static int ZeigefingerDegree=0;

//Mittelfinger:
const int MITTELFINGER = 34; 
static int MittelfingerVal;
static int MittelfingerDegree=0;

//Ringfinger:
const int RINGFINGER = 25; 
static int RingfingerVal;
static int RingfingerDegree=0;

//Kleiner Finger:
const int KLEINFINGER = 12; 
static int KleinfingerVal;
static int KleinfingerDegree=0;

// Interrupt function called when the button is pressed
void IRAM_ATTR isr() {
  mode = !mode;
  output = 1;
  esp_task_wdt_reset();
}

void setup()
{
  // Start Serial
  Serial.begin(115200);
  while (!Serial);

  // Configure the ESP32 WDT
  esp_task_wdt_init(100, false); // Set timeout to 10 seconds, enable panic (reset) if WDT expires
  // Enable the ESP32 WDT interrupt
  esp_task_wdt_add(NULL); // NULL means this task, add the current task to the WDT watch

  // Configure the button input to react to a high level
  attachInterrupt(BUTTON_PIN, isr, HIGH);

  // Attach the servo to the corresponding pin and set to 0
  servoThumb.attach(THUMB_PIN);
  servoThumb.write(30);
  servoRing.attach(RING_PIN);
  servoRing.write(0);
  servoMiddle.attach(MIDDLE_PIN);
  servoMiddle.write(0);
  servoPointer.attach(POINTER_PIN);
  servoPointer.write(0);
  servoPinky.attach(PINKY_PIN);
  servoPinky.write(0);
/*
  pinMode(THUMB_PIN,OUTPUT);
  pinMode(RING_PIN,OUTPUT);
  pinMode(MIDDLE_PIN,OUTPUT);
  pinMode(POINTER_PIN,OUTPUT);
  pinMode(THUMB_PIN,OUTPUT);
*/

  pinMode(myoware.getStatusLEDPin(), OUTPUT); // initialize the built-in LED pin to indicate 
                                              // when a central is connected

  // begin initialization
  if (!BLE.begin())
  {
    Serial.println("Starting BLE failed!");
    while (1);
  }

  if (debugLogging)
  {
    Serial.println("MyoWare BLE Central");
    Serial.println("-------------------");
  }

  // start scanning for MyoWare Wireless Shields
  if (debugLogging)
  {
    Serial.print("Scanning for MyoWare Wireless Shields: ");
    Serial.println(MyoWareBLE::uuidMyoWareService.c_str());
  }

  BLE.scanForUuid(MyoWareBLE::uuidMyoWareService.c_str(), true);

  // scan for Wireless Shields for 10sec
  const long startMillis = millis();
  while (millis() - startMillis < 10000) 
  {
    myoware.blinkStatusLED();

    BLEDevice peripheral = BLE.available();
    // If peripheral is there an not yet in the list of periphs
    if (peripheral && std::find(vecMyoWareShields.begin(), vecMyoWareShields.end(), peripheral) == vecMyoWareShields.end())
    {
      if (debugLogging)
      {
        Serial.print("Connecting to ");
        PrintPeripheralInfo(peripheral);
      }

      // connect to the peripheral
      BLE.stopScan();
      if (peripheral.connect())
      {
        if (!peripheral.discoverAttributes())
        {
          Serial.println("Discovering Attributes... Failed!");
          if (!peripheral.discoverAttributes())
          {
            Serial.println("Discovering Attributes... Failed!");
            Serial.print("Disconnecting... ");
            PrintPeripheralInfo(peripheral);
            peripheral.disconnect();
            Serial.println("Disconnected");
            continue;
          }
        }
        vecMyoWareShields.push_back(peripheral);
      }
      else
      {
        Serial.print("Failed to connect: ");        
        PrintPeripheralInfo(peripheral);
      }
      BLE.scanForUuid(MyoWareBLE::uuidMyoWareService.c_str(), true);
    }
  }
  BLE.stopScan();

  if (vecMyoWareShields.empty())
  {
    Serial.println("No MyoWare Wireless Shields found!");
    while (1);
  }  
    
  digitalWrite(myoware.getStatusLEDPin(), HIGH); // turn on the LED to indicate a 
                                                 // connection

  for (auto shield : vecMyoWareShields)
  {
    auto ritr = vecMyoWareShields.rbegin();
    if (ritr != vecMyoWareShields.rend() && shield != (*ritr))
    {
      Serial.print(shield.localName());
      Serial.print("\t");
    }
    else
    {
      Serial.println(shield.localName());
    }
  }
  
}

void loop()
{ 
  esp_task_wdt_reset();
  // Controll hand depending on mode (Myoware sensor or glove)
  if( mode )
  {
      for (auto shield : vecMyoWareShields)
      {
        if (!shield)
        {
          Serial.print("Invalid MyoWare Wireless Shields pointer! MAC Address: ");
          Serial.println(shield);
          auto itr = std::find(vecMyoWareShields.begin(), vecMyoWareShields.end(), shield);
          if (itr != vecMyoWareShields.end())
            vecMyoWareShields.erase(itr);
          continue;
        }

        if (debugLogging)
        {
          Serial.print("Updating ");
          PrintPeripheralInfo(shield);
        }

        if (!shield.connected())
        {
          // output zero if the Wireless shield gets disconnected
          // this ensures data capture can continue for the 
          // other shields that are connected
          Serial.print("0.0"); 
          Serial.print("\t"); 
          continue;
        }

        BLEService myoWareService = shield.service(MyoWareBLE::uuidMyoWareService.c_str());
        if (!myoWareService)
        {
          Serial.println("Failed finding MyoWare BLE Service!");
          shield.disconnect();
          continue;
        }
        
        // get sensor data
        BLECharacteristic sensorCharacteristic = myoWareService.characteristic(MyoWareBLE::uuidMyoWareCharacteristic.c_str());

        // Read and Handle sensor data
        const double sensorValue = ReadBLEData(sensorCharacteristic);
        handleData(sensorValue);
        Serial.print(sensorValue);

        if (vecMyoWareShields.size() > 1)
          Serial.print(","); 
    }
    Serial.println("");
  }
  else
  {
    //Use the glove to control the hand
    //Analogwerte auslesen
    if( output )
    {
      output = 0;
      Serial.println("glove mode activated....");
    }
    DaumenVal = analogRead(DAUMEN);
    ZeigefingerVal = analogRead(ZEIGEFINGER);
    MittelfingerVal = analogRead(MITTELFINGER);
    RingfingerVal = analogRead(RINGFINGER);
    KleinfingerVal = analogRead(KLEINFINGER); 
/*
    Serial.println(DaumenVal);
    Serial.println(ZeigefingerVal);
    Serial.println(MittelfingerVal);
    Serial.println(RingfingerVal);
    Serial.println(KleinfingerVal);
*/
    // Potiwert zu Fingerwinkel "transformieren"
    DaumenDegree = regler_to_degree(DaumenVal);
    ZeigefingerDegree = regler_to_degree(ZeigefingerVal);
    MittelfingerDegree = regler_to_degree(MittelfingerVal);
    RingfingerDegree = regler_to_degree(RingfingerVal);
    KleinfingerDegree = regler_to_degree(KleinfingerVal);
/*
    Serial.println("Daumen" + DaumenDegree);
    Serial.println("Zeige" + ZeigefingerDegree);
    Serial.println("Mittel" + MittelfingerDegree);
    Serial.println("Ring" + RingfingerDegree);
    Serial.println("Klein" + KleinfingerDegree);
*/
    // Winkel an den Servomotor senden
    if(DaumenDegree == 0)
    {
      DaumenDegree = 30;
    }
    servoThumb.write(DaumenDegree);
    servoPointer.write(ZeigefingerDegree);
    servoMiddle.write(MittelfingerDegree);
    servoRing.write(RingfingerDegree);
    servoPinky.write(KleinfingerDegree);
  }
 
}

// adjust the servos depending on the value from the sensor
void handleData(const double val)
{
  // Open/Close the hand depending on the value
  // TODO: maybe add range around threshold in which the hand does not respond
  if( val > THRESHOLD)
  {
    servoThumb.write(180);
    servoRing.write(180);
    servoMiddle.write(180);
    servoPointer.write(180);
    servoPinky.write(180);
  }
  else if( val < THRESHOLD)
  {
    servoThumb.write(30);
    servoRing.write(0);
    servoMiddle.write(0);
    servoPointer.write(0);
    servoPinky.write(0);
  }
  
}

// Read the sensor values from the characteristic
double ReadBLEData(BLECharacteristic& dataCharacteristic)
{
  if (dataCharacteristic)
  {
    if (dataCharacteristic.canRead())
    {
      // read the characteristic value as string
      char characteristicValue[20];
      dataCharacteristic.readValue( &characteristicValue,20);
      const String characteristicString(characteristicValue); 

      return characteristicString.toDouble();
    }
    else
    {
      if (debugLogging)
      {
        Serial.print("Unable to read characteristic:");
        Serial.println(dataCharacteristic.uuid());
      }
      return 0.0;
    }
  }
  else
  {
    if (debugLogging)
      Serial.println("Characteristic not found!");
  }
  return 0.0;
}

// Function to print the Info of the connected peripheral device
void PrintPeripheralInfo(BLEDevice peripheral)
{
  Serial.print(peripheral.address());
  Serial.print(" '");
  Serial.print(peripheral.localName());
  Serial.print("' ");
  Serial.println(peripheral.advertisedServiceUuid());
}

// Function to translate the resistor values to angle
int regler_to_degree(int regler) {
  int degree;
  if (regler >= 4000 && regler <= 4200) { //10kohm, 0-64Poti
    degree = 0;
  }
  else if (regler >= 3500 && regler <= 3999) { //65-130Poti
    degree = 20;
  }
  else if (regler >= 2800 && regler <= 3499) { //131-200Poti
    degree = 40;
  }
  else if (regler >= 2200 && regler <= 2799) { //201-270Poti
    degree = 60;
  }
  else if (regler >= 1800 && regler <= 2199) { //271-330Poti
    degree = 80;
  }
  else if (regler >= 1200 && regler <= 1799) { //331-400Poti
    degree = 100;
  }
  else if (regler >= 900 && regler <= 1199) { //401-470Poti
    degree = 120;
  }
  else if (regler >= 600 && regler <= 899) { //471-540Poti
    degree = 140;
  }
  else if (regler >= 300 && regler <= 599) { //541-615Poti
    degree = 160;
  }
  else if (regler >= 0 && regler <= 299) { //20kohm; 616-677Poti
    degree = 180;
  }
  else {
    degree = 0;  // Fehlerbehandlung für Werte außerhalb des Bereichs
  }
  return degree;
}
