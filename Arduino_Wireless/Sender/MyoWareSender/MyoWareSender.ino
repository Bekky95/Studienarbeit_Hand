#include <ArduinoBLE.h>
#include <MyoWare.h>

const String localName = "MyoWareSensor1"; 

MyoWare::OutputType outputType = MyoWare::ENVELOPE;
// debug parameters
const bool debugLogging = true;      // set to true for verbose logging
const bool debugOutput = true;        // set to true to print output values to serial

// MyoWare class object
MyoWare myoware;

// BLE Service
BLEService myoWareService(MyoWareBLE::uuidMyoWareService.c_str());

// BLE Muscle Sensor Characteristics
BLEStringCharacteristic sensorCharacteristic(MyoWareBLE::uuidMyoWareCharacteristic.c_str(), BLERead | BLENotify, 128);

void setup()
{
  Serial.begin(115200);
  while (!Serial);

  myoware.setConvertOutput(false);
  myoware.setGainPotentiometer(50.);  
  
  myoware.setENVPin(A3);              // Arduino pin connected to ENV (defult is A3 for Wireless Shield)
  myoware.setRAWPin(A4);              // Arduino pin connected to RAW (defult is A4 for Wireless Shield)
  myoware.setREFPin(A5);              // Arduino pin connected to REF (defult is A5 for Wireless Shield)

  pinMode(myoware.getStatusLEDPin(), OUTPUT);  // initialize the built-in LED pin to indicate
                                               // when a central is connected
  digitalWrite(myoware.getStatusLEDPin(), HIGH);

  // begin initialization
  bool error = !BLE.begin();
  if (error)
  {
    Serial.println("FAILED - BLE Initialization!");

    while (error);
  }

  BLE.setLocalName(localName.c_str());
  BLE.setAdvertisedService(myoWareService);
  myoWareService.addCharacteristic(sensorCharacteristic);
  BLE.addService(myoWareService);

  // set initial values for the characteristics
  sensorCharacteristic.writeValue("");

  BLE.advertise();

  if (debugLogging)
  {
    Serial.println("Setup Complete!");
    Serial.print(BLE.address());
    Serial.print(" '");
    Serial.print(localName.c_str());
    Serial.print("' ");
    Serial.print(myoWareService.uuid());
    Serial.println();
    Serial.print("Waiting to connect...");
  }

  digitalWrite(myoware.getStatusLEDPin(), LOW);
}

void loop()
{
  // wait for a BLE central
  BLEDevice central = BLE.central();
  if (central)
  {
    if (debugLogging)
    {
      Serial.print("Connected to central: ");
      Serial.println(central.address());
    }

    digitalWrite(myoware.getStatusLEDPin(), HIGH); // turn on the LED to indicate the
                                                   // connection

    while (central.connected())
    {
      // Read sensor output
      const String strValue = String(myoware.readSensorOutput(outputType));
      if (debugOutput)
        Serial.println(strValue.c_str());

      // "post" to "BLE bulletin board"
      sensorCharacteristic.writeValue(strValue);
    }

    // when the central disconnects, turn off the LED:
    digitalWrite(myoware.getStatusLEDPin(), LOW);

    if (debugLogging)
    {
      Serial.print("Disconnected from central: ");
      Serial.println(central.address());
    }
  }
  else
  {
    myoware.blinkStatusLED();
  }
}