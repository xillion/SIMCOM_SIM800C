/*
 * Copyright (c) 2018, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SIMCOM_SIM800C_H_
#define SIMCOM_SIM800C_H_


#include "AT_CellularDevice.h"
#include "DigitalOut.h"

#define PWR_KEY_TIMING 1000
#define RST_KEY_TIMING 200

namespace mbed {

/**
 * Generic Cellular module which can be used as a default module when porting new cellular module.
 * GENERIC_AT3GPP uses standard 3GPP AT commands (3GPP TS 27.007 V14.5.0 (2017-09)) to communicate with the modem.
 *
 * GENERIC_AT3GPP can be used as a shield for example on top K64F.
 * Cellular example can be used for testing: https://github.com/ARMmbed/mbed-os-example-cellular
 * Define in mbed_app.json "target_overrides" correct pins and other setup for your modem.
 *
 * If new target don't work with GENERIC_AT3GPP then it needs some customizations.
 * First thing to try can be checking/modifying cellular_properties array in GENERIC_AT3GPP.cpp, does the module support
 * these commands or not? Modify array and if that's not enough then some AT_xxx classes might need to be created and
 * methods overridden. Check help how other modules are done what methods they have overridden. Happy porting!
 */
class SIMCOM_SIM800C : public AT_CellularDevice {
public:
    SIMCOM_SIM800C(FileHandle *fh, PinName pwr = NC, bool active_high = false, PinName rst = NC, PinName spl = NC);
    
    virtual nsapi_error_t hard_power_on();  // Turn on  modem power supply 
    virtual nsapi_error_t hard_power_off(); // Turn off modem power supply 
    virtual nsapi_error_t soft_power_on();  // Turn on  modem with pwrkey
    virtual nsapi_error_t soft_power_off(); // Turn off modem with pwrkey

    //virtual nsapi_error_t init();
    //virtual nsapi_error_t shutdown();
    //virtual nsapi_error_t is_ready();

private:
    nsapi_error_t press_button(DigitalOut &button, uint32_t timeout);
    nsapi_error_t status();
    bool _active_high;
    DigitalOut _pwr; //power on/off key pin
    DigitalOut _rst; //reset pin
    DigitalOut _supply; //power supply enable pin

};
} // namespace mbed
#endif // SIMCOM_SIM800C_H_
