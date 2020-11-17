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

#include "SIMCOM_SIM800.h"
#include "AT_CellularNetwork.h"
#include "CellularLog.h"
#include "rtos/ThisThread.h"
#include "drivers/BufferedSerial.h"
#include "SIMCOM_SIM800_CellularInformation.h"

#define PWR_KEY_TIMING 1500ms
#define RST_KEY_TIMING 200ms

using namespace std::chrono;
using namespace mbed;
using namespace rtos;
using namespace events;


static const intptr_t cellular_properties[AT_CellularDevice::PROPERTY_MAX] = {
    AT_CellularNetwork::RegistrationModeDisable,    // C_EREG AT_CellularNetwork::RegistrationMode. What support modem has for this registration type.
    AT_CellularNetwork::RegistrationModeLAC,    // C_GREG AT_CellularNetwork::RegistrationMode. What support modem has for this registration type. 
    AT_CellularNetwork::RegistrationModeLAC,    // C_REG  AT_CellularNetwork::RegistrationMode. What support modem has for this registration type.
    1,   // AT_CGSN_WITH_TYPE                 0 = not supported, 1 = supported. AT+CGSN without type is likely always supported similar to AT+GSN. 
    1,   // AT_CGDATA                         0 = not supported, 1 = supported. Alternative is to support only ATD*99***<cid>#
    0,   // AT_CGAUTH                         0 = not supported, 1 = supported. APN authentication AT commands supported
    1,   // AT_CNMI                           0 = not supported, 1 = supported. New message (SMS) indication AT command
    1,   // AT_CSMP                           0 = not supported, 1 = supported. Set text mode AT command
    1,   // AT_CMGF                           0 = not supported, 1 = supported. Set preferred message format AT command
    1,   // AT_CSDH                           0 = not supported, 1 = supported. Show text mode AT command
    1,   // PROPERTY_IPV4_STACK               0 = not supported, 1 = supported. Does modem support IPV4?
    0,   // PROPERTY_IPV6_STACK               0 = not supported, 1 = supported. Does modem support IPV6?
    0,   // PROPERTY_IPV4V6_STACK             0 = not supported, 1 = supported. Does modem support IPV4 and IPV6 simultaneously?
    0,   // PROPERTY_NON_IP_PDP_TYPE          0 = not supported, 1 = supported. Does modem support Non-IP?
    1,   // PROPERTY_AT_CGEREP                0 = not supported, 1 = supported. Does modem support AT command AT+CGEREP.
    1,   // PROPERTY_AT_COPS_FALLBACK_AUTO    0 = not supported, 1 = supported. Does modem support mode 4 of AT+COPS= ?
    6,   // PROPERTY_SOCKET_COUNT             The number of sockets of modem IP stack
    1,   // PROPERTY_IP_TCP                   0 = not supported, 1 = supported. Modem IP stack has support for TCP
    1,   // PROPERTY_IP_UDP                   0 = not supported, 1 = supported. Modem IP stack has support for TCP
    200  // PROPERTY_AT_SEND_DELAY            Sending delay between AT commands in ms
};


SIMCOM_SIM800::SIMCOM_SIM800(FileHandle *fh, PinName pwrkey, PinName reset, PinName supply): 
    AT_CellularDevice(fh),
    _powerkey(pwrkey, 0),
    _reset(reset, 1),
    _supply(supply, 0)
{
    set_cellular_properties(cellular_properties);
    rtos::ThisThread::sleep_for(1000ms);
}


nsapi_error_t SIMCOM_SIM800::init(){
    setup_at_handler();
    _at.lock();
    for (int retry = 1; retry <= 3; retry++) {
        _at.clear_error();
        _at.flush();
        _at.at_cmd_discard("E0", ""); // echo off
        if (_at.get_last_error() == NSAPI_ERROR_OK) {
            /*
            0 Disable +CME ERROR: <err> result code and use ERROR instead.
            1 Enable +CME ERROR: <err> result code and use numeric <err>
            2 Enable+CME  ERROR:  <err>  result  code  and  use  verbose  <err>
            */
            _at.at_cmd_discard("+CMEE", "=1"); // verbose responses
            _at.at_cmd_discard("+CFUN", "=1"); // set full functionality
            if (_at.get_last_error() == NSAPI_ERROR_OK) {
                break;
            }
        }
        tr_debug("Wait 100ms to init modem");
        rtos::ThisThread::sleep_for(100ms); // let modem have time to get ready
    }
    
#if defined (MBED_CONF_SIMCOM_SIM800_RTS) && defined(MBED_CONF_SIMCOM_SIM800_CTS)
    _at.at_cmd_discard("+IFC", "=", "%d%d", 2, 2);
#else
    _at.at_cmd_discard("+IFC", "=", "%d%d", 0, 0);
#endif
    return _at.unlock_return_error();
}

#if MBED_CONF_SIMCOM_SIM800_PROVIDE_DEFAULT
#include "drivers/BufferedSerial.h"
CellularDevice *CellularDevice::get_default_instance()
{
    static BufferedSerial serial(MBED_CONF_SIMCOM_SIM800_TX, MBED_CONF_SIMCOM_SIM800_RX, MBED_CONF_SIMCOM_SIM800_BAUDRATE);
#if defined (MBED_CONF_SIMCOM_SIM800_RTS) && defined(MBED_CONF_SIMCOM_SIM800_CTS)
    tr_debug("SIMCOM_SIM800 flow control: RTS %d CTS %d", MBED_CONF_SIMCOM_SIM800_RTS, MBED_CONF_SIMCOM_SIM800_CTS);
    serial.set_flow_control(SerialBase::RTSCTS, MBED_CONF_SIMCOM_SIM800_RTS, MBED_CONF_SIMCOM_SIM800_CTS);
#endif
    static SIMCOM_SIM800 device(&serial, MBED_CONF_SIMCOM_SIM800_PWRKEY, MBED_CONF_SIMCOM_SIM800_RESET, MBED_CONF_SIMCOM_SIM800_SUPPLY);
    return &device;
}
#endif

nsapi_error_t SIMCOM_SIM800::soft_power_on()
{
    if (_powerkey.is_connected()) {
        tr_info("SIMCOM_SIM800::soft_power_on");
        // check if modem was powered on already
        if(is_ready() != NSAPI_ERROR_OK)
        {
            _powerkey = 1;
            ThisThread::sleep_for(PWR_KEY_TIMING);
            _powerkey = 0;
        }
    }
    ThisThread::sleep_for(10000ms);
    //tr_info("SIMCOM_SIM800::soft_power_on - waiting 6sec");
    return NSAPI_ERROR_OK;
}

nsapi_error_t SIMCOM_SIM800::soft_power_off()
{
    tr_info("SIM800::soft_power_off");
    if (_powerkey.is_connected()) {
        _powerkey = 1;
        ThisThread::sleep_for(PWR_KEY_TIMING);
        _powerkey = 0;
    }
    return NSAPI_ERROR_OK;
}

nsapi_error_t SIMCOM_SIM800::hard_power_on(){

    if(_supply.is_connected())
    {
        tr_info("SIM800::hard_power_on");
        if(_supply.read())
        {
            tr_info("Power suplly of SIM800 was already enabled");
            return NSAPI_ERROR_OK;
        }
        else
        {
            _supply.write(1);
            tr_info("Power suplly of SIM800 has been enabled");
            return NSAPI_ERROR_OK;
        }
    }
    tr_info("SIM800::hard_power_on - unsupported function");
    return NSAPI_ERROR_UNSUPPORTED;
}

nsapi_error_t SIMCOM_SIM800::hard_power_off(){

    soft_power_off();

    if(_supply.is_connected())
    {
        tr_info("SIM800::hard_power_off");
        if(_supply.read())
        {
            _supply.write(0);
            tr_info("Power suplly of SIM800 has been disabled");
            return NSAPI_ERROR_OK;
        }
        else
        {
            tr_info("Power suplly of SIM800 was already disabled");
            return NSAPI_ERROR_OK;
        }
    }
    tr_info("SIM800::hard_power_off - unsupported function");
    return NSAPI_ERROR_UNSUPPORTED;
}

AT_CellularInformation *SIMCOM_SIM800::open_information_impl(ATHandler &at){
    return new SIMCOM_SIM800_CellularInformation(at, *this);
}
