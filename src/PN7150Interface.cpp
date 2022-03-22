// #############################################################################
// ###                                                                       ###
// ### NXP PN7150 Driver                                                     ###
// ###                                                                       ###
// ### https://github.com/Strooom/PN7150                                     ###
// ### Author(s) : Pascal Roobrouck - @strooom                               ###
// ### License : https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode ###
// ###                                                                       ###
// #############################################################################

#include "PN7150Interface.h"									// NCI protocol runs over a hardware interface, in this case an I2C with 2 extra handshaking signals


PN7150Interface::PN7150Interface() : I2Caddress(0x50)
{
    // Constructor, initializing IRQ and VEN and setting I2Caddress to a default value of 0x28
}

PN7150Interface::PN7150Interface(uint8_t I2Caddress) : I2Caddress(I2Caddress)
{
    // Constructor, initializing IRQ and VEN and initializing I2Caddress to a custom value
}

#define IRQ_PIN  GPIO_PIN_7
#define IRQ_GPIO GPIOG

#define VEN_PIN  GPIO_PIN_0
#define VEN_GPIO GPIOI

extern "C" I2C_HandleTypeDef hi2c1;


extern "C" void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(hi2c->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspInit 0 */

  /* USER CODE END I2C1_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    PeriphClkInitStruct.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      printf("ERROR HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK\n");
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB8     ------> I2C1_SCL
    PB9     ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Peripheral clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();
  /* USER CODE BEGIN I2C1_MspInit 1 */

  /* USER CODE END I2C1_MspInit 1 */
  }

}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
  if(hi2c->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspDeInit 0 */

  /* USER CODE END I2C1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();

    /**I2C1 GPIO Configuration
    PB8     ------> I2C1_SCL
    PB9     ------> I2C1_SDA
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_9);

  /* USER CODE BEGIN I2C1_MspDeInit 1 */

  /* USER CODE END I2C1_MspDeInit 1 */
  }

}

void PN7150Interface::initialize(void)
{
	__HAL_RCC_GPIOI_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	// IRQ goes from PN7150 to DeviceHost, so is an input
    GPIO_InitStruct.Pin = IRQ_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(IRQ_GPIO, &GPIO_InitStruct);

    // VEN controls the PN7150's mode, so is an output
	GPIO_InitStruct.Pin = VEN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(VEN_GPIO, &GPIO_InitStruct);

    //pinMode(IRQ, INPUT);
    //pinMode(VEN, OUTPUT);

    // PN7150 Reset procedure : see PN7150 datasheet 12.6.1, 12.6.2.2, Fig 18 and 16.2.2
    HAL_GPIO_WritePin(VEN_GPIO, VEN_PIN, GPIO_PIN_RESET);					// drive VEN LOW...
    delay_ms(1);															// ...for at least 10us
    HAL_GPIO_WritePin(VEN_GPIO, VEN_PIN, GPIO_PIN_SET);						// then VEN HIGH again, and wait for 2.5 ms for the device to boot and allow communication
    delay_ms(3);

    //Wire.begin();														// Start I2C interface

    __HAL_RCC_SYSCFG_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	hi2c1.Instance = I2C1;
	hi2c1.Init.Timing = 0x00303D5B;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK)
	{
	    printf("ERROR: HAL_I2C_Init(&hi2c1) != HAL_OK\n");
	    return;
	}

	/** Configure Analogue filter
	*/
	if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
	{
		printf("ERROR: HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK\n");
		return;
	}

	/** Configure Digital filter
	*/
	if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
	{
	     printf("HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK\n");
	     return;
	}

}

bool PN7150Interface::hasMessage() const
{
    return (GPIO_PIN_SET == HAL_GPIO_ReadPin(IRQ_GPIO, IRQ_PIN));		// PN7150 indicates it has data by driving IRQ signal HIGH
}

uint8_t PN7150Interface::write(uint8_t txBuffer[], uint32_t txBufferLevel) const
{
    //Wire.beginTransmission(I2Caddress);									// Setup I2C to transmit
    uint32_t nmbrBytesWritten = 0;
    //nmbrBytesWritten = Wire.write(txBuffer, txBufferLevel);				// Copy the data into the I2C transmit buffer
    if (HAL_I2C_Master_Transmit(&hi2c1, I2Caddress, txBuffer, txBufferLevel, HAL_MAX_DELAY) == HAL_OK)								// If this worked..
    {
        return 0;
    }
    else
    {
        return 4;														// Could not properly copy data ti I2C buffer, so treat as other error, see i2c_t3
    }
}

uint32_t PN7150Interface::read(uint8_t rxBuffer[]) const
{
    uint32_t bytesReceived = 0;												// keeps track of how many bytes we actually received
    if (hasMessage())													// only try to read something if the PN7150 indicates it has something
    {
        // using 'Split mode' I2C read. See UM10936 section 3.5
        //bytesReceived = Wire.requestFrom((int)I2Caddress, 3);			// first reading the header, as this contains how long the payload will be
    	if (HAL_I2C_Master_Receive(&hi2c1, I2Caddress | 0x01, rxBuffer, 3, HAL_MAX_DELAY) == HAL_OK )
    		bytesReceived += 3;

        uint8_t payloadLength = rxBuffer[2];
        if (payloadLength > 0)
        {
        	if (HAL_I2C_Master_Receive(&hi2c1, I2Caddress | 0x01, rxBuffer + 3, payloadLength, HAL_MAX_DELAY) == HAL_OK )
        		bytesReceived += payloadLength;

//            bytesReceived += Wire.requestFrom(I2Caddress, payloadLength);		// then reading the payload, if any
//            uint32_t index = 3;
//            while (index < bytesReceived)
//                {
//                rxBuffer[index] = Wire.read();
//                index++;
//                }
        }
    }
    else
    {
        bytesReceived = 0;
    }
    return bytesReceived;
}

// void PN7150Interface::test001()
// // is the VEN signal being properly controlled ? Measure with a multimeter and verify the 2 second high + second low square wave
//     {
//     Serial.println("Test 001 Cycle ---- Start");
//     Serial.println("Driving VEN HIGH");
//     digitalWrite(VEN, HIGH);
//     delay(2000);
//     Serial.println("Driving VEN LOW");
//     digitalWrite(VEN, LOW);
//     delay(2000);
//     Serial.println("Test 001 Cycle ---- End");
//     }

// void PN7150Interface::test002()
// // is the IRQ signal being properly read ? Remove the NFC module and put a resistor between IRQ and VEN to serve as a loopback observe result in Serial monitor
//     {
//     Serial.println("Test 002 Cycle ---- Start");
//     Serial.println("Driving VEN HIGH");
//     digitalWrite(VEN, HIGH);
//     delay(10);
//     if (hasMessage())
//         {
//         Serial.println("reading IRQ HIGH - ok");
//         }
//     else
//         {
//         Serial.println("reading IRQ LOW - error");
//         }
//     delay(500);
//     Serial.println("Driving VEN LOW");
//     digitalWrite(VEN, LOW);
//     delay(10);
//     if (hasMessage())
//         {
//         Serial.println("reading IRQ HIGH - error");
//         }
//     else
//         {
//         Serial.println("reading IRQ LOW - ok");
//         }
//     delay(500);
//     Serial.println("Test 002 Cycle ---- End");
//     }

// void PN7150Interface::test003()
//     {
//     // This will write data to the I2C. Monitor with a scope the I2C signals on the bus...
//     Serial.println("Test 003 Cycle ---- Start");
//     uint8_t tmpBuffer[] = { 0x00, 0xFF, 0xAA, 0x55 };
//     uint8_t resultCode;
//     resultCode = write(tmpBuffer, 4);
//     switch (resultCode)
//         {
//         case 0:
//             Serial.println("I2C Write succesfull");
//             break;

//         case 1:
//             Serial.println("I2C Write fail : data too long");
//             break;

//         case 2:
//             Serial.println("I2C Write fail : address NACK");
//             break;

//         case 3:
//             Serial.println("I2C Write fail : data NACK");
//             break;

//         default:
//             Serial.println("I2C Other Error");
//         }

//     Serial.println("Test 003 Cycle ---- End");
//     delay(500);
//     }

// void PN7150Interface::test004()
//     {
// 	// This will write data to the I2C, and then Check if the PN7150 indicates it wants to answer..
// 	Serial.println("Test 004 Cycle ---- Start");

// 	// Reset the PN7150, otherwise you can only run this test once..
// 	digitalWrite(VEN, LOW);									// drive VEN LOW for at least 0.5 ms after power came up : datasheet table 16.2.3
// 	delay(100);
// 	digitalWrite(VEN, HIGH);									// then VEN HIGH again, and wait for 2.5 ms for the device to boot and allow communication
// 	delay(50);

//     if (hasMessage())
//         {
//         Serial.println("IRQ was already HIGH before sending - error");
//         }
//     else
//         {
//         Serial.println("IRQ LOW before sending - ok");
//         }
//     uint8_t tmpBuffer[] = { 0x20, 0x00, 0x01, 0x01 };
//     write(tmpBuffer, 4);

//     delay(5); // How much delay do you need to check if there is an answer from the Device ? I checked this with a scope and the device responded 2.3ms after the end of the message

//     if (hasMessage())
//         {
//         Serial.println("IRQ HIGH after sending - ok");
//         }
//     else
//         {
//         Serial.println("IRQ still LOW after sending - error");
//         }

//     Serial.println("Test 004 Cycle ---- End");
//     }

// void PN7150Interface::test005()
//     {
//     // This will write CORE_REST_CMD to the PN7150, and then Check if we receive CORE_RESET_RSP back.. See NCI specification V1.0 section 4.1
//     // I am using the reset behaviour of the NCI to test send and response here, as it is otherwise difficult to trigger a read
//     Serial.println("Test 005 Cycle ---- Start");
// 	// Reset the PN7150, otherwise you can only run this test once..
// 	digitalWrite(VEN, LOW);									// drive VEN LOW for at least 0.5 ms after power came up : datasheet table 16.2.3
// 	delay(100);
// 	digitalWrite(VEN, HIGH);									// then VEN HIGH again, and wait for 2.5 ms for the device to boot and allow communication
// 	delay(50);

// 	uint8_t tmpBuffer[] = { 0x20, 0x00, 0x01, 0x01 };
//     write(tmpBuffer, 4);

//     delay(5); // How much delay do you need to check if there is an answer from the Device ?

//     uint8_t tmpRxBuffer[260];
//     uint32_t nmbrBytesReceived;

//     nmbrBytesReceived = read(tmpRxBuffer);

//     if (6 == nmbrBytesReceived)
//         {
//         Serial.print(nmbrBytesReceived);
//         Serial.println(" bytes received, 6 bytes expected - ok");
//         if (0x40 == tmpRxBuffer[0])
//             {
//             Serial.println("byte[0] = 0x40 : MT = Control Packet Response, PBF = 0, GID = Core = 0 - ok");
//             }
//         else
//             {
//             Serial.print("byte[0] = ");
//             Serial.print(tmpRxBuffer[0]);
//             Serial.println(" - error");
//             }

//         if (0x00 == tmpRxBuffer[1])
//             {
//             Serial.println("byte[1] = 0x00 : OID = CORE_RESET_RSP - ok");
//             }
//         else
//             {
//             Serial.print("byte[1] = ");
//             Serial.print(tmpRxBuffer[1]);
//             Serial.println(" - error");
//             }

//         if (0x03 == tmpRxBuffer[2])
//             {
//             Serial.println("byte[2] = 0x03 : payload length = 3 bytes - ok");
//             }
//         else
//             {
//             Serial.print("byte[2] = ");
//             Serial.print(tmpRxBuffer[2]);
//             Serial.println(" - error");
//             }

//         Serial.print("byte[3] = Status = ");							// See NCI V1.0 Specification Table 94. 0x00 = Status_OK
//         Serial.print(tmpRxBuffer[3]);
//         Serial.println("");

//         Serial.print("byte[4] = NCI Version = ");						// See NCI V1.0 Specification Table 6. 0x17 = V1.7 ?? Not sure about this as I don't have official specs from NCI as they are quite expensive
//         Serial.print(tmpRxBuffer[4]);
//         Serial.println("");

//         Serial.print("byte[5] = Configuration Status = ");				// See NCI V1.0 Specification Table 7. 0x01 = NCI RF Configuration has been reset
//         Serial.print(tmpRxBuffer[5]);
//         Serial.println("");
//         }
//     else
//         {
//         Serial.print(nmbrBytesReceived);
//         Serial.println(" bytes received, 6 bytes expected - error");
//         }

//     Serial.println("Test 005 Cycle ---- End");
//     delay(1000);
//     }
