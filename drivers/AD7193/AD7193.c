/***************************************************************************//**
 *   @file   AD7193.c
 *   @brief  Implementation of AD7193 Driver.
 *   @author DNechita (Dan.Nechita@analog.com)
********************************************************************************
 * Copyright 2012(c) Analog Devices, Inc.
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
********************************************************************************
 *   SVN Revision: $WCREV$
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include "platform_drivers.h"
#include "AD7193.h"    // AD7193 definitions.

/******************************************************************************/
/************************ Functions Definitions *******************************/
/******************************************************************************/

/***************************************************************************//**
 * @brief Initializes the communication peripheral and the initial Values for
 *        AD7193 Board and resets the device.
 *
 * @param device     - The device structure.
 * @param init_param - The structure that contains the device initial
 * 		       parameters.
 *
 * @return ret - The result of the initialization procedure.
 *               Example: -1 - SPI peripheral was not initialized or the
 *                             device is not present.
 *                         0 - SPI peripheral was initialized and the
 *                             device is present.
*******************************************************************************/
char AD7193_Init(ad7193_dev **device,
		    ad7193_init_param init_param)
{
	ad7193_dev *dev;
    char          status = 0;
    unsigned char regVal = 0;

	dev = (ad7193_dev *)malloc(sizeof(*dev));
	if (!dev)
		return -1;

	dev->currentPolarity = init_param.currentPolarity;
	dev->currentGain = init_param.currentGain;

	/* SPI */
	status = spi_init(&dev->spi_desc, init_param.spi_init);

	/* GPIO */
	status |= gpio_get(&dev->gpio_cs, init_param.gpio_cs);
	status |= gpio_get(&dev->gpio_miso, init_param.gpio_miso);

	if (dev->gpio_cs)
		status |= gpio_direction_output(dev->gpio_cs,
					     GPIO_HIGH);
	if (dev->gpio_miso)
		status |= gpio_direction_input(dev->gpio_miso);

    regVal = AD7193_GetRegisterValue(dev,
				     AD7193_REG_ID,
				     1,
				     1);
    if((regVal & AD7193_ID_MASK) != ID_AD7193)
    {
        status = -1;
    }

	*device = dev;

    return status;
}

/***************************************************************************//**
 * @brief Free the resources allocated by AD7193_Init().
 *
 * @param dev - The device structure.
 *
 * @return ret - The result of the remove procedure.
*******************************************************************************/
int32_t AD7193_remove(ad7193_dev *dev)
{
	int32_t status;

	status = spi_remove(dev->spi_desc);

	status |= gpio_remove(dev->gpio_cs);
	status |= gpio_remove(dev->gpio_miso);

	free(dev);

	return status;
}

/***************************************************************************//**
 * @brief Writes data into a register.
 *
 * @param dev             - The device structure.
 * @param registerAddress - Address of the register.
 * @param registerValue   - Data value to write.
 * @param bytesNumber     - Number of bytes to be written.
 * @param modifyCS        - Allows Chip Select to be modified.
 *
 * @return none.
*******************************************************************************/
void AD7193_SetRegisterValue(ad7193_dev *dev,
			     unsigned char registerAddress,
                             unsigned long registerValue,
                             unsigned char bytesNumber,
			     unsigned char modifyCS)
{
    unsigned char writeCommand[5] = {0, 0, 0, 0, 0};
    unsigned char* dataPointer    = (unsigned char*)&registerValue;
    unsigned char bytesNr         = bytesNumber;

    writeCommand[0] = AD7193_COMM_WRITE |
                      AD7193_COMM_ADDR(registerAddress);
    while(bytesNr > 0)
    {
        writeCommand[bytesNr] = *dataPointer;
        dataPointer ++;
        bytesNr --;
    }
	if (modifyCS)
		AD7193_CS_LOW;
	spi_write_and_read(dev->spi_desc, writeCommand, bytesNumber + 1);
	if (modifyCS)
		AD7193_CS_HIGH;
}

/***************************************************************************//**
 * @brief Reads the value of a register.
 *
 * @param dev             - The device structure.
 * @param registerAddress - Address of the register.
 * @param bytesNumber     - Number of bytes that will be read.
 * @param modifyCS        - Allows Chip Select to be modified.
 *
 * @return buffer         - Value of the register.
*******************************************************************************/
unsigned long AD7193_GetRegisterValue(ad7193_dev *dev,
				      unsigned char registerAddress,
                                      unsigned char bytesNumber,
				      unsigned char modifyCS)
{
    unsigned char registerWord[5] = {0, 0, 0, 0, 0};
    unsigned long buffer          = 0x0;
    unsigned char i               = 0;

    registerWord[0] = AD7193_COMM_READ |
                      AD7193_COMM_ADDR(registerAddress);
	if (modifyCS)
		AD7193_CS_LOW;
	spi_write_and_read(dev->spi_desc, registerWord, bytesNumber + 1);
	if (modifyCS)
		AD7193_CS_HIGH;
    for(i = 1; i < bytesNumber + 1; i++)
    {
        buffer = (buffer << 8) + registerWord[i];
    }

    return buffer;
}

/***************************************************************************//**
 * @brief Resets the device.
 *
 * @param dev - The device structure.
 *
 * @return none.
*******************************************************************************/
void AD7193_Reset(ad7193_dev *dev)
{
    unsigned char registerWord[6] = {0, 0, 0, 0, 0, 0};

    registerWord[0] = 0xFF;
    registerWord[1] = 0xFF;
    registerWord[2] = 0xFF;
    registerWord[3] = 0xFF;
    registerWord[4] = 0xFF;
    registerWord[5] = 0xFF;
	spi_write_and_read(dev->spi_desc, registerWord, 6);
}

/***************************************************************************//**
 * @brief Set device to idle or power-down.
 *
 * @param dev     - The device structure.
 * @param pwrMode - Selects idle mode or power-down mode.
 *                  Example: 0 - power-down
 *                           1 - idle
 *
 * @return none.
*******************************************************************************/
void AD7193_SetPower(ad7193_dev *dev,
		     unsigned char pwrMode)
{
     unsigned long oldPwrMode = 0x0;
     unsigned long newPwrMode = 0x0;

     oldPwrMode  = AD7193_GetRegisterValue(dev,
					   AD7193_REG_MODE,
					   3,
					   1);
     oldPwrMode &= ~(AD7193_MODE_SEL(0x7));
     newPwrMode  = oldPwrMode |
                   AD7193_MODE_SEL((pwrMode * (AD7193_MODE_IDLE)) |
                                  (!pwrMode * (AD7193_MODE_PWRDN)));
     AD7193_SetRegisterValue(dev,
			     AD7193_REG_MODE,
			     newPwrMode,
			     3,
			     1);
}

/***************************************************************************//**
 * @brief Waits for RDY pin to go low.
 *
 * @return none.
*******************************************************************************/
void AD7193_WaitRdyGoLow(ad7193_dev *dev)
{
	uint8_t wait = 1;

    while (wait)
        AD7193_RDY_STATE(wait);
}

/***************************************************************************//**
 * @brief Selects the channel to be enabled.
 *
 * @param dev     - The device structure.
 * @param channel - Selects a channel.
 *                  Example: AD7193_CH_0 - AIN1(+) - AIN2(-);  (Pseudo = 0)
 *                           AD7193_CH_1 - AIN3(+) - AIN4(-);  (Pseudo = 0)
 *                           AD7193_TEMP - Temperature sensor
 *                           AD7193_SHORT - AIN2(+) - AIN2(-); (Pseudo = 0)
 *
 * @return none.
*******************************************************************************/
void AD7193_ChannelSelect(ad7193_dev *dev,
			  unsigned short channel)
{
    unsigned long oldRegValue = 0x0;
    unsigned long newRegValue = 0x0;

    oldRegValue  = AD7193_GetRegisterValue(dev,
					   AD7193_REG_CONF,
					   3,
					   1);
    oldRegValue &= ~(AD7193_CONF_CHAN(0x3FF));
    newRegValue  = oldRegValue | AD7193_CONF_CHAN(1 << channel);
    AD7193_SetRegisterValue(dev,
			    AD7193_REG_CONF,
			    newRegValue,
			    3,
			    1);
}

/***************************************************************************//**
 * @brief Performs the given calibration to the specified channel.
 *
 * @param dev     - The device structure.
 * @param mode    - Calibration type.
 * @param channel - Channel to be calibrated.
 *
 * @return none.
*******************************************************************************/
void AD7193_Calibrate(ad7193_dev *dev,
		      unsigned char mode,
		      unsigned char channel)
{
    unsigned long oldRegValue = 0x0;
    unsigned long newRegValue = 0x0;

    AD7193_ChannelSelect(dev,
			 channel);
    oldRegValue  = AD7193_GetRegisterValue(dev,
					   AD7193_REG_MODE,
					   3,
					   1);
    oldRegValue &= ~AD7193_MODE_SEL(0x7);
    newRegValue  = oldRegValue | AD7193_MODE_SEL(mode);
    AD7193_CS_LOW;
    AD7193_SetRegisterValue(dev,
			    AD7193_REG_MODE,
			    newRegValue,
			    3,
			    0); // CS is not modified.
    AD7193_WaitRdyGoLow(dev);
    AD7193_CS_HIGH;
}

/***************************************************************************//**
 * @brief Selects the polarity of the conversion and the ADC input range.
 *
 * @param dev      - The device structure.
 * @param polarity - Polarity select bit.
 *                   Example: 0 - bipolar operation is selected.
 *                            1 - unipolar operation is selected.
 *@param range     - Gain select bits. These bits are written by the user to select
 *                   the ADC input range.
 *
 * @return none.
*******************************************************************************/
void AD7193_RangeSetup(ad7193_dev *dev,
		       unsigned char polarity,
		       unsigned char range)
{
    unsigned long oldRegValue = 0x0;
    unsigned long newRegValue = 0x0;

    oldRegValue  = AD7193_GetRegisterValue(dev,
					   AD7193_REG_CONF,
					   3,
					   1);
    oldRegValue &= ~(AD7193_CONF_UNIPOLAR |
                     AD7193_CONF_GAIN(0x7));
    newRegValue  = oldRegValue |
                  (polarity * AD7193_CONF_UNIPOLAR) |
                   AD7193_CONF_GAIN(range);
    AD7193_SetRegisterValue(dev,
			    AD7193_REG_CONF,
			    newRegValue,
			    3,
			    1);
    /* Store the last settings regarding polarity and gain. */
    dev->currentPolarity = polarity;
    dev->currentGain     = 1 << range;
}

/***************************************************************************//**
 * @brief Returns the result of a single conversion.
 *
 * @param dev - The device structure.
 *
 * @return regData - Result of a single analog-to-digital conversion.
*******************************************************************************/
unsigned long AD7193_SingleConversion(ad7193_dev *dev)
{
    unsigned long command = 0x0;
    unsigned long regData = 0x0;

    command = AD7193_MODE_SEL(AD7193_MODE_SINGLE) |
              AD7193_MODE_CLKSRC(AD7193_CLK_INT) |
              AD7193_MODE_RATE(0x060);
    AD7193_CS_LOW;
    AD7193_SetRegisterValue(dev,
			    AD7193_REG_MODE,
			    command,
			    3,
			    0); // CS is not modified.
    AD7193_WaitRdyGoLow(dev);
    regData = AD7193_GetRegisterValue(dev,
				      AD7193_REG_DATA,
				      3,
				      0); // CS is not modified.
    AD7193_CS_HIGH;

    return regData;
}

/***************************************************************************//**
 * @brief Returns the average of several conversion results.
 *
 * @param dev - The device structure.
 *
 * @return samplesAverage - The average of the conversion results.
*******************************************************************************/
unsigned long AD7193_ContinuousReadAvg(ad7193_dev *dev,
				       unsigned char sampleNumber)
{
    unsigned long samplesAverage = 0;
    unsigned long command        = 0;
    unsigned char count          = 0;

    command = AD7193_MODE_SEL(AD7193_MODE_CONT) |
              AD7193_MODE_CLKSRC(AD7193_CLK_INT) |
              AD7193_MODE_RATE(0x060);
    AD7193_CS_LOW;
    AD7193_SetRegisterValue(dev,
			    AD7193_REG_MODE,
			    command,
			    3,
			    0); // CS is not modified.
    for(count = 0; count < sampleNumber; count++)
    {
        AD7193_WaitRdyGoLow(dev);
        samplesAverage += AD7193_GetRegisterValue(dev,
						  AD7193_REG_DATA,
						  3,
						  0); // CS is not modified.
    }
    AD7193_CS_HIGH;
    samplesAverage = samplesAverage / sampleNumber;

    return samplesAverage;
}

/***************************************************************************//**
 * @brief Read data from temperature sensor and converts it to Celsius degrees.
 *
 * @param dev - The device structure.
 *
 * @return temperature - Celsius degrees.
*******************************************************************************/
float AD7193_TemperatureRead(ad7193_dev *dev)
{
    unsigned long dataReg     = 0;
    float temperature = 0;

    AD7193_RangeSetup(dev,
		      0,
		      AD7193_CONF_GAIN_1); // Bipolar operation, 0 Gain.
    AD7193_ChannelSelect(dev,
			 AD7193_CH_TEMP);
    dataReg      = AD7193_SingleConversion(dev);
    dataReg     -= 0x800000;
    temperature  = (float) dataReg / 2815;   // Kelvin Temperature
    temperature -= 273;                      // Celsius Temperature

    return temperature;
}

/***************************************************************************//**
 * @brief Converts 24-bit raw data to milivolts.
 *
 * @param dev      - The device structure.
 * @param rawData  - 24-bit data sample.
 * @param vRef     - The value of the voltage reference used by the device.
 *
 * @return voltage - The result of the conversion expressed as volts.
*******************************************************************************/
float AD7193_ConvertToVolts(ad7193_dev *dev,
			    unsigned long rawData,
			    float vRef)
{
    float voltage = 0;

    if(dev->currentPolarity == 0 )   // Bipolar mode
    {
        voltage = 1000 * (((float)rawData / (1ul << 23)) - 1) * vRef / dev->currentGain;
    }
    else                        // Unipolar mode
    {
        voltage = 1000 * ((float)rawData * vRef) / (1ul << 24) / dev->currentGain;
    }

    return voltage;
}