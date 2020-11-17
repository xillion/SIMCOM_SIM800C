#include "SIMCOM_SIM800_HTTP.h"
#include "mbed_debug.h"
#include "rtos/ThisThread.h"
#include <ctype.h>
#include <stdio.h>

using namespace mbed;
using namespace std::chrono_literals;

SIMCOM_SIM800_HTTP::SIMCOM_SIM800_HTTP(ATHandler &at): _at(at)
{

}
SIMCOM_SIM800_HTTP::~SIMCOM_SIM800_HTTP()
{

}

device_err_t SIMCOM_SIM800_HTTP::init(unsigned int timeout)
{
    tr_info("Init HTTP");
    device_err_t err;
    if(timeout != 0){
        _at.set_at_timeout(timeout);
    }

    for (int retry = 1; retry <= 3; retry++) 
    {
       _at.at_cmd_discard("+HTTPINIT","", "");
        err = _at.get_last_device_error();
        if(err.errType == DeviceErrorTypeNoError)
        {
           return err;
        }
        tr_debug("Wait 100ms to try again Initialize http");
        rtos::ThisThread::sleep_for(100ms); // let modem have time to get ready
    }

    if(timeout != 0){
    _at.restore_at_timeout();
    }
    tr_info("Modem CME ERROR - %d", err.errCode);
    return err;
}

device_err_t SIMCOM_SIM800_HTTP::terminate(unsigned int timeout)
{
    device_err_t err;
    if(timeout){
        _at.set_at_timeout(timeout);
    }

    for (int retry = 1; retry <= 3; retry++) 
    {
        _at.at_cmd_discard("+HTTPTERM", "");
        err = _at.get_last_device_error();
        if(err.errType == DeviceErrorTypeNoError)
        {
           return err;
        }
        tr_debug("Wait 1000ms to try terminate http again");
        rtos::ThisThread::sleep_for(1000ms); // let modem have time to get ready
    }

    if(timeout){
    _at.restore_at_timeout();
    }
    tr_info("Modem CME ERROR - %d", err.errCode);
    return err;
}

nsapi_error_t SIMCOM_SIM800_HTTP::set_http_parameters(http_parameters_t *param,  unsigned int timeout)
{
    if(param->user_data != nullptr)
    {
        parameter("USERDATA", param->user_data, timeout);
    }
    if(param->proxy_addr != nullptr)
    {
        parameter("PROPORT", param->proxy_port, timeout);
        parameter("PROIP", param->proxy_addr, timeout);
    }
    if(param->timeout != 0)
    {
        parameter("TIMEOUT", param->timeout, timeout);
        parameter("BREAK", param->brk, timeout);
        parameter("BREAKEND", param->brk_end, timeout);
    }
    if(param->user_agent != nullptr)
    {
        parameter("UA", param->user_agent, timeout);
    }
    parameter("CID", param->cid, timeout);
    parameter("REDIR", param->redir == true ? 1:0, timeout);
    set_ssl(param->ssl);

return NSAPI_ERROR_OK;
}

nsapi_error_t SIMCOM_SIM800_HTTP::parameter(const char* paramTag, const char* paramValue, unsigned int timeout)
{
    if(timeout != 0){
        _at.set_at_timeout(timeout);
    }
    for (int retry = 1; retry <= 3; retry++) 
    {
        _at.clear_error();
        _at.at_cmd_discard("+HTTPPARA","=", "%s%s", paramTag, paramValue);
        if (_at.get_last_error() == NSAPI_ERROR_OK) 
        {
            break;
        }
        tr_debug("Wait 100ms to try again set %s parameter", paramTag);
        rtos::ThisThread::sleep_for(100ms); // let modem have time to get ready
    }

    if(timeout != 0){
    _at.restore_at_timeout();
    }

    return _at.get_last_error();
}

nsapi_error_t SIMCOM_SIM800_HTTP::parameter(const char* paramTag, int paramValue, unsigned int timeout)
{
    if(timeout != 0){
        _at.set_at_timeout(timeout);
    }
    for (int retry = 1; retry <= 3; retry++) 
    {
        _at.clear_error();
        _at.at_cmd_discard("+HTTPPARA","=", "%s%d", paramTag, paramValue);
        if (_at.get_last_error() == NSAPI_ERROR_OK) 
        {
            break;
        }
        #if MBED_CONF_MBED_TRACE_ENABLE
        tr_debug("Wait 100ms to try again set %s parameter", paramTag);
        #endif
        rtos::ThisThread::sleep_for(100ms); // let modem have time to get ready
    }

    if(timeout != 0){
    _at.restore_at_timeout();
    }

    return _at.get_last_error();
}

bool SIMCOM_SIM800_HTTP::request(http_request_t *req, unsigned int waittime)
{
#if MBED_CONF_MBED_TRACE_ENABLE
    tr_info("\nHTTP request type - %d\n", req->method);
    tr_info("\nServer address - %s\n", req->url);
#endif
    http_action_result_t result;

    if(parameter("URL", req->url, waittime) != NSAPI_ERROR_OK)
    {
        return false;
    }
    switch(req->method)
    {
    case http_method::GET:
        return false;
    break;
    case http_method::POST:    
        if(http_write(req->outgo, req->outgo_size).errType != DeviceErrorType::DeviceErrorTypeNoError)
        {
            return false;
        }
        if(http_action(http_method::POST, &result).errType != DeviceErrorType::DeviceErrorTypeNoError)
        {
            return false;
        }
        if(result.status_code != 200)
        {
            #if MBED_CONF_MBED_TRACE_ENABLE
            tr_info("HTTP status code - %d", result.status_code);
            #endif
            return false;
        }
        if(req->get_respose)
        {
            http_read(req->income, 0, result.data_len);
            tr_info("DBG-> Response %s", req->income);
        }
    break;

    case http_method::HEAD:
        return false;
    break;

    default:
        #if MBED_CONF_MBED_TRACE_ENABLE
        tr_info("Unsuported HTTP request typ");
        #endif
        return false;
    }
    return true;
}

bool SIMCOM_SIM800_HTTP::request(http_method_t type, const char *URL, const char *data_out, int len_out, unsigned int waittime)
{
    
#if MBED_CONF_MBED_TRACE_ENABLE
    tr_info("\nHTTP request type - %d\n", type);
    tr_info("\nServer address - %s\n", URL);
#endif
    http_action_result_t result;

    if(parameter("URL", URL, 0) != NSAPI_ERROR_OK)
    {
        return false;
    }
    set_ssl(_use_ssl);
    switch(type)
    {
    case http_method::GET:
        return false;
    break;
    case http_method::POST:    
        if(http_write(data_out, len_out).errType != DeviceErrorType::DeviceErrorTypeNoError)
        {
            return false;
        }
        if(http_action(type, &result).errType != DeviceErrorType::DeviceErrorTypeNoError)
        {
            return false;
        }
        if(result.status_code != 200)
        {
            #if MBED_CONF_MBED_TRACE_ENABLE
            tr_info("HTTP status code - %d", result.status_code);
            #endif
            return false;
        }
        #if MBED_CONF_MBED_TRACE_ENABLE
        tr_info("HTTP status code - %d, recieved size %d", result.status_code, result.data_len);
        #endif
    break;

    case http_method::HEAD:
        return false;
    break;

    default:
        #if MBED_CONF_MBED_TRACE_ENABLE
        tr_info("Unsuported HTTP request typ");
        #endif
        return false;
    }
    return true;
}

bool SIMCOM_SIM800_HTTP::response(char* data_in, int len_in, unsigned int waittime)
{
    return true;
}

device_err_t SIMCOM_SIM800_HTTP::get_status(http_status_t *stat)
{
    device_err_t err;
    char buf[8] = {0};
    _at.lock();
    _at.flush();
    _at.clear_error();

    _at.cmd_start("AT+HTTPSTATUS?");
    _at.cmd_stop();
    _at.resp_start("+HTTPSTATUS:");
    _at.read_string(buf, 16);
    stat->status = _at.read_int();
    stat->finish = _at.read_int();
    stat->remain = _at.read_int();
    _at.resp_stop();

     err = _at.get_last_device_error();
    _at.unlock();

    if(strcmp(buf, "GET") == 0)
    {
        stat->mode = 0;
    }
    else if(strcmp(buf, "POST") == 0)
    {
        stat->mode = 1;
    }
    else if(strcmp(buf, "HEAD") == 0)
    {
        stat->mode = 2;
    }
#if MBED_CONF_MBED_TRACE_ENABLE
    tr_info("+HTTPSTATUS: - %d,%d,%d,%d,", stat->mode, stat->status, stat->finish, stat->remain);
#endif
    return err;
}

nsapi_error_t SIMCOM_SIM800_HTTP::set_ssl(bool onoff)
{
    _at.lock();
    for (int retry = 1; retry <= 3; retry++) 
    {
        _at.clear_error();
        _at.flush();
        _at.at_cmd_discard("+HTTPSSL", "=", "%d", onoff == true ? 1:0);
        if (_at.get_last_error() == NSAPI_ERROR_OK) 
        {
            break;
        }
        tr_debug("Wait 100ms to try again set HTTPSSL parameter");
        rtos::ThisThread::sleep_for(100ms); // let modem have time to get ready
    }
    return _at.unlock_return_error();
}

device_err_t SIMCOM_SIM800_HTTP::http_write(const char *data_out, int len_out)
{
    device_err_t err;
    _at.lock();
    _at.flush();
    _at.clear_error();
    _at.set_at_timeout(10000ms);
    _at.cmd_start_stop("+HTTPDATA","=", "%d%d", len_out, 2000);
    _at.resp_start("DOWNLOAD", true);
    _at.write_bytes((uint8_t *)data_out, len_out);
    _at.restore_at_timeout();
    _at.resp_stop();
    err = _at.get_last_device_error();
    _at.unlock();
    return err;
}

device_err_t SIMCOM_SIM800_HTTP::http_action(http_method_t type, http_action_result_t *res_act)
{
    device_err_t err;
    char buf[32];

    sprintf(buf, "AT+HTTPACTION=%d", (int)type);
    _at.lock();
    _at.flush();
    _at.clear_error();
    _at.set_at_timeout(5s);
    _at.cmd_start(buf);
    _at.cmd_stop();
    _at.resp_start();
    _at.resp_stop();
    _at.set_delimiter(0);
    _at.resp_start("+HTTPACTION:");
    int counter = 200;
    tr_info("DBG-> wait +HTTPACTION:");
    while(!_at.info_resp() && counter--)
    {
        rtos::ThisThread::sleep_for(100ms);
        //_at.cmd_stop();
    }
    tr_info("DBG-> counter: %d", counter);
    _at.read_string(buf, sizeof(buf));
    _at.resp_stop();
    _at.set_default_delimiter();
    err = _at.get_last_device_error();
    _at.restore_at_timeout();
    _at.unlock();
    //tr_info("DBG-> +HTTPACTION: %s", buf);
    sscanf(buf, "%d,%d,%d", &(res_act->method), &(res_act->status_code), &(res_act->data_len));
    return err;

}

device_err_t SIMCOM_SIM800_HTTP::http_read(char *data_in, unsigned int start_address, size_t data_len)
{
    device_err_t err;
    _at.lock();
    _at.flush();
    _at.clear_error();
    _at.cmd_start("AT+HTTPREAD");
    _at.cmd_stop();
    _at.resp_start("+HTTPREAD:");
     while(!_at.info_resp())
    {
        rtos::ThisThread::sleep_for(100ms);
        //_at.cmd_stop();
    }

    //*data_len = (size_t)_at.read_int();
    _at.skip_param();
    _at.read_bytes((uint8_t *)data_in, data_len);
    //_at.read_string(data_in, data_len);
    _at.resp_stop();
    data_in[data_len+1] = '\0';
    err = _at.get_last_device_error();
    _at.unlock();
    
    if(err.errType != DeviceErrorTypeNoError)
    {
        tr_debug("Modem CME ERROR - %d", err.errCode);
        return err;
    }
    return err;
}