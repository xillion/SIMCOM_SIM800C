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

#include <stdio.h>

#include "SIMCOM_SIM800_CellularInformation.h"

namespace mbed {

SIMCOM_SIM800_CellularInformation::SIMCOM_SIM800_CellularInformation(ATHandler &at, AT_CellularDevice &device) : AT_CellularInformation(at, device)
{
}

nsapi_error_t SIMCOM_SIM800_CellularInformation::get_time(time_t *_time)
{
    char buf[32] = {0};
    if (time == NULL) {
        return NSAPI_ERROR_PARAMETER;
    }
    _at.at_cmd_str("+CCLK", "?", buf, sizeof(buf));
    struct tm   t;
    t.tm_isdst = 0;
    int timezone;
    sscanf(buf, "%d/%d/%d,%d:%d:%d+%d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec, &timezone);
    t.tm_year = t.tm_year + 100;
    t.tm_mon  = t.tm_mon - 1; 
    *_time = mktime(&t);
    return NSAPI_ERROR_OK;
}

}