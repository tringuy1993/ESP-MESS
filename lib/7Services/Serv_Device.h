// #define TTGO_LORA32 true
// #define DEV_KMC_70011 1

class Serv_Device: public Loggable, public Serv_Serial {
    std::function<void(bool, uint32_t)> irSwitchCb = [&](bool status, uint32_t value) {
        String output = "IrRead = " + (status ? String(value) : "Locked");
        AppPrint("[IR]", output);
        addDisplayQueue1(output, 6);
    };

    std::function<void(bool, uint8_t)> pirCb = [&](bool status, uint8_t pin) {
        Serial.print("PirTriggered = "); Serial.print(pin);
        Serial.print(" | "); Serial.println(status);
    };

    std::function<void(RotaryDirection, uint16_t)> rotaryCb = [&](RotaryDirection state, uint16_t counter) {
        #ifdef ESP32
            String readings = "IO36=" + String(digitalRead(36)) + " IO39=" + String(digitalRead(39));
            AppPrint("Read ", readings);
            addDisplayQueues(readings, 5);
        #endif

        String dir = (state == CLOCKWISE) ? "CW" : "CCW";
        String output =  "val=" + String(counter) + " Dir=" + dir;
        AppPrint("[Rot]", output);
        addDisplayQueues(output, 6);       // display

        i2c1.pca96.drivePWM(0, counter);
        if (onHandleRotary) (*onHandleRotary)(state, counter);
    };

    BNT_Hold releasedState = HOLD_TRANSITION;

    std::function<void(BTN_Action, BNT_Hold, uint32_t)> buttonCb = 
                        [&](BTN_Action action, BNT_Hold hold, uint32_t elapse) {
        switch (action) {
            case ACTION_SINGLE_CLICK: {
                xSerial.println("Hellow there\n");
                addDisplayQueues("Single Click", 6);
                // network.handleSingleClick();
                if (onHandleSingleClick) (*onHandleSingleClick)();
                toggleRelay();

                // servDev.buzzer.toggle();
                // led.toggle();

                // power.goToSleep(27, true);
                // if (power.wakeUpCauseResume()) {
                //     network.resetNetwork();
                // }

                break;
            }   
            case ACTION_DOUBLE_CLICK: {
                addDisplayQueues("Double Click", 6);
                // servDev.i2c1.switchDisplayMode();
                if (onHandleDoubleClick) (*onHandleDoubleClick)();
                break;
            }
            case ACTION_PRESS_ACTIVE: {
                //! Long press 20 sec -> Recovery/Factory Reset - turn off
                if (hold == HOLD_20_SEC) {
                    addDisplayQueues("[Bnt] 20sec hold", 6);
                    led.turnOFF();
                    storage.deleteData();

                //! Long press 10 sec -> Reset - fash flashing
                } else if (hold == HOLD_10_SEC) {
                    addDisplayQueues("[Bnt] 10sec hold", 6);
                    led.uniformPulse20ms();
                    releasedState = hold;

                //! Long press 5 sec -> Pairing - double flashing
                } else if (hold == HOLD_5_SEC) {
                    addDisplayQueues("[Bnt] 5sec hold", 6);
                    led.uniformPulse500ms();
                    releasedState = hold;
                }
                break;
            }
            case ACTION_PRESS_END: {
                addDisplayQueues("Press Ended " + String(elapse), 6);
                if (releasedState == HOLD_5_SEC) {
                    if (onHandleAPRequest) (*onHandleAPRequest)();
                } else if (releasedState == HOLD_10_SEC) {
                    MY_ESP.restart();           //! Restart Device
                }
                break;
            }
        }
    };

    public:
        Serv_Device(): Loggable("Dev"), Serv_Serial() {}

        WS28xx ws2812;
        MyButton button1;
        IRSwitch irSwitch;
        EdgeDetector edgeDetector;
        ExtraSerial xSerial;

        PulseController led, buzzer;
        PinWritable relay1;
        RotaryEncoder rotary;
            
        std::function<void()> *onHandleSingleClick;
        std::function<void()> *onHandleDoubleClick;
        std::function<void()> *onHandleAPRequest;
        std::function<void(RotaryDirection state, uint8_t counter)> *onHandleRotary;

        virtual void showLadderId() {}

        void configure(PinConfig* conf) {        
            led.setup(conf->led1);
            led.uniformPluse1000ms();
            relay1.begin(conf->relay1);

            // buzzer.setup(conf->buzzer1);
            ws2812.setup(conf->ws2812);
            // xSerial.setup(conf->swRx, conf->swTx);

            setupSerial(conf);

            irSwitch.load(conf->irSwitch, &irSwitchCb);
            edgeDetector.setup(conf->pir1, &pirCb);
            button1.setup(conf->btn1, &buttonCb);

            rotary.setup(conf->rotaryA, conf->rotaryB);
            rotary.onCallback = &rotaryCb;
        }

        void toggleRelay() {
            relay1.toggle();
        }

        void runGroupTasks() {
            button1.run();
            irSwitch.run(); 
            led.run();
            ws2812.run();
            serial.run();

            // buzzer.run();
            // edgeDetector.run();
            // xSerial.run();
        }

        void runMainTask2() {
            rotary.run();
        }

        void setFrequency() {
            #ifdef ESP32
                // system_update_cpu_freq(80);
            #else
                
            #endif
        }
};