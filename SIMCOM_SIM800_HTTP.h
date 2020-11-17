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

#ifndef SIMCOM_SIM800_HTTP_H_
#define SIMCOM_SIM800_HTTP_H_

#include "platform/mbed_chrono.h"
#include "AT_CellularDevice.h"
#include "CellularLog.h"
#include "ATHandler.h"
 #include <stdint.h>

#define GET_RESPONSE_FLAG        1<<0
#define HEAD_INTO_RESPONSE_FLAG  1<<1

#define SIM_HTTP_METHOD SIMCOM_SIM800_HTTP::http_method

namespace mbed {

/**
 * Class SIMCOM_SIM800_HTTP
 *
 * Class that provides HTTP functionality.
 */

class SIMCOM_SIM800_HTTP{
public:
    typedef enum http_method
    {
        GET    =0, 
        POST   =1, 
        HEAD   =2, 
        DELETE =3 
    }http_method_t;

    typedef struct http_parameters
    {
    int           cid;         //
    bool          ssl;         //
    const char*   user_data;   //
    const char*   user_agent;  //
    const char*   proxy_addr;  //
    int           proxy_port;  //
    bool          redir;       //
    unsigned int  brk;         //
    unsigned int  brk_end;     //
    unsigned int  timeout;     //
    bool          head_in_resp;//
    }http_parameters_t;

    typedef struct http_request
    {
    http_method_t method;      // GET POST HEAD
    const char*   url;         //
    const char*   content;     // 
    const char*         outgo;       //
    size_t        outgo_size;  //
    char*         income;      //
    size_t*       income_size; //
    bool          get_respose; //
    }http_request_t;



    typedef struct http_action_result
    {
        int method;      //
        int status_code; // HTTP HTTP Status Code responded by remote server
        int data_len;    // The length of data got
    } http_action_result_t;

    typedef struct http_status
    {
        int mode;   //HTTP method specification (GET, POST, HEAD)
        int status; // Idle Receiving Sending
        int finish; // The amount of data which have been transmitted
        int remain; //The amount of data remaining to be sent or received

    } http_status_t;

    SIMCOM_SIM800_HTTP(ATHandler &at);
    virtual ~SIMCOM_SIM800_HTTP();

public:

    /** Initialize HTTP Service.
     *
     *  @return error with error code and type
     */
    virtual device_err_t init(unsigned int timeout);
    /** Terminate HTTP Service.
     *
     *  @return error with error code and type (No ERROR, AT ERROR, AT ERROR CMS, AT ERROR CME)
     */
    virtual device_err_t terminate(unsigned int timeout);
    virtual nsapi_error_t parameter(const char* paramTag, const char* paramValue, unsigned int timeout);
    virtual nsapi_error_t parameter(const char* paramTag, int paramValue, unsigned int timeout);
    virtual nsapi_error_t set_http_parameters(http_parameters_t *param, unsigned int timeout);
    virtual bool request(http_method_t type, const char *URL, const char *data_out, int len_out, unsigned int waittime);
    virtual bool request(http_request_t *req, unsigned int waittime);
    virtual bool response(char* data_in, int len_in, unsigned int waittime);
    virtual device_err_t get_status(http_status_t *stat);
    virtual nsapi_error_t set_ssl(bool onoff=false);
    
    /** Show the HTTP Header Information in HTTPREAD.
     *
     *  @return error with error code.
     */
    //virtual nsapi_error_t showhead(bool show);

private:
    device_err_t http_write(const char *data_out, int len_out);
    device_err_t http_read(char *data_in, unsigned int start_address, size_t data_len);
    device_err_t http_action(http_method_t type, http_action_result_t *res_act);
    bool _use_ssl;
    ATHandler &_at;
};

} // namespace mbed

#endif // SIMCOM_SIM800_HTTP_H_
