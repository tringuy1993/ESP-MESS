template <int address, class T, uint8_t count>
class Sto_Array: public Loggable {
   EEPROM_Data rawData[count];

   uint16_t getEEPROM_Address(uint8_t index) {
      return address+index*sizeof(T);
   }

   protected:
      T* array = new T[count];
      bool isLoaded = false; 

      Sto_Array(const char* id): Loggable(id) {}
      
   public:
      void reload() {
         for (int i=0; i<count; i++) {
            uint16_t addr = getEEPROM_Address(i);
            rawData[i].loadAddress(addr);
            rawData[i].loadData(&array[i], sizeof(T));
         }
         isLoaded = true;
      }

      void deleteData() {
         for (int i=0; i<count; i++) {
            uint16_t addr = getEEPROM_Address(i);
            rawData[i].deleteData(addr, sizeof(T));
         }
         reload();   
      }

      void updateData(uint8_t index, T* newItem) {
         if (index-1>count) return;
         rawData[index].storeData(newItem, sizeof(T));
         bool check2 = rawData[index].loadData(&array[index], sizeof(T));
      }

      T* firstMatch(std::function<bool(T*, uint8_t index)> cb) {
         if (!isLoaded) return NULL;

         for (int i=0; i<count; i++) {
            T* target = &array[i];
            bool match = cb(target, i);
            if (match) return target;
         }

         return NULL;        
      }

      void forEach(std::function<void(T*, uint8_t index)> cb) {
         if (!isLoaded) return;
         
         for (int i=0; i<count; i++) {
            cb(&array[i], i);
         }
      }
};

struct PeerItem {
   uint8_t mac[6];
   uint8_t peerId = INVALID_UINT8;  //! PeerId is assigned by Sto_Peer
   uint64_t builtTime = 0;

   PeerItem() {}
   
   PeerItem(const uint8_t macVal[6], uint64_t time = 0) {
      memcpy(mac, macVal, sizeof(mac));
      builtTime = time;
   }

   bool isValid() { return peerId != INVALID_UINT8; }

   bool hasSameMac(uint8_t* targetMac) {
      Serial.println("\n***ITEM "); printRaw(); Serial.println();
      Serial.printf("***TARGET_MAC=%02X:%02X:%02X:%02X:%02X:%02X", 
                           targetMac[0], targetMac[1], targetMac[2], 
                           targetMac[3], targetMac[4], targetMac[5]);
      Serial.println();

      bool compare = memcmp(mac, targetMac, sizeof(mac)) == 0;
      Serial.printf("Compare = %u", compare); Serial.println();
      return compare;
   }

   void assignPeerId(uint8_t id) {
      peerId = id;
   }

   void printRaw() {
      Serial.printf("Mac=%02X:%02X:%02X:%02X:%02X:%02X", 
                           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      // Serial.printf("\npeerId = %u, builtTime = %llu", peerId, builtTime);
      Serial.printf(" peerId = %u", peerId);
   }
};

#define MAX_PEER_COUNT 5

class Sto_Peer: public Sto_Array<1000, PeerItem, MAX_PEER_COUNT> {
   public:
      Sto_Peer(): Sto_Array("Sto_Peer") {}
      
      void addPeer(uint8_t* peerMac) {
         uint8_t lastAvailIndex = INVALID_UINT8;

         PeerItem *match = firstMatch([&](PeerItem* item, uint8_t index) {
            bool foundMatch = item->hasSameMac(peerMac);

            //! get last Available index
            if (!item->isValid() && !foundMatch) lastAvailIndex = index;
            if (foundMatch) {
               xLogLine(); xLogf("FirstMatchFound = %u", index);
            }
            return foundMatch; 
         });

         // if (match != NULL) {
         //    xLogLine(); xLogf("**FOUND MATCH");
         //    xLogLine(); xLogf("foundIndex = %u", match->peerId);

         // } else if (lastAvailIndex != INVALID_UINT8) {
         //    //! match == NULL
         //    xLogLine(); xLogf("\n**ADD NEW PEER");
         //    newPeer.assignPeerId(lastAvailIndex);
         //    updateData(lastAvailIndex, &newPeer);
         // }
      }

      // uint8_t findPeer(uint8_t* targetMac) {
      //    forEach([&](PeerItem* item, uint8_t index) {
      //       if (item->hasSameMac(targetMac)) {
      //          return item->peerId;
      //       }
      //    });

      //    return INVALID_UINT8;
      // }
};

#define MAX_BEHAVIOR_ITEMS 10

class Sto_Behavior: public Sto_Array<500, BehaviorItem, MAX_BEHAVIOR_ITEMS> {
   public:
      Sto_Behavior(): Sto_Array("Sto_Behav") {}
      
      void handleCue(uint8_t peerId, Cue_Trigger cue) {
         forEach([&](BehaviorItem* item, uint8_t index) {
            // xLogf("At Index = %u", index);
            if (item->check(peerId, cue) == false) return;      
            ControlOutput out1(0, 0);
            ControlWS2812 out2(0, 0);

            if (out1.extract(item)) {
               xLogf("IM HERE1 pin = %u, value = %u", out1.pin, out1.value);
            } else if (out2.extract(item)) {
               xLogf("IM HERE2 pin = %u, value = %u", out2.pin, out2.value);
            }
         });
      }
};
