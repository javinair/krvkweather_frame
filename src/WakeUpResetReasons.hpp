#ifndef RESET_MANAGER_HPP
#define RESET_MANAGER_HPP

#include <esp_sleep.h>
#include "Debugger.h"

class WakeUpResetManager {
private:
    Debugger& debugger;

public:
WakeUpResetManager(Debugger& dbg) : debugger(dbg) {}

    void printWakeupReason(bool& resetByButton) {
        resetByButton = false;
        esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
        switch (wakeup_reason) {
            case ESP_SLEEP_WAKEUP_GPIO:
                debugger.log("Wakeup by GPIO");
                resetByButton = true;
                break;
            case ESP_SLEEP_WAKEUP_EXT0:
                debugger.log("Wakeup caused by external signal using RTC_IO");
                break;
            case ESP_SLEEP_WAKEUP_EXT1:
                debugger.log("Wakeup caused by external signal using RTC_CNTL");
                break;
            case ESP_SLEEP_WAKEUP_TIMER:
                debugger.log("Wakeup caused by timer");
                break;
            case ESP_SLEEP_WAKEUP_TOUCHPAD:
                debugger.log("Wakeup caused by touchpad");
                break;
            case ESP_SLEEP_WAKEUP_ULP:
                debugger.log("Wakeup caused by ULP program");
                break;
            default:
                debugger.log("Wakeup was not caused by deep sleep");
                break;
        }
    }

    void logResetReason() {
        esp_reset_reason_t reason = esp_reset_reason();
        const char* reasonText;

        switch (reason) {
            case ESP_RST_POWERON: reasonText = "Power-on reset"; break;
            case ESP_RST_EXT: reasonText = "External reset"; break;
            case ESP_RST_SW: reasonText = "Software reset"; break;
            case ESP_RST_PANIC: reasonText = "Exception/panic reset"; break;
            case ESP_RST_INT_WDT: reasonText = "Interrupt watchdog"; break;
            case ESP_RST_TASK_WDT: reasonText = "Task watchdog"; break;
            case ESP_RST_WDT: reasonText = "Other watchdog reset"; break;
            case ESP_RST_DEEPSLEEP: reasonText = "Deep sleep reset"; break;
            case ESP_RST_BROWNOUT: reasonText = "Brownout reset"; break;
            case ESP_RST_SDIO: reasonText = "SDIO reset"; break;
            default: reasonText = "Unknown reset"; break;
        }
        debugger.log(reasonText);
    }

    void enableTimerWakeUp(int timeInSeconds) {
        esp_sleep_enable_timer_wakeup(timeInSeconds * 1000000);
    }
};

#endif // RESET_MANAGER_HPP