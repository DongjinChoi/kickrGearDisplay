/**
* An external gear display for the Wahoo Kickr Bike
* On the LiLiGo T-Display
* Based on the Arduino BLE Example
* Extremely dirty code missing any kind of comments....
 */
// New background colour
//#define TFT_BROWN 0x38E0

#include "BLEDevice.h"
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
//#include "zwiftlogo.h"
//#include "BLEScan.h"
#include "lock32.h"
#include "unlock32.h"
#include "up32.h"
#include "down32.h"
#include "percent32.h"
TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

// The remote service we wish to connect to.
static BLEUUID    serviceUUID("a026ee0d-0a7d-4ab3-97fa-f1500f9feb8b");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("a026e03a-0a7d-4ab3-97fa-f1500f9feb8b");

//for grade
// The remote service we wish to connect to.
static BLEUUID    serviceUUID2("a026ee0b-0a7d-4ab3-97fa-f1500f9feb8b");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID2("a026e037-0a7d-4ab3-97fa-f1500f9feb8b");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLERemoteCharacteristic* pRemoteCharacteristic2;
static BLEAdvertisedDevice* myDevice;

String reargear = ("0");
String frontgear = ("0");
// String grade = ("0.0");

int fg, rg;
uint8_t arr[32];
int tilt_lock = 1;
int negative = 0;
int preNegative = -1;

static int prelock = -1;
static int preFG = -1;
static int preRG = -1;

static float preGrade = -100.0;
static float grade = 0.0;

unsigned int counter = 0;

void calc_tilt(uint8_t *pData, size_t length){

  for (int i = 0; i < length; i++) {
    Serial.print(pData[i], HEX);
    Serial.print(" "); //separate values with a space
  }
  Serial.println("");

  //lock, unlock update
  if(length == 3 && pData[0] == 0xfd && pData[1] == 0x33) {
    tilt_lock = pData[2] == 0x01;
  }

  //grade update
  if(length == 4 && pData[0] == 0xfd && pData[1] == 0x34){
    char s[10] = {};
    if(pData[3] < 0x80)
    {
      negative = 0; 
      float tmp = (float)(pData[3] << 8 | pData[2]) / 100;
      Serial.println(tmp);
      sprintf(s, "+ %.1f %%", tmp); 
      // grade = s;
      grade = tmp;
      Serial.println(s);
    }
    else
    {
      negative = 1;
      uint16_t tmp16 = 0xffff - (pData[3] << 8 | pData[2]);
      float tmp = (float)tmp16 / 100;
      Serial.println(tmp16);
      Serial.println(tmp);
      sprintf(s, "- %.1f %%", tmp);
      // grade = s;
      grade = tmp;
      Serial.println(s);
    }
  }
}


static void notifyCallback2(
  BLERemoteCharacteristic* pBLERemoteCharacteristic2,
  uint8_t* pData,
  size_t length,
  bool isNotify) {

  Serial.print("noty2: ");
  calc_tilt(pData, length); 
}


static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {

  if(length < 5) // brake garbage data
    return ;

  frontgear = (1+pData[2]);
  reargear = (1+pData[3]);

  fg = (int)(1+pData[2]);
  rg = (int)(1+pData[3]);
  Serial.println("Front / Rear");
  Serial.print(frontgear);
  Serial.print(" : ");
  Serial.println(reargear);
  updatedisp();
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
    {
        pRemoteCharacteristic->registerForNotify(notifyCallback);
    }

// tilt
// Obtain a reference to the service we are after in the remote BLE server.
    pRemoteService = pClient->getService(serviceUUID2);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service2");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic2 = pRemoteService->getCharacteristic(charUUID2);
    if (pRemoteCharacteristic2 == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID2.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic2");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic2->canRead()) {
      std::string value = pRemoteCharacteristic2->readValue();
      std::copy(value.begin(), value.end(), std::begin(arr));
      Serial.print("grade or lock first read : ");
      calc_tilt(arr, value.size());
    }

    if(pRemoteCharacteristic2->canNotify()) {
         pRemoteCharacteristic2->registerForNotify(notifyCallback2);       
    }
    
    connected = true;
    return true;
}

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");
  tft.init();
  tft.setRotation(1);

  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
  tft.drawCentreString("KICKR Bike", 120, 0, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Gear Display", 120, 30, 4);

  tft.drawString("Connecting.....", 50, 94, 4);

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
} // End of setup.

// | / - \ | / - \ |

// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
      tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
      tft.drawString("Connected :-)   ", 0, 78, 4);
      //tft.drawXBitmap(155, 0, zwiftlogo, 100, 100, TFT_BLACK, TFT_ORANGE);      
      tft.fillScreen(TFT_BLACK);
      //tft.drawXBitmap(155, 0, zwiftlogo, 100, 100, TFT_BLACK, TFT_ORANGE);
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  } else {
    if (!connected) {
      switch (counter % 5) {
          case 0: tft.drawString("Connecting.       ", 50, 94, 4); break;
          case 1: tft.drawString("Connecting..     ", 50, 94, 4); break;
          case 2: tft.drawString("Connecting...   ", 50, 94, 4); break;
          case 3: tft.drawString("Connecting.... ", 50, 94, 4); break;
          case 4: tft.drawString("Connecting.....", 50, 94, 4); break;
        }

        counter++;
    }
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
      String newValue = "Time since boot: " + String(millis()/1000);
  //  Serial.println("Setting new characteristic value to \"" + newValue + "\"");
    
     // Set the characteristic's value to be the array of bytes that is actually a string.
    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }
  
  delay(1000); // Delay a second between loops.
} // End of loop


void update_front(void)
{
  static int pre_pos = -1;
  if(pre_pos == fg) return;
  if(fg == 1) {
    tft.fillRect(10, 70, 5, 50, TFT_GREEN);
    tft.fillRect(20, 60, 5, 70, TFT_BLACK);
    tft.drawRect(20, 60, 5, 70, TFT_WHITE);
  }
  else{
    tft.fillRect(10, 70, 5, 50, TFT_BLACK);
    tft.drawRect(10, 70, 5, 50, TFT_WHITE);
    tft.fillRect(20, 60, 5, 70, TFT_GREEN);
  }
  pre_pos = fg;
}

void clear_cassette(int i)
{
  i--;
   tft.fillRect(40 + i*10, 60 + i*2, 5, 70 - i*4, TFT_BLACK);
   tft.drawRect(40 + i*10, 60 + i*2, 5, 70 - i*4, TFT_WHITE);
}

void fill_cassette(int i)
{
  i--;
  tft.fillRect(40 + i*10, 60 + i*2, 5, 70 - i*4, TFT_GREEN);
}
void update_rear(void)
{
  static int pre_rg = -1;
  if(pre_rg == -1)
  {
    for(int i = 0 ; i < 12 ; i++)
      tft.drawRect(40 + i*10,60 + i*2,5,70 - i*4, TFT_WHITE);
    pre_rg = 3;
  }  
  clear_cassette(pre_rg);
  fill_cassette(rg); 
  pre_rg = rg;
}

void update_gear(void){
  // tft.drawString(frontgear, 0, 0, 7);
  // tft.drawString(":", 35, 0, 7);
  // tft.fillRect(50, 0, tft.width() - 80, 50, TFT_BLACK);
  // tft.drawString(reargear, 50, 0, 7);

  if (preFG != fg || preRG != rg) {

    char s[7] = {};
    
    sprintf(s, "%d:%d", fg, rg); 
    tft.fillRect(0, 0, tft.width(), 80, TFT_BLACK);
    tft.drawCentreString(s, 120, 0, 8);
    
    // tft.drawString(frontgear, 20, 0, 8);
    // tft.drawString(":", 75, 0, 8 );
    // tft.fillRect(105, 0, tft.width() - 105, 80, TFT_BLACK);

    // tft.drawString(reargear, 105, 0, 8);
    preFG = fg;
    preRG = rg;
  }
}

void update_grade(void) {
  //tft.drawRightString(grade, 240, 15, 4);
  // tft.drawString(grade, 50, 88, 6);

  if (preNegative != negative || preGrade != grade) {
    char s[12] = {};
    int xPos = 0;
    sprintf(s, "%.1f", grade);
    
    if (grade > 9.99) {
      xPos = 25;
    } else {
      xPos = 45;
    }

    tft.fillRect(0, 80, 200, 130, TFT_BLACK);
    tft.setSwapBytes(true);
    if (grade == 0.0) {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString(s, 55, 88, 6);
    } else {

      if (negative) {
        tft.pushImage(xPos, 92, 32, 32, down32);

        if (grade > 9.99) {
          tft.setTextColor(TFT_GREEN, TFT_BLACK);
        } else {
          tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
        }
        
      } else {
        tft.pushImage(xPos, 92, 32, 32, up32);
        tft.setTextColor(TFT_ORANGE, TFT_BLACK);

        if (grade > 12.99) {
          tft.setTextColor(TFT_RED, TFT_BLACK);
        } else if (grade < 12.99 && grade > 6.99 ) {
          tft.setTextColor(TFT_ORANGE, TFT_BLACK);
        } else {
          tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        }
      }

      tft.drawString(s, xPos + 32 + 5, 88, 6);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
    }

    if (grade > 9.99) {
      // 140 is xx.x string length
      tft.pushImage(xPos + 140, 92, 32, 32, percent32);
    } else {
      // 110 is x.x string length
      tft.pushImage(xPos + 110, 92, 32, 32, percent32);
    }
  
    preNegative = negative;
    preGrade = grade;
  }

}

void update_lock(void)
{
  if(prelock == tilt_lock)
    return;

  tft.fillRect(208, 92, 32, 32, TFT_BLACK);
  tft.setSwapBytes(true);
  if(tilt_lock)
    tft.pushImage(208, 92, 32, 32, lock32);
  else
    tft.pushImage(208, 92, 32, 32, unlock32);
      
   prelock = tilt_lock;
}

void updatedisp(){
  update_gear();
  update_grade();
  // update_front();
  // update_rear();
  update_lock();
}
