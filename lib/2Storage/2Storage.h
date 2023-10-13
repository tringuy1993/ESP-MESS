#ifdef ESP32
   #include "LITTLEFS.h"
#else
   #include "LittleFS.h"
#endif

#include "Sto_RTC.h"
#include "Sto_LittleFS.h"
#include "Sto_EEPROM.h"

#define EEPROM_SIZE 2000

struct RTC_Data {
   unsigned long bootCount = 0;
};

template <int addr1, size_t len1, int addr2, size_t len2>
class Sto_PairValues {
   EEPROM_FixData<addr1, len1> value1Data;
   EEPROM_FixData<addr2, len2> value2Data;

   protected:
      char value1[len1], value2[len2];

   public:
      virtual void reloadData() {
         AppPrint("[StoPair]", __func__);
         value1Data.loadFixedData(value1);
         value2Data.loadFixedData(value2);
      }

      void storeData(const void* data1, const void* data2) {
         AppPrint("\n[StoPair]", __func__);
         value1Data.storeFixedData(data1);
         value2Data.storeFixedData(data2);

         //! reload stored data
         reloadData();
      }

      void deleteData() {
         value1Data.deleteData();
         value2Data.deleteData();
      }
};

// template format: startAddr of ssid, ssid length, startAddr of passw, passw length
class Sto_Cred: public Sto_PairValues<9, 33, 43, 64> {
   public:
      const char* ssid()    { return value1; }
      const char* passw()   { return value2; }

      void reloadData() override {
         Sto_PairValues::reloadData();
         AppPrint("[StoCred] SSID", ssid());
         AppPrint("[StoCred] PASSW", passw());
      }
};      

// template format: startAddr of devName, devName length, startAddr of mqttIp, mqttIp length
class Sto_Config: public Sto_PairValues<108, 21, 130, 21> {
   public:
      const char* devName()    { return value1; }
      const char* mqttIp()     { return value2; }
};


#define MAX_BEHAVIOR_ITEMS 10

class Sto_Behavior: public Loggable {
   EEPROM_Data rawData[MAX_BEHAVIOR_ITEMS];
   BehaviorItem* behaviors;
   bool isLoaded = false; 

   uint16_t getAddress(uint8_t index) {
      return 500+index*sizeof(BehaviorItem);
   }

   void reload() {
      for (int i=0; i<MAX_BEHAVIOR_ITEMS; i++) {
         uint16_t addr = getAddress(i);
         rawData[i].loadAddress(addr);
         rawData[i].loadData(&behaviors[i], sizeof(BehaviorItem));
      }
   }

   public:
      Sto_Behavior(): Loggable("Sto_Behav") {}

      void setup() {
         behaviors = new BehaviorItem[MAX_BEHAVIOR_ITEMS];
         reload();
         isLoaded = true;
      }

      void deleteData() {
         for (int i=0; i<MAX_BEHAVIOR_ITEMS; i++) {
               uint16_t addr = getAddress(i);
               rawData[i].deleteData();
         }
         reload();          
      }

      void updateData(uint8_t index, BehaviorItem* newItem) {
         if (index-1>MAX_BEHAVIOR_ITEMS) return;
         rawData[index].storeData(newItem, sizeof(BehaviorItem));
         bool check2 = rawData[index].loadData(&behaviors[index], sizeof(BehaviorItem));
         // xLogf("RetrievedData2 cue = 0x%02X; cmd = 0x%02X", behaviors[index].cue, behaviors[index].actionCmd);
      }

      BehaviorItem* getData(uint8_t index) {
         return &behaviors[index];
      }

      void handleCue(uint8_t refId, Cue_Trigger cue) {
         if (!isLoaded) return;

         for (int i=0; i<MAX_BEHAVIOR_ITEMS; i++) {
            BehaviorItem *behav = &behaviors[i];
            if (behav->cue != cue) continue;
            
            ControlOutput out1(0, 0);
            ControlWS2812 out2(0, 0);

            if (out1.extract(behav)) {
               xLogf("IM HERE1 pin = %u, value = %u", out1.pin, out1.value);
            } else if (out2.extract(behav)) {
               xLogf("IM HERE2 pin = %u, value = %u", out2.pin, out2.value);
            }
               // behaviors[i].handle(refId, cue);
         }
      }
};

#define MAX_VALUE_QUEUE 10

struct StoringValue {
   char value[32];
};

class Mng_Storage {
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
      EEPROM_ResetCount resetCount;
      Sto_Cred stoCred;
      Sto_Config stoConfig;
      Sto_Behavior stoBehavior;
      Sto_LittleFS littleFS;
      Sto_SD sd1;

      char sensorDataPath[32] = "";
      bool isValidPath()  { return String(sensorDataPath).isEmpty() == false && sd1.isReady(); }

      void setup() {
         EEPROM.begin(EEPROM_SIZE);
         resetCount.loadValue();
         
         char val[32];
         sprintf(val, "%llu", resetCount.value);
         AppPrintSeparator("[ResetCount]", val);
         stoCred.reloadData();
         stoConfig.reloadData();
         // stoBehavior.reloadData();

         // littleFS.begin();
         // Serial.println("\n\n***LittleFS test");
         // littleFS.test();
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

      void testSetupData() {
         stoCred.storeData("ffffffffff22222222223333333333", "gggggggggg3333333333444444444455555555556666666666");
         stoConfig.storeData("hhhhhhhhhhjjjjjjjjjj", "kkkkkkkkkkmmmmmmmmmm");
      }

      void deleteData() {
         AppPrint("[Sto]", __func__);
         resetCount.deleteValue();
         stoCred.deleteData();
         stoConfig.deleteData();
         stoBehavior.deleteData();
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