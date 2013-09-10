/**************************************************************************//**
 *   @file   Command.c
 *   @brief  Implementation of the commands given by user through UART for AD5415.
 *   @author Istvan Csomortani (istvan.csomortani@analog.com)
 *******************************************************************************
 * Copyright 2013(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/
#include "Command.h"
#include "Console.h"
#include "AD5425.h"
#include "TIME.h"

/******************************************************************************/
/************************ Constants Definitions *******************************/
/******************************************************************************/
/* List of available commands */
const char* cmdList[] ={"help?",
                        "register=",
                        "ldacPin=",
                        "ldacPin?",
};

const char* cmdDescription[] = {
"  -  Displays all available commands.",
"  -  Loads the DAC input register with a given value. Accepted values:\r\n\
\tvalue:\r\n \
\t0 .. 256 - value to be written in register.",
"  -  Sets the output value of LDAC pin. Accepted values:\r\n\
\t0 - sets LDAC pin low.(default)\r\n\
\t1 - sets LDAC pin high.",
"  -  Displays the value of LDAC pin.",
};


const char* cmdExample[] = {
"",
"To load DAC B input register with 128, type: register=128",
"To set the LDAC pin high, type: ldacPin=1",
""
};

const char cmdNo = (sizeof(cmdList) / sizeof(const char*));

/******************************************************************************/
/************************ Variables Definitions *******************************/
/******************************************************************************/
cmdFunction cmdFunctions[12] = {GetHelp, SetRegister, SetLdacPin, GetLdacPin};

/* Variables holding information about the device */
unsigned char ldac = 0;

/**************************************************************************//***
 * @brief Displays all available commands.
 *
 * @return None.
*******************************************************************************/
void GetHelp(double* param, char paramNo) // "help?" command
{
    unsigned char displayCmd;

    CONSOLE_Print("Available commands:\r\n");
    for(displayCmd = 0; displayCmd < cmdNo; displayCmd++)
    {
        CONSOLE_Print("%s%s\r\n", (char*)cmdList[displayCmd],
                                  (char*)cmdDescription[displayCmd]);
    }
}

/***************************************************************************//**
 * @brief Initializes the device.
 *
 * @return - The result of the initialization.
 *              Example: ERROR  - the device was not initialized or the device
 *                              is not present.
 *                       SUCCES - the device was initialized and the device
 *                              is present.
*******************************************************************************/
char DoDeviceInit(void)
{
    if(AD5425_Init() == 0)
    {
        CONSOLE_Print("AD5425 OK\r\n");
        GetHelp(NULL, 0);
        return SUCCESS;
    }
    else
    {
        CONSOLE_Print("AD5425 Error\r\n");
        return ERROR;
    }
}

/***************************************************************************//**
 * @brief Loads the DAC register with a given value.
 *
 * @param param[0] - value to be written in register.
 *
 * @return None.
*******************************************************************************/
void SetRegister(double* param, char paramNo) // "loadAndUpdate=" command
{
    unsigned char dacValue = 0;

    /* Check if the parameters are valid */
    if(paramNo >= 1)
    {
        if(param[0] < 0)
        {
            param[0] = 0;
        }
        else
        {
            if(param[0] > 255)
            {
                param[0] = 255;
            }
        }

        dacValue = (unsigned char)param[0];
        AD5425_SetRegister(dacValue);
        /* Send feedback to user */
        CONSOLE_Print("value=%d\r\n", dacValue);
    }
    else
    {
        /* Display error messages */
        CONSOLE_Print("Invalid parameter!\r\n");
        CONSOLE_Print("%s%s\r\n", (char*)cmdList[1], (char*)cmdDescription[1]);
        CONSOLE_Print("Example: %s\r\n", (char*)cmdExample[1]);
    }
}

/***************************************************************************//**
 * @brief Sets the output value of LDAC pin.
 *
 * @param param[0] - value to be set for LDAC pin.
 *
 * @return None.
*******************************************************************************/
void SetLdacPin(double* param, char paramNo) // "ldacPin=" command
{
    unsigned char status = 0;

    /* Check if the parameter is valid */
    if(paramNo >= 1)
    {
        if(param[0] < 0)
        {
            param[0] = 0;
        }
        else
        {
            if(param[0] > 1)
            {
                param[0] = 1;
            }
        }

        status = (unsigned char) param[0];

        if (status == 0)
        {
            AD5425_LDAC_LOW;
            ldac = 0;
        }
        else
        {
            if (status == 1)
            {
                AD5425_LDAC_HIGH;
                ldac = 1;
            }
        }
        /* Send feedback to user */
        CONSOLE_Print("%s%d\r\n",(char*)cmdList[2], status);
     }
     else
     {
        /* Display error messages */
        CONSOLE_Print("Invalid parameter!\r\n");
        CONSOLE_Print("%s%s\r\n", (char*)cmdList[2], (char*)cmdDescription[2]);
        CONSOLE_Print("Example: %s\r\n", (char*)cmdExample[2]);
     }
}

/***************************************************************************//**
 * @brief Displays the value of LDAC pin.
 *
 * @param None.
 *
 * @return None.
*******************************************************************************/
void GetLdacPin(double* param, char paramNo) // "ldacPin?" command
{

    /* Send requested value to user */
    CONSOLE_Print("LDAC=%d\r\n",ldac);
}

