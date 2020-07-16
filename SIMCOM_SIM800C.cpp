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

#include "SIMCOM_SIM800C.h"
#include "AT_CellularNetwork.h"
#include "CellularLog.h"
#include "rtos/ThisThread.h"

using namespace mbed;

// by default all properties are supported
static const intptr_t cellular_properties[AT_CellularBase::PROPERTY_MAX] = {
    AT_CellularNetwork::RegistrationModeLAC,    // C_EREG AT_CellularNetwork::RegistrationMode. What support modem has for this registration type.
    AT_CellularNetwork::RegistrationModeLAC,    // C_GREG AT_CellularNetwork::RegistrationMode. What support modem has for this registration type. 
    AT_CellularNetwork::RegistrationModeLAC,    // C_REG  AT_CellularNetwork::RegistrationMode. What support modem has for this registration type.
    1,   // AT_CGSN_WITH_TYPE        0 = not supported, 1 = supported. AT+CGSN without type is likely always supported similar to AT+GSN. 
    1,   // AT_CGDATA                0 = not supported, 1 = supported. Alternative is to support only ATD*99***<cid>#
    0,   // AT_CGAUTH                0 = not supported, 1 = supported. APN authentication AT commands supported
    1,   // AT_CNMI                  0 = not supported, 1 = supported. New message (SMS) indication AT command
    1,   // AT_CSMP                  0 = not supported, 1 = supported. Set text mode AT command
    1,   // AT_CMGF                  0 = not supported, 1 = supported. Set preferred message format AT command
    1,   // AT_CSDH                  0 = not supported, 1 = supported. Show text mode AT command
    1,   // PROPERTY_IPV4_STACK      0 = not supported, 1 = supported. Does modem support IPV4?
    0,   // PROPERTY_IPV6_STACK      0 = not supported, 1 = supported. Does modem support IPV6?
    0,   // PROPERTY_IPV4V6_STACK    0 = not supported, 1 = supported. Does modem support IPV4 and IPV6 simultaneously?
    0,   // PROPERTY_NON_IP_PDP_TYPE 0 = not supported, 1 = supported. Does modem support Non-IP?
    1,   // PROPERTY_AT_CGEREP       0 = not supported, 1 = supported. Does modem support AT command AT+CGEREP.
};

SIMCOM_SIM800C::SIMCOM_SIM800C(FileHandle *fh, PinName pwr, bool active_high, PinName rst, PinName spl) 
    : AT_CellularDevice(fh),
    _active_high(active_high),
    _pwr(pwr, !_active_high),
    _rst(rst, !_active_high),
    _spl(spl, !_active_high)
{
    AT_CellularBase::set_cellular_properties(cellular_properties);
}

nsapi_error_t SIMCOM_SIM800C::hard_power_on(){

    if(_spl.is_connected())
    {
        tr_info("SIM800C::hard_power_on");
        if(_spl.read())
        {
            tr_info("Power suplly of SIM800C was already enabled");
            return NSAPI_ERROR_OK;
        }
        else
        {
            _spl.write(!_active_high);
            tr_info("Power suplly of SIM800C has been enabled");
            return NSAPI_ERROR_OK;
        }
        
    }
    return NSAPI_ERROR_UNSUPPORTED;
}

nsapi_error_t SIMCOM_SIM800C::hard_power_off(){

    if(_spl.is_connected())
    {
        tr_info("SIM800C::hard_power_off");
        if(_spl.read())
        {
            _spl.write(_active_high);
            tr_info("Power suplly of SIM800C has been disabled");
            return NSAPI_ERROR_OK;
        }
        else
        {
            tr_info("Power suplly of SIM800C was already disabled");
            return NSAPI_ERROR_OK;
        }
    }
    return NSAPI_ERROR_UNSUPPORTED;
}

nsapi_error_t SIMCOM_SIM800C::soft_power_on(){

    if(_pwr.is_connected())
    {
        tr_info("SIM800C::soft_power_on");
        if(status()!= NSAPI_ERROR_OK){
            tr_info("Power on modem");
            press_button(_pwr, PWR_KEY_TIMING); // SIM800C_Hardware_Design_V1.05  requires time 600 ms, but 1000 ms seems to be more robust
        }

    }
    return NSAPI_ERROR_UNSUPPORTED;
}

nsapi_error_t SIMCOM_SIM800C::soft_power_off(){

    if(_pwr.is_connected())
    {
        tr_info("SIM800C::soft_power_off");
        if(status() == NSAPI_ERROR_OK){
            // SIM800C_Hardware_Design_V1.05  requires VBAT to be stable over 30 ms, that's handled above
            shutdown();
            tr_info("Power off modem");
            press_button(_pwr, PWR_KEY_TIMING); // SIM800C_Hardware_Design_V1.05  requires time 600 ms, but 1000 ms seems to be more robust
        }
    }
    return NSAPI_ERROR_UNSUPPORTED;
} 

#if MBED_CONF_SIMCOM_SIM800C_PROVIDE_DEFAULT
#include "UARTSerial.h"
CellularDevice *CellularDevice::get_default_instance()
{
    static UARTSerial serial(MBED_CONF_SIMCOM_SIM800C_TX, MBED_CONF_SIMCOM_SIM800C_RX, MBED_CONF_SIMCOM_SIM800C_BAUDRATE);
#if defined (MBED_CONF_SIMCOM_SIM800C_RTS) && defined(MBED_CONF_SIMCOM_SIM800C_CTS)
    tr_debug("SIMCOM_SIM800C flow control: RTS %d CTS %d", MBED_CONF_SIMCOM_SIM800C_RTS, MBED_CONF_SIMCOM_SIM800C_CTS);
    serial.set_flow_control(SerialBase::RTSCTS, MBED_CONF_SIMCOM_SIM800C_RTS, MBED_CONF_SIMCOM_SIM800C_CTS);
#endif
    static SIMCOM_SIM800C device(&serial);
    return &device;
}
#endif

nsapi_error_t SIMCOM_SIM800C::press_button(DigitalOut &button, uint32_t timeout)
{
    if (!button.is_connected()) {
        return NSAPI_ERROR_UNSUPPORTED;
    }
    button = _active_high;
    ThisThread::sleep_for(timeout);
    button = !_active_high;
    return NSAPI_ERROR_OK;
}

nsapi_error_t SIMCOM_SIM800C::status()
{
    // check if modem is already ready
    _at->lock();
    _at->flush();
    _at->set_at_timeout(30);
    _at->cmd_start("AT");
    _at->cmd_stop_read_resp();
    nsapi_error_t err = _at->get_last_error();
    _at->restore_at_timeout();
    _at->unlock();
    _at->sync(500);
    return err;

    // modem is not responding, power it on
    if (err != NSAPI_ERROR_OK) {
        if (!reset) {
            // SIM800C_Hardware_Design_V1.05  requires VBAT to be stable over 30 ms, that's handled above
            tr_info("Power on modem");
            press_button(_pwr, PWR_KEY_TIMING); // SIM800C_Hardware_Design_V1.05  requires time 600 ms, but 1000 ms seems to be more robust
        } else {
            tr_warn("Reset modem");
            press_button(_rst, 150); // SIM800C_Hardware_Design_V1.05  requires RESET_N timeout at least 150 ms
        }
        _at->lock();
        _at->set_at_timeout(6000);
        _at->resp_start();
        _at->set_stop_tag("RDY");
        bool rdy = _at->consume_to_stop_tag();
        _at->set_stop_tag(OK);
        _at->restore_at_timeout();
        _at->unlock();
        if (!rdy) {
            return false;
        }
    }

    // sync to check that AT is really responsive and to clear garbage
    return _at->sync(500);
}