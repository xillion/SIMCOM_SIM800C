#include "SIMCOM_SIM800_Bearer.h"

using namespace mbed;
using namespace std::chrono_literals;

SIMCOM_SIM800_Bearer::SIMCOM_SIM800_Bearer(AT_CellularDevice &device): _device(device), _at(*device.get_at_handler())
{

}

//~SIMCOM_SIM800_Bearer()
//{
//
//}

void SIMCOM_SIM800_Bearer::set_credentials(const char *apn, const char *uname, const char *pwd)
{
    _apn = apn;
    _uname = uname;
    _pwd = pwd;
}

nsapi_error_t SIMCOM_SIM800_Bearer::setup_bearer()
{
    tr_info("enter setup_bearer");
    _at.at_cmd_discard("+SAPBR","=", "%d%d%s%s", 3,1,"Contype","GPRS");
    _at.at_cmd_discard("+SAPBR","=", "%d%d%s%s", 3,1,"APN",(_apn));
    _at.at_cmd_discard("+SAPBR","=", "%d%d%s%s", 3,1,"USER",(_uname));
    _at.at_cmd_discard("+SAPBR","=", "%d%d%s%s", 3,1,"PWD",(_pwd));
    tr_info("exit setup_bearer");
    return NSAPI_ERROR_OK;
}

void SIMCOM_SIM800_Bearer::init_bearer(const char* apn, const char *uname, const char *pwd)
{
tr_info("enter init_bearer");
#if MBED_CONF_CELLULAR_USE_APN_LOOKUP
    if(!apn)
    {
        char imsi[MAX_IMSI_LENGTH + 1];
        nsapi_error_t error = _device.open_information()->get_imsi(imsi, sizeof(imsi));
        if (error == NSAPI_ERROR_OK) {
            const char *apn_config = apnconfig(imsi);
            if (apn_config) {
                apn = _APN_GET(apn_config);
                uname = _APN_GET(apn_config);
                pwd = _APN_GET(apn_config);
                tr_info("Looked up APN %s", apn);
            }
        } 
        _device.close_information();
    }
#endif // MBED_CONF_CELLULAR_USE_APN_LOOKUP
    set_credentials(apn, uname, pwd);
    setup_bearer();
    tr_info("exit init_bearer");
}

gprs_status_t SIMCOM_SIM800_Bearer::get_bearer_status(char* ipaddress, size_t length)
{
    //+SAPBR: 1,1,"10.138.2.236"
    gprs_status_t status;

    _at.lock();
    _at.flush();
    _at.clear_error();
    _at.set_at_timeout(5000ms);
    _at.cmd_start_stop("+SAPBR", "=", "%d%d", 2,1);
    _at.resp_start("+SAPBR:");
    if(_at.info_resp())
    {
        _at.skip_param(1);
        status = _at.read_int();
        tr_info("\nGPRS status - %d\n", status);
        if(ipaddress == nullptr)
        {
            _at.skip_param(1);
        }
        else
        {
            _at.read_string(ipaddress, length);
            ipaddress[IPV4_ADDRESS_LENGTH+1] = '\0';
            tr_info("\nIP address - %s\n", ipaddress);
        }
    }
    _at.resp_stop();
    _at.restore_at_timeout();
    _at.unlock();

    return status;
}

nsapi_error_t SIMCOM_SIM800_Bearer::enable_bearer(bool onoff)
{
    tr_info("enter enable_bearer");
    if(onoff)
    {
    _at.lock();
    _at.flush();
    _at.clear_error();
    _at.set_at_timeout(85000ms);
    _at.cmd_start_stop("+SAPBR", "=", "%d%d", 1,1);
    _at.resp_start();
    _at.resp_stop();
    _at.restore_at_timeout();
    _at.unlock();
    }
    else
    {
        _at.at_cmd_discard("+SAPBR","=", "%d%d", 0,1);
    }
    tr_info("exit enable_bearer");
    return NSAPI_ERROR_OK;
}

SIMCOM_SIM800_HTTP *SIMCOM_SIM800_Bearer::open_http()
{
    if (!_http) {
        _http = open_http_impl(*_device.get_at_handler());
    }
    _http_ref_count++;
    return _http;
}

void SIMCOM_SIM800_Bearer::close_http()
{
    if (_http) {
        _http_ref_count--;
        if (_http_ref_count == 0) {
            delete _http;
            _http = NULL;
        }
    }
}

SIMCOM_SIM800_HTTP *SIMCOM_SIM800_Bearer::open_http_impl(mbed::ATHandler &at)
{
    return new SIMCOM_SIM800_HTTP(_at);
}