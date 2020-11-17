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

#ifndef SIMCOM_SIM800_BEARER_H_
#define SIMCOM_SIM800_BEARER_H_

#include "AT_CellularDevice.h"
#include "AT_CellularInformation.h"
#include "CellularLog.h"
#include "ATHandler.h"
#include "APN_db.h"
#include "SIMCOM_SIM800_HTTP.h"


/*
ToDO - spred CID through Bearer->(IP Apps). App shouldn't have posebility to change it.
     - Add email App.
     - Add feature to port to SIM7070G

*/


#define IPV4_ADDRESS_LENGTH 15

typedef signed int gprs_cmd_t;
typedef signed int gprs_status_t;

namespace mbed {

class SIMCOM_SIM800_Bearer;
class ATHandler;
/**
 * Class SIMCOM_SIM800_Bearer
 *
 * Class that provides ....
 */
class SIMCOM_SIM800_Bearer{
public:
enum gprs_status
{
    connecting = 0, //Bearer is connecting
    connected  = 1, //Bearer is connected
    closing    = 2, //Bearer is closing
    closed     = 3  //Bearer is closed
};

enum gprs_cmd
{
    close = 0, //Close bearer
    open  = 1, //Open bearer
    query = 2, //Query bearer
    set   = 3, //Set bearer parameters
    get   = 4  //Get bearer parameters
}; 

public:
    SIMCOM_SIM800_Bearer(AT_CellularDevice &device);
    
    //~SIMCOM_SIM800_Bearer();

    virtual nsapi_error_t setup_bearer();
    virtual gprs_status_t get_bearer_status(char* ipaddress=nullptr, size_t length = 0);
    virtual nsapi_error_t enable_bearer(bool onoff);
    void init_bearer(const char* apn, const char *uname, const char *pwd);

    void close_http();
    SIMCOM_SIM800_HTTP *open_http();
    SIMCOM_SIM800_HTTP *open_http_impl(ATHandler &at);


private:

void set_credentials(const char *apn, const char *uname, const char *pwd);

private:
    const char *_apn;
    const char *_uname;
    const char *_pwd;

    ATHandler            &_at;
    AT_CellularDevice    &_device;
    SIMCOM_SIM800_HTTP  *_http;

    int _http_ref_count = 0;
};

} // namespace mbed

#endif // SIMCOM_SIM800_BEARER_H_