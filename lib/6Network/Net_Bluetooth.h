#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "BLEEddystoneURL.h"
#include "BLEEddystoneTLM.h"
#include "BLEBeacon.h"

#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00) >> 8) + (((x)&0xFF) << 8))

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
   void onResult(BLEAdvertisedDevice advertisedDevice) {
      // Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
      BLEAddress addr = advertisedDevice.getAddress();
      BLEAddress compareAddr = BLEAddress("98:89:13:0a:4e:36");
      if (!addr.equals(compareAddr)) return;

      uint8_t* payload = advertisedDevice.getPayload();
      Serial.print("addr = "); Serial.println(advertisedDevice.getAddress().toString().c_str());
      Serial.print("Payload = "); Serial.println(payload[0]);
   }
};

class Net_Bluetooth {
   BLEScan *pBLEScan;
   BLERemoteCharacteristic* pRemoteCharacteristic;
   BLEAdvertisedDevice* myDevice;
   
   void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    // Process the received data
   }

   public:
      void setup() {
         BLEDevice::init("");
         pBLEScan = BLEDevice::getScan(); //create new scan
         // pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
         pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
         // pBLEScan->setInterval(500);
         // pBLEScan->setWindow(99); // less or equal setInterval value
         // pBLEScan->start(0, true);
      }

      void connectToDevice(const char* targetName, bool filter = true) {
         BLEScanResults foundDevices = pBLEScan->start(1, true);

         for (int i=0; i<foundDevices.getCount(); i++) {
            BLEAdvertisedDevice device = foundDevices.getDevice(i);
            BLEAddress addr = device.getAddress();
            const char* name = device.getName().c_str();
            const char* servUUID = device.getServiceUUID().toString().c_str();
            const char *servData = device.getServiceDataUUID().toString().c_str();

            if (!filter) {
               Serial.printf("\nName = %s", name);
               Serial.printf("\nservUUID = %s", servUUID);
               Serial.printf("\nservDataUUID = %s", servData);
               Serial.println();
            }

            std::string nameStr = name;
            if (nameStr.find("JDY-") != std::string::npos) {
               BLEClient* pClient = BLEDevice::createClient();
               myDevice = new BLEAdvertisedDevice(device);
               bool check = pClient->connect(myDevice);

               // std::map<std::string, BLERemoteService*> *services = pClient->getServices();
               // // Print the UUIDs of all services
               // for (auto const& service : *services) {
               //    BLERemoteService* pRemoteService = service.second;
               //    Serial.print("\n______Service:");
               //    Serial.println(pRemoteService->getUUID().toString().c_str());
               //    std::map<std::string, BLERemoteCharacteristic*>* characteristics = pRemoteService->getCharacteristics();

               //    Serial.println("\nList of all Characteristics:");
               //    for (auto const& characteristic : *characteristics) {
               //          Serial.println(characteristic.second->getUUID().toString().c_str());
               //    }
               // }

               BLERemoteService* pRemoteService = pClient->getService("0000ffe0-0000-1000-8000-00805f9b34fb");
               Serial.println("\nList of all Characteristics:");
               std::map<std::string, BLERemoteCharacteristic*>* characteristics = pRemoteService->getCharacteristics();
               for (auto const& characteristic : *characteristics) {
                     Serial.println(characteristic.second->getUUID().toString().c_str());
               }

               BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic("0000ffe1-0000-1000-8000-00805f9b34fb");
               const char* readValue = pRemoteCharacteristic->readValue().c_str();
               
               Serial.printf("\n***READ = %s", readValue); 
               for (int i=0; i<strlen(readValue); i++) {
                  Serial.printf("%02X ", readValue[i]);
               }
               Serial.println();
               
               pClient->disconnect();
            }
         }
      }
      
      void scanForDevice(const char* target) {
         BLEScanResults foundDevices = pBLEScan->start(1, true);

         for (int i=0; i<foundDevices.getCount(); i++) {
            BLEAdvertisedDevice device = foundDevices.getDevice(i);
            BLEAddress addr = device.getAddress();
            BLEAddress compareAddr = BLEAddress(target);

            if (addr.equals(compareAddr)) {
               uint8_t* payload = device.getPayload();
               Serial.print("addr = "); Serial.println(device.getAddress().toString().c_str());
               Serial.print("Payload = "); Serial.println(payload[0]);

               std::string strManufacturerData = device.getManufacturerData();
               uint8_t cManufacturerData[100];
               strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);

               if (strManufacturerData.length() == 25 && cManufacturerData[0] == 0x4C && cManufacturerData[1] == 0x00) {
                  Serial.println("Found an iBeacon!");
                  BLEBeacon oBeacon = BLEBeacon();
                  oBeacon.setData(strManufacturerData);
                  Serial.printf("iBeacon Frame\n");
                  Serial.printf("ID: %04X Major: %d Minor: %d UUID: %s Power: %d\n", oBeacon.getManufacturerId(), ENDIAN_CHANGE_U16(oBeacon.getMajor()), ENDIAN_CHANGE_U16(oBeacon.getMinor()), oBeacon.getProximityUUID().toString().c_str(), oBeacon.getSignalPower());
               } else {
                  Serial.println("Found another manufacturers beacon!");
                  Serial.printf("strManufacturerData: %d ", strManufacturerData.length());
                  for (int i = 0; i < strManufacturerData.length(); i++)
                  {
                     Serial.printf("%X", cManufacturerData[i]);
                  }
                  Serial.printf("\n");
               }


               BLEClient* pClient = BLEDevice::createClient();
               myDevice = new BLEAdvertisedDevice(device);
               bool check = pClient->connect(myDevice);
               Serial.print("\n****CHECK = "); Serial.println(check);
               delay(100);
               pClient->disconnect();
            }
         }
      }

         // 98:89:13:0a:4e:36
      void run() {
         BLEScanResults foundDevices = pBLEScan->start(1);
         Serial.print("Devices found: ");
         Serial.println(foundDevices.getCount());
         Serial.println("Scan done!");
         Serial.println("--------------");

         for (int i=0; i<foundDevices.getCount(); i++) {
            BLEAdvertisedDevice device = foundDevices.getDevice(i);
            uint8_t* payload = device.getPayload();

            Serial.print("\nName = ") + Serial.println(device.getName().c_str());
            Serial.print("servUUID = ") + Serial.println(device.getServiceUUID().toString().c_str());
            Serial.print("servDataUUID = ") + Serial.println(device.getServiceDataUUID().toString().c_str());
            BLEAddress addr = device.getAddress();
            BLEAddress compareAddr = BLEAddress("98:89:13:0a:4e:36");

            bool compare = addr.equals(compareAddr);
            Serial.print("****Compare = "); Serial.println(compare);

            if (addr.equals(compareAddr)) {
               Serial.print("addr = "); Serial.println(device.getAddress().toString().c_str());
               Serial.print("Payload = "); Serial.println(payload[0]);

               std::string strManufacturerData = device.getManufacturerData();

               uint8_t cManufacturerData[100];
               strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);

               if (strManufacturerData.length() == 25 && cManufacturerData[0] == 0x4C && cManufacturerData[1] == 0x00) {
                  Serial.println("Found an iBeacon!");
                  BLEBeacon oBeacon = BLEBeacon();
                  oBeacon.setData(strManufacturerData);
                  Serial.printf("iBeacon Frame\n");
                  Serial.printf("ID: %04X Major: %d Minor: %d UUID: %s Power: %d\n", oBeacon.getManufacturerId(), ENDIAN_CHANGE_U16(oBeacon.getMajor()), ENDIAN_CHANGE_U16(oBeacon.getMinor()), oBeacon.getProximityUUID().toString().c_str(), oBeacon.getSignalPower());
               } else {
                  Serial.println("Found another manufacturers beacon!");
                  Serial.printf("strManufacturerData: %d ", strManufacturerData.length());
                  for (int i = 0; i < strManufacturerData.length(); i++)
                  {
                     Serial.printf("%X", cManufacturerData[i]);
                  }
                  Serial.printf("\n");
               }
            }

            // Serial.print("addr = "); Serial.println(device.getAddress().toString().c_str());
            // Serial.print("Payload = "); Serial.println(payload[0]);

            // std::string strManufacturerData = device.getManufacturerData();

            // uint8_t cManufacturerData[100];
            // strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);

            // if (strManufacturerData.length() == 25 && cManufacturerData[0] == 0x4C && cManufacturerData[1] == 0x00) {
            //    Serial.println("Found an iBeacon!");
            //    BLEBeacon oBeacon = BLEBeacon();
            //    oBeacon.setData(strManufacturerData);
            //    Serial.printf("iBeacon Frame\n");
            //    Serial.printf("ID: %04X Major: %d Minor: %d UUID: %s Power: %d\n", oBeacon.getManufacturerId(), ENDIAN_CHANGE_U16(oBeacon.getMajor()), ENDIAN_CHANGE_U16(oBeacon.getMinor()), oBeacon.getProximityUUID().toString().c_str(), oBeacon.getSignalPower());
            // } else {
            //    Serial.println("Found another manufacturers beacon!");
            //    Serial.printf("strManufacturerData: %d ", strManufacturerData.length());
            //    for (int i = 0; i < strManufacturerData.length(); i++)
            //    {
            //       Serial.printf("%X", cManufacturerData[i]);
            //    }
            //    Serial.printf("\n");
            // }
         }

         pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
      }
};