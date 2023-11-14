#include <LittleFS.h>

#include "Sto_RTC.h"
#include "Sto_LittleFS.h"
#include "Sto_EEPROM.h"
#include "Sto_Behavior.h"

#define EEPROM_SIZE 2000

struct RTC_Data {
   unsigned long bootCount = 0;
};

struct DeviceStats {
   uint8_t status = 0;
   uint16_t builtCode = 0;
   uint64_t resetCnt = 0;

   void increseResetCnt() {
      resetCnt++;
   }
};

//! This object get stored in EEPROM
//! please keep size minimal, dont inherit Loggable
class Sto_Stat: public EEPROM_Value<DeviceStats>{
   public:
      uint64_t resetCnt() { return value.resetCnt; }

      void load(uint16_t address) {
         loadData(address);
         value.increseResetCnt();
         storeData();
      }
};

bool extractValues(const char* key, char* input, char *value1, char *value2) {
   //! strtok detroys the original string, copy it before perform operation
   char inputStr[240] = "";
   memcpy(inputStr, input, sizeof(inputStr));
   char *ref = strtok(inputStr, " ");
   // Serial.print("\n*********KEY = "); Serial.println(key);

   if (strcmp(ref, key) == 0) {
      ref = strtok(NULL, " ");
      strcpy(value1, ref);
      // Serial.print("\n*********VALUE = "); Serial.println(key);

      if (value2 != nullptr) {
         ref = strtok(NULL, " ");
         ref[strlen(ref) - 1] = '\0';  // Replace '\n' with string terminator
         strcpy(value2, ref);
      }

      return true;
   }

   // Serial.print("\n*********KEY222 = "); Serial.println(key);
   return false;
}

//! This object get stored in EEPROM
//! please keep size minimal, dont inherit Loggable
class Dat_Cred: public ExtractorInterface {
   public:
      char ssid[33] = "";
      char password[64] = "";

      bool makeExtraction(char* input) override {
         return extractValues("cred", input, ssid, password);
      }

      void printValues() override {
         Loggable logger = Loggable("Dat_Cred");
         logger.xLogf("SSID = %s", ssid);
         logger.xLogf("PASSW = %s", password);   
      }
};

//! This object get stored in EEPROM
//! please keep size minimal, dont inherit Loggable
class Dat_Conf: public ExtractorInterface {
   public:
      char name[21], mqttIP[21];

      bool makeExtraction(char* input) override {
         return extractValues("conf", input, name, mqttIP);
      }

      void printValues() override {
         Loggable logger = Loggable("Dat_Conf");
         logger.xLogf("name = %s", name);
         logger.xLogf("mqttIP = %s", mqttIP);    
      }
};

class Dat_Plotter: public ExtractorInterface {
   public:
      char iotPlotter[63];
      char feedId[32];

      bool makeExtraction(char* input) override {
         return extractValues("iotPlotter", input, iotPlotter, feedId);
      }

      void printValues() override {
         Loggable logger = Loggable("Dat_Plotter");
         logger.xLogf("iotPlotter = %s", iotPlotter);
         logger.xLogf("feedId = %s", feedId);
      }
};

class Dat_Settings: public ExtractorInterface {
   public:
      bool xSerial = true;
      bool espNow = true;
      
      bool makeExtraction(char* input) override {
         char value[2], value2[2];

         if (extractValues("xSerial", input, value, NULL)) {
            Serial.println("TTTTTTTTTT 1111");
            xSerial = strcmp("1", value) == 0;
            return true;
         }
         else if (extractValues("espNow", input, value2, NULL)) {
            Serial.println("TTTTTTTTTT 2222");
            espNow = strchr(value2, '1');    //! search for char, strcmp doesn't work
            return true;
         }

         return false;
      }

      void printValues() override {
         Loggable logger = Loggable("DevConf");
         logger.xLogf("xSerial = %d", xSerial);
         logger.xLogf("espNow = %d", espNow);
      }
};

#define MAX_VALUE_QUEUE 10

struct StoringValue {
   char value[32];
};

class Mng_Storage: public Loggable {
   RTC_Data rtc_data;
   AppQueue<StoringValue, MAX_VALUE_QUEUE> valueQueue;

   // void saveBootCount() {
   //     rtc_storage.write(65, &rtc_data, sizeof(rtc_data));
   // }

   // void readBootCount() {
   //     rtc_storage.read(65, &rtc_data, sizeof(rtc_data));
   // }

   public:
      Sto_RTC rtc_storage;
      Sto_Stat stoStat;                               //! length 17 [0 - 17]
      EEPROM_Extractor<Dat_Cred> stoCred;             //! start 32 + len 104
      EEPROM_Extractor<Dat_Conf> stoConf;              //! start 144 + len 48
      EEPROM_Extractor<Dat_Plotter> stoPlotter;       //! start 200 + len 100
      EEPROM_Extractor<Dat_Settings> stoSettings;     //! start 312 + len 4?

      // Sto_Peer stoPeer;                      //! length 17*Count(20) [192 - 532/536]
      // Sto_Behavior stoBehavior;
      Sto_LittleFS littleFS;
      Sto_SD sd1;

      char sensorDataPath[32] = "";
      bool isValidPath()  { return String(sensorDataPath).isEmpty() == false && sd1.isReady(); }

      Mng_Storage(): Loggable("Mng_Sto") {}

      void setupStorage() {
         xLogSection(__func__);

         EEPROM.begin(EEPROM_SIZE);
         stoStat.load(0);
         xLogSectionf("resetCount = %llu", stoStat.resetCnt());

         stoCred.loadEEPROM(32);
         stoConf.loadEEPROM(144);
         stoPlotter.loadEEPROM(200);
         stoSettings.loadEEPROM(312);

         // stoPeer.load(192);

         // stoBehavior.reloadData();

         // littleFS.begin();
         // Serial.println("\n\n***LittleFS test");
         // littleFS.test();
      }

      void handleConsoleStr(char* inputStr) {
         xLogf("%s %s", __func__, inputStr);
         
         if (stoCred.extractToEEPROM("cred", inputStr)) {
            xLog("cred extracted");
         }
         else if(stoConf.extractToEEPROM("conf", inputStr)) {
            xLog("conf extracted");
         }
         else if (stoPlotter.extractToEEPROM("iotPlotter", inputStr)) {
            xLog("iotPlotter extracted");
         }
         else if (stoSettings.extractToEEPROM("settings", inputStr)) {
            xLog("setting extracted");
         }
      }

      void setupSDCard(uint8_t sdCS) {
         if (sdCS == 255) return;
         sd1.begin(sdCS);
         // sd1.test();
      }

      void loadStoragePath(String dateTimeStr) {
         if (dateTimeStr.isEmpty() || !sd1.isReady()) {
               AppPrint("[Sto] Err: sd loadPath Failed");
               return;
         }
         String path = "/sensors/" + dateTimeStr + "/reading.txt";
         memcpy(sensorDataPath, path.c_str(), sizeof(sensorDataPath));
         sd1.makeFile(sensorDataPath);
      }

      void addStoreTempHumLuxQueue(float temp, float hum, float lux, time_t timeStamp) {
         char output[32];
         sprintf(output, "%.2f %.2f %.2f %ld\n", temp, hum, lux, (long)timeStamp);

         StoringValue storeValue;
         strcpy(storeValue.value, output);
         valueQueue.sendQueue(&storeValue);
      }

      void handleValueQueue(std::function<void(uint32_t)> onComplete) {
         StoringValue item;
         if (!valueQueue.getQueue(&item)) return;
         // AppPrint("StoreValue", item.value);

         uint32_t timeRef = millis();
         sd1.appendFile(sensorDataPath, item.value);
         onComplete(millis()-timeRef);
      }

      void deleteData() {
         AppPrint("[Sto]", __func__);
         stoStat.deleteData();
         stoCred.deleteData();
         stoConf.deleteData();
         // stoBehavior.deleteData();
      }

      // void resetBootCnt() {
      //     // bootCount = 0;
      //     rtc_data.bootCount = 0;
      //     saveBootCount();
      // }

      // void incBootCnt() {
      //     // bootCount++;
      //     rtc_data.bootCount++;
      //     saveBootCount();
      // }

      // unsigned long getBootCnt() {
      //     readBootCount();
      //     #ifndef ESP32
      //         return rtc_data.bootCount;
      //     #else
      //         // return bootCount;
      //     #endif
      // }
};