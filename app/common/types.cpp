/*
 *  Copyright 2013 Embedded Artists AB
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include "types.h"

/*!
    \class Types
    \brief This is a helper class defining common types for this application.

    \ingroup Common

*/


/*!
    \enum Types::DataFormat

    This enum describes different data formats that can be used when
    representing a value as a string.

    \var Types::DataFormat Types::DataFormatHex
    Hexadecimal representation

    \var Types::DataFormat Types::DataFormatDecimal
    Decimal representation

    \var Types::DataFormat Types::DataFormatAscii
    ASCII representation
*/


/*!
    \enum Types::UartParity

    This enum describes different UART parity settings.

    \var Types::UartParity Types::ParityNone
    No parity

    \var Types::UartParity Types::ParityOdd
    Odd parity

    \var Types::UartParity Types::ParityEven
    Even parity

    \var Types::UartParity Types::ParityMark
    Mark parity

    \var Types::UartParity Types::ParitySpace
    Space parity
*/


/*!
    \enum Types::I2CAddress

    This enum describes different I2C address lengths.

    \var Types::I2CAddress Types::I2CAddress_7bit
    7-bit address

    \var Types::I2CAddress Types::I2CAddress_10bit
    10-bit address
*/


/*!
    \enum Types::SpiMode

    This enum describes different SPI modes.

    \var Types::SpiMode Types::SpiMode_0
    Mode 0 - CPOL=0, CPHA=0

    \var Types::SpiMode Types::SpiMode_1
    Mode 1 - CPOL=0, CPHA=1

    \var Types::SpiMode Types::SpiMode_2
    Mode 2 - CPOL=1, CPHA=0

    \var Types::SpiMode Types::SpiMode_3
    Mode 3 - CPOL=1, CPHA=1


*/

/*!
    \enum Types::SpiEnable

    This enum describes different SPI enable modes.

    \var Types::SpiEnable Types::SpiEnableLow
    Enable is active low

    \var Types::SpiEnable Types::SpiEnableHigh
    Enable is active high
*/


/*!
    \enum Types::EmvLogicConvention

    This enum describes different EMV logic conventions.

    \var Types::EmvLogicConvention Types::EmvLogicConvention_Auto
    Determine automatically. Expects IO capture to start with ATR TS

    \var Types::EmvLogicConvention Types::EmvLogicConvention_InverseConvention
    Inverse convention where low level (0) is logic one. MSB first.

    \var Types::EmvLogicConvention Types::EmvLogicConvention_DirectConvention
    Direct convention where high level (1) is logic one. LSB first.
*/

/*!
    \enum Types::EmvProtocol

    This enum describes different EMV protocols.

    \var Types::EmvProtocol Types::EmvProtocol_Auto
    Determine automatically. Expects IO capture to contain ATR T0

    \var Types::EmvProtocol Types::EmvProtocol_T0
    Character Protocol T=0

    \var Types::EmvProtocol Types::EmvProtocol_T1
    Block Protocol T=1
*/



/*!
    Constructs the Types.
*/
Types::Types()
{
}
