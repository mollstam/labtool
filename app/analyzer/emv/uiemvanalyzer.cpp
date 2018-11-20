/*
 *  Copyright 2018 Tobias Mollstam
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
#include "uiemvanalyzer.h"

#include <QPainter>
#include <cmath>

#include "uiemvanalyzerconfig.h"
#include "common/configuration.h"
#include "common/stringutil.h"
#include "capture/cursormanager.h"
#include "device/devicemanager.h"

namespace
{
    enum AnalyzerState {
        STATE_ATR_TS,
        STATE_ATR_T0,
        STATE_ATR_TB1,
        STATE_ATR_TC1,
        STATE_ATR_HISTORICAL_BYTES,

        STATE_COMMAND_CLA,
        STATE_COMMAND_INS,
        STATE_COMMAND_P1,
        STATE_COMMAND_P2,
        STATE_COMMAND_P3,
        STATE_COMMAND_DATA,
        //STATE_COMMAND_LE,

        STATE_RESPONSE_STATUS,
        STATE_RESPONSE_INS,
        STATE_RESPONSE_C0,
        STATE_RESPONSE_DATA,

        STATE_RAW_BYTES,

        STATE_DONE,
    };

    QString stateLabel(AnalyzerState state)
    {
        switch (state)
        {
        case STATE_ATR_TS:
            return "TS";
        case STATE_ATR_T0:
            return "T0";
        case STATE_ATR_TB1:
            return "TB1";
        case STATE_ATR_TC1:
            return "TC1";
        case STATE_ATR_HISTORICAL_BYTES:
            return "Historical Byte";
        default:
            return "";
        }
    }
}

/*!
    Counter used when creating the editable name.
*/
int UiEmvAnalyzer::emvAnalyzerCounter = 0;

/*!
    Name of this analyzer.
*/
const QString UiEmvAnalyzer::signalName = "EMV Analyzer";

/*!
    \class UiEmvAnalyzer
    \brief This class is an EMV protocol analyzer.

    \ingroup Analyzer

    The class will analyze specified digital signals and visualize the
    interpretation as EMV protocol data.

*/


/*!
    Constructs the UiEmvAnalyzer with the given \a parent.
*/
UiEmvAnalyzer::UiEmvAnalyzer(QWidget *parent) :
    UiAnalyzer(parent)
{
    mIoSignalId = -1;
    mRstSignalId = -1;

    mClkFreq = -1;
    mInitialEtu = 0;
    mCurrentEtu = mInitialEtu;
    mLogicConvention = Types::EmvLogicConvention_Auto;
    mProtocol = Types::EmvProtocol_Auto;
    mFormat = Types::DataFormatHex;

    mIdLbl->setText("EMV");
    mNameLbl->setText(QString("EMV %1").arg(emvAnalyzerCounter++));

    // Deallocation: "Qt Object trees" (See UiMainWindow)
    mIoLbl = new QLabel(this);
    // Deallocation: "Qt Object trees" (See UiMainWindow)
    mRstLbl = new QLabel(this);
    // Deallocation: "Qt Object trees" (See UiMainWindow)
    mClkLbl = new QLabel(this);

    QPalette palette= mIoLbl->palette();
    palette.setColor(QPalette::Text, Qt::gray);
    mIoLbl->setPalette(palette);
    mRstLbl->setPalette(palette);
    mClkLbl->setPalette(palette);

    setFixedHeight(110);
}

/*!
    Set the I/O signal ID to \a id.
*/
void UiEmvAnalyzer::setIoSignal(int id)
{
    mIoSignalId = id;
    mIoLbl->setText(QString("I/O: D%1").arg(id));
}


/*!
    \fn int UiEmvAnalyzer::ioSignal() const

    Returns I/O signal ID
*/

/*!
    Set the RST signal ID to \a id.
*/
void UiEmvAnalyzer::setRstSignal(int id)
{
    mRstSignalId = id;
    mRstLbl->setText(QString("RST: D%1").arg(id));
}


/*!
    \fn int UiEmvAnalyzer::rstSignal() const

    Returns RST signal ID
*/

/*!
    Set the CLK signal frequency \a freq Hz.
*/
void UiEmvAnalyzer::setClkFreq(int freq)
{
    mClkFreq = freq;
    mClkLbl->setText(QString("CLK: %1").arg(StringUtil::frequencyToString((double)freq)));
}

/*!
    \fn int UiEmvAnalyzer::clkFreq() const

    Returns CLK frequency
*/

/*!
    \fn void UiEmvAnalyzer::setLogicConvention(Types::EmvLogicConvention convention)

    Set the logic convention to \a convention.
*/

/*!
    \fn Types::EmvLogicConvention UiEmvAnalyzer::logicConvention() const

    Returns the logic convention.
*/

/*!
    \fn void UiEmvAnalyzer::setProtocol(Types::EmvProtocol protocol)

    Set the protocol to \a protocol.
*/

/*!
    \fn Types::EmvProtocol UiEmvAnalyzer::protocol() const

    Returns the protocol.
*/

/*!
    \fn void UiEmvAnalyzer::setDataFormat(Types::DataFormat format)

    Set the data format to \a format.
*/

/*!
    \fn Types::DataFormat UiEmvAnalyzer::dataFormat() const

    Returns the data format.
*/


/*!
    Start to analyze the signal data.
*/
void UiEmvAnalyzer::analyze()
{
    mEmvItems.clear();

    if (mIoSignalId == -1 || mRstSignalId == -1) return;

    CaptureDevice* device = DeviceManager::instance().activeDevice()
            ->captureDevice();

    QVector<int>* ioData = device->digitalData(mIoSignalId);
    QVector<int>* rstData = device->digitalData(mRstSignalId);

    if (ioData == NULL || rstData == NULL) return;
    if (ioData->size() == 0 || rstData->size() == 0) return;

    if (mClkFreq == -1) return;
    mInitialEtu = 372.f / mClkFreq;
    mCurrentEtu = mInitialEtu;
    int minSampleRate = 1 + (1.f / (mInitialEtu * 0.2));

    quint8 numHistoricalBytes = 0x00;
    quint8 extraTermToICCGuardTime = 0; // in etus
    quint8 ICCToTermGuardTime = 0; // in etus

    AnalyzerState state = STATE_ATR_TS;
    if (mLogicConvention == Types::EmvLogicConvention_Auto)
    {
        mProtocol = Types::EmvProtocol_Auto;
    }
    else
    {
        if (mProtocol == Types::EmvProtocol_Auto)
            state = STATE_ATR_T0;
        else
            state = STATE_ATR_TB1;
    }

    mDeterminedLogicConvention = mLogicConvention;
    mDeterminedProtocol = mProtocol;

    EmvItem::DataDirection dataDirection = EmvItem::ICC_TO_TTL;

    if (mDeterminedProtocol == Types::EmvProtocol_T0)
    {
        ICCToTermGuardTime = 12;
    }
    if (mDeterminedProtocol == Types::EmvProtocol_T1)
    {
        // T=1 not currently supported
        EmvItem item(EmvItem::TYPE_ERROR_PROTOCOL, 0, "", 0, -1, dataDirection);
        mEmvItems.append(item);
        state = STATE_DONE;
    }

    int pos = 0;
    int lastStartBitIdx = -1;
    int startBitIdx = -1;
    int nextStartBitPos = 0;
    int bitRank = 0;

    bool expectDirectionSwitch = false;

    quint8 currByte = 0x00;
    EmvCommandMessage currCommand;
    bool commandDone = false;
    int currCommandStartBitIdx = -1;
    quint8 currCommandRemainingData;
    quint8 currCommandRemainingResponseData;
    int currIo = ioData->at(pos);

    if (device->usedSampleRate() < minSampleRate)
    {
        state = STATE_DONE;
        EmvItem item(EmvItem::TYPE_ERROR_RATE, 0, "", startBitIdx, -1, dataDirection);
        mEmvItems.append(item);
    }

    while (state != STATE_DONE) {

        // reached end of data
        if (pos >= ioData->size()) break;
        if (pos >= rstData->size()) break;
        const EmvItem::DataDirection currDataDirection = dataDirection;

        if (rstData->at(pos) == 1)
        {
            currIo = ioData->at(pos);

            if (startBitIdx == -1)
            {
                if (pos >= nextStartBitPos && currIo == 0)
                {
                    startBitIdx = pos;
                    nextStartBitPos = pos + (ICCToTermGuardTime * mCurrentEtu * device->usedSampleRate());
                    bitRank = 1;

                    if (expectDirectionSwitch)
                    {
                        int directionSwitchGuardTime = 16 * mCurrentEtu * device->usedSampleRate(); // T=0 specific
                        if (lastStartBitIdx != -1 && startBitIdx - lastStartBitIdx < directionSwitchGuardTime)
                        {
                            state = STATE_DONE;
                            EmvItem item(EmvItem::TYPE_ERROR_DIRECTION_GUARD_TIME, 0, "", startBitIdx, -1, currDataDirection);
                            mEmvItems.append(item);
                        }
                    }
                    expectDirectionSwitch = false;
                }
            }
            else
            {
                int nextBitPos = round((startBitIdx * (1.0 / device->usedSampleRate()) + ((bitRank + 0.5) * mCurrentEtu)) * device->usedSampleRate());
                int nextBitPosTolerance = round(0.2 * mCurrentEtu * device->usedSampleRate());
                if (abs(pos - nextBitPos) <= nextBitPosTolerance)
                {
                    if (bitRank >= 1 && bitRank <= 8)
                    {
                        if (mDeterminedLogicConvention == Types::EmvLogicConvention_InverseConvention)
                        {
                            int bitToWrite = currIo == 0 ? 1 : 0; // low = logic one
                            currByte = currByte | (bitToWrite << (8 - bitRank)); // msb first
                        }
                        else // treat "auto" as direct convention until determined
                        {
                            currByte = currByte | (currIo << (bitRank - 1)); // lsb first
                        }
                        ++bitRank;
                    }
                    else if (bitRank == 9)
                    {
                        int numSetBits = currIo;
                        quint8 n = currByte;
                        while (n)
                        {
                            n &= (n-1);
                            ++numSetBits;
                        }
                        if (numSetBits % 2 == 0)
                        {
                            // a valid byte has been read

                            AnalyzerState stateForByte = state;
                            bool error = false;

                            if (state == STATE_ATR_TS)
                            {
                                if (currByte == 0x3) // 0x3f (inverse indicator) has value 0x3 if read as direct
                                {
                                    currByte = 0x3f;
                                    mDeterminedLogicConvention = Types::EmvLogicConvention_InverseConvention;
                                    state = STATE_ATR_T0;
                                }
                                else if (currByte == 0x3b)
                                {
                                    mDeterminedLogicConvention = Types::EmvLogicConvention_DirectConvention;
                                    state = STATE_ATR_T0;
                                }
                                else
                                {
                                    EmvItem item(EmvItem::TYPE_ERROR_TS, 0, "", startBitIdx, -1, currDataDirection);
                                    mEmvItems.append(item);
                                    state = STATE_DONE;
                                    error = true;
                                }
                            }
                            else if (state == STATE_ATR_T0)
                            {
                                if ((currByte >> 4) == 0x6)
                                {
                                    mDeterminedProtocol = Types::EmvProtocol_T0;
                                    numHistoricalBytes = currByte & 0xf;
                                    ICCToTermGuardTime = 12;
                                    state = STATE_ATR_TB1;
                                }
                                else if ((currByte >> 4) == 0xe)
                                {
                                    // T=1 not currently supported
                                    EmvItem item(EmvItem::TYPE_ERROR_PROTOCOL, 0, "", startBitIdx, -1, currDataDirection);
                                    mEmvItems.append(item);
                                    state = STATE_DONE;
                                    error = true;
                                }
                                else
                                {
                                    EmvItem item(EmvItem::TYPE_ERROR_T0, currByte, "", startBitIdx, -1, currDataDirection);
                                    mEmvItems.append(item);
                                    state = STATE_DONE;
                                    error = true;
                                }
                            }
                            else if (state == STATE_ATR_TB1)
                            {
                                if (currByte == 0x00)
                                {
                                    if (mDeterminedProtocol == Types::EmvProtocol_T0)
                                    {
                                        state = STATE_ATR_TC1;
                                    }
                                    else
                                    {
                                        state = STATE_DONE;
                                    }
                                }
                                else
                                {
                                    EmvItem item(EmvItem::TYPE_ERROR_TB1, currByte, "", startBitIdx, -1, currDataDirection);
                                    mEmvItems.append(item);
                                    state = STATE_DONE;
                                    error = true;
                                }
                            }
                            else if (state == STATE_ATR_TC1)
                            {
                                extraTermToICCGuardTime = currByte;
                                if (numHistoricalBytes > 0)
                                {
                                    state = STATE_ATR_HISTORICAL_BYTES;
                                }
                                else
                                {
                                    state = STATE_COMMAND_CLA;
                                    dataDirection = EmvItem::TTL_TO_ICC;
                                }
                            }
                            else if (state == STATE_ATR_HISTORICAL_BYTES)
                            {
                                numHistoricalBytes--;
                                if (numHistoricalBytes == 0)
                                {
                                    state = STATE_COMMAND_CLA;
                                    dataDirection = EmvItem::TTL_TO_ICC;
                                }
                            }
                            else if (state == STATE_COMMAND_CLA)
                            {
                                if (currCommand.Case == 0)
                                {
                                    currCommand = EmvCommandMessage();
                                    currCommandStartBitIdx = startBitIdx;
                                }
                                if (currCommand.Case == 4)
                                {
                                    if (currByte != 0x00)
                                    {
                                        EmvItem item(EmvItem::TYPE_ERROR_GENERIC, currByte, "", startBitIdx, -1, currDataDirection);
                                        mEmvItems.append(item);
                                        state = STATE_DONE;
                                        error = true;
                                    }
                                }
                                else
                                {
                                    currCommand.Cla = currByte;
                                }
                                state = STATE_COMMAND_INS;
                            }
                            else if (state == STATE_COMMAND_INS)
                            {
                                if (currCommand.Case == 4)
                                {
                                    if (currByte != 0xc0)
                                    {
                                        EmvItem item(EmvItem::TYPE_ERROR_GENERIC, currByte, "", startBitIdx, -1, currDataDirection);
                                        mEmvItems.append(item);
                                        state = STATE_DONE;
                                        error = true;
                                    }
                                }
                                else
                                {
                                    currCommand.Ins = currByte;
                                }
                                state = STATE_COMMAND_P1;
                            }
                            else if (state == STATE_COMMAND_P1)
                            {
                                if (currCommand.Case == 4)
                                {
                                    if (currByte != 0x00)
                                    {
                                        EmvItem item(EmvItem::TYPE_ERROR_GENERIC, currByte, "", startBitIdx, -1, currDataDirection);
                                        mEmvItems.append(item);
                                        state = STATE_DONE;
                                        error = true;
                                    }
                                }
                                else
                                {
                                    currCommand.P1 = currByte;
                                }
                                state = STATE_COMMAND_P2;
                            }
                            else if (state == STATE_COMMAND_P2)
                            {
                                if (currCommand.Case == 4)
                                {
                                    if (currByte != 0x00)
                                    {
                                        EmvItem item(EmvItem::TYPE_ERROR_GENERIC, currByte, "", startBitIdx, -1, currDataDirection);
                                        mEmvItems.append(item);
                                        state = STATE_DONE;
                                        error = true;
                                    }
                                }
                                else
                                {
                                    currCommand.P2 = currByte;
                                }
                                state = STATE_COMMAND_P3;
                            }
                            else if (state == STATE_COMMAND_P3)
                            {
                                if (currCommand.Case == 4)
                                {
                                    if (currByte != currCommand.Licc)
                                    {
                                        EmvItem item(EmvItem::TYPE_ERROR_GENERIC, currByte, "", startBitIdx, -1, currDataDirection);
                                        mEmvItems.append(item);
                                        state = STATE_DONE;
                                        error = true;
                                    }
                                    else
                                    {
                                        state = STATE_RESPONSE_C0;
                                    }
                                }
                                else
                                {
                                    currCommand.P3 = currByte;
                                    if (currCommand.P3 > 0)
                                    {
                                        currCommand.Data = (quint8*)malloc(currCommand.P3);
                                        currCommandRemainingData = currCommand.P3;
                                    }

                                    if (currCommand.Case == 2)
                                    {
                                        if (currCommand.P3 != currCommand.Licc)
                                        {
                                            EmvItem item(EmvItem::TYPE_ERROR_GENERIC, currByte, "", startBitIdx, -1, currDataDirection);
                                            mEmvItems.append(item);
                                            state = STATE_DONE;
                                            error = true;
                                        }
                                        else
                                        {
                                            state = STATE_RESPONSE_INS;
                                        }
                                    }
                                    else if (currCommand.P3 == 0)
                                    {
                                        currCommand.Data = NULL;
                                        currCommandRemainingData = 0;
                                        state = STATE_RESPONSE_STATUS;
                                    }
                                    else
                                    {
                                        state = STATE_RESPONSE_INS;
                                    }
                                }

                                dataDirection = EmvItem::ICC_TO_TTL;
                            }
                            else if (state == STATE_RESPONSE_STATUS)
                            {
                                if (currCommand.Sw1 == 0xff)
                                {
                                    currCommand.Sw1 = currByte;
                                }
                                else if (currCommand.Sw2 == 0xff)
                                {
                                    currCommand.Sw2 = currByte;
                                    if (currCommand.Sw1 == 0x90
                                        && currCommand.Sw2 == 0x00)
                                    {
                                        if (currCommand.Case == 0 && currCommand.P3 == 0)
                                            currCommand.Case = 1;
                                        else if (currCommand.Case == 0 && currCommand.P3 != 0)
                                            currCommand.Case = 3;
                                        commandDone = true;
                                    }
                                    else if (currCommand.Sw1 == 0x6c)
                                    {
                                        currCommand.Case = 2;
                                        currCommand.Licc = currCommand.Sw2;
                                        currCommand.ResponseData = (quint8*)malloc(currCommand.Licc);
                                        currCommandRemainingResponseData = currCommand.Licc;
                                        currCommand.Sw1 = 0xff;
                                        currCommand.Sw2 = 0xff;
                                        state = STATE_COMMAND_CLA;
                                    }
                                    else if (currCommand.Sw1 == 0x61)
                                    {
                                        currCommand.Case = 4;
                                        currCommand.Licc = currCommand.Sw2;
                                        currCommand.ResponseData = (quint8*)malloc(currCommand.Licc);
                                        currCommandRemainingResponseData = currCommand.Licc;
                                        currCommand.Sw1 = 0xff;
                                        currCommand.Sw2 = 0xff;
                                        state = STATE_COMMAND_CLA;
                                    }

                                    dataDirection = EmvItem::TTL_TO_ICC;
                                }
                                else
                                {
                                    EmvItem item(EmvItem::TYPE_ERROR_GENERIC, currByte, "", startBitIdx, -1, currDataDirection);
                                    mEmvItems.append(item);
                                    state = STATE_DONE;
                                    error = true;
                                }
                            }
                            else if (state == STATE_RESPONSE_INS)
                            {
                                if (currByte != currCommand.Ins)
                                {
                                    EmvItem item(EmvItem::TYPE_ERROR_GENERIC, currByte, "", startBitIdx, -1, currDataDirection);
                                    mEmvItems.append(item);
                                    state = STATE_DONE;
                                    error = true;
                                }
                                else if (currCommand.Case == 2)
                                {
                                    state = STATE_RESPONSE_DATA;
                                }
                                else
                                {
                                    state = STATE_COMMAND_DATA;
                                    dataDirection = EmvItem::TTL_TO_ICC;
                                }
                            }
                            else if (state == STATE_RESPONSE_C0)
                            {
                                if (currByte != 0xc0)
                                {
                                    EmvItem item(EmvItem::TYPE_ERROR_GENERIC, currByte, "", startBitIdx, -1, currDataDirection);
                                    mEmvItems.append(item);
                                    state = STATE_DONE;
                                    error = true;
                                }
                                else
                                {
                                    state = STATE_RESPONSE_DATA;
                                }
                            }
                            else if (state == STATE_RESPONSE_DATA)
                            {
                                memcpy(&currCommand.ResponseData[currCommand.Licc - currCommandRemainingData], &currByte, 1);
                                currCommandRemainingResponseData--;
                                if (currCommandRemainingResponseData == 0)
                                {
                                    state = STATE_RESPONSE_STATUS;
                                }
                            }
                            else if (state == STATE_COMMAND_DATA)
                            {
                                memcpy(&currCommand.Data[currCommand.P3 - currCommandRemainingData], &currByte, 1);
                                currCommandRemainingData--;
                                if (currCommandRemainingData == 0)
                                {
                                    state = STATE_RESPONSE_STATUS;
                                    dataDirection = EmvItem::ICC_TO_TTL;
                                }
                            }

                            if (commandDone)
                            {
                                if (currCommand.Cla == 0x00 && currCommand.Ins == 0xa4)
                                {
                                    currCommand.Label = "SELECT";
                                }
                                else if (currCommand.Cla == 0x00 && currCommand.Ins == 0xb2)
                                {
                                    currCommand.Label = "READ RECORD";
                                }
                                else
                                {
                                    currCommand.Label = "UNKNOWN";
                                }

                                EmvItem item = EmvItem::Create(EmvItem::TYPE_COMMAND_MESSAGE,
                                                               currCommand,
                                                               "Command Message",
                                                               currCommandStartBitIdx,
                                                               pos + (mCurrentEtu * device->usedSampleRate()),
                                                               currDataDirection);
                                mEmvItems.append(item);
                                currCommand = EmvCommandMessage();
                                dataDirection = EmvItem::TTL_TO_ICC;
                                commandDone = false;

                                state = STATE_COMMAND_CLA;
                            }

                            if (error == false)
                            {
                                EmvItem item(EmvItem::TYPE_CHARACTER_FRAME,
                                             currByte,
                                             stateLabel(stateForByte),
                                             startBitIdx,
                                             pos + (mCurrentEtu * device->usedSampleRate()),
                                             currDataDirection);
                                mEmvItems.append(item);
                                lastStartBitIdx = startBitIdx;
                                startBitIdx = -1;
                                bitRank = 0;
                                currByte = 0x00;
                            }
                        }
                        else
                        {
                            state = STATE_DONE;
                            EmvItem item(EmvItem::TYPE_ERROR_PARITY, 0, "", startBitIdx, -1, currDataDirection);
                            mEmvItems.append(item);
                        }
                    }
                }
            }
        }

        if (dataDirection != currDataDirection)
        {
            expectDirectionSwitch = true;
        }

        pos++;
    }

}

/*!
    Configure the analyzer.
*/
void UiEmvAnalyzer::configure(QWidget *parent)
{
    UiEmvAnalyzerConfig dialog(parent);
    dialog.setIoSignal(mIoSignalId);
    dialog.setRstSignal(mRstSignalId);
    dialog.setClkFreq(mClkFreq);
    dialog.setLogicConvention(mLogicConvention);
    dialog.setDataFormat(mFormat);

    dialog.exec();

    setIoSignal(dialog.ioSignal());
    setRstSignal(dialog.rstSignal());
    setClkFreq(dialog.clkFreq());
    setLogicConvention(dialog.logicConvention());
    setDataFormat(dialog.dataFormat());

    analyze();
    update();
}

/*!
    Returns a string representation of this analyzer.
*/
QString UiEmvAnalyzer::toSettingsString() const
{
    // type;name;IO;RST;CLKFreq;LogicConvention;Format

    QString str;
    str.append(UiEmvAnalyzer::signalName);str.append(";");
    str.append(getName());str.append(";");
    str.append(QString("%1;").arg(ioSignal()));
    str.append(QString("%1;").arg(rstSignal()));
    str.append(QString("%1;").arg(clkFreq()));
    str.append(QString("%1;").arg(logicConvention()));
    str.append(QString("%1;").arg(dataFormat()));

    return str;
}

/*!
    Create an EMV analyzer from the string representation \a s.

    \sa toSettingsString
*/
UiEmvAnalyzer* UiEmvAnalyzer::fromSettingsString(const QString &s)
{
    UiEmvAnalyzer* analyzer = NULL;
    QString name;

    bool ok = false;

    do {
        // type;name;IO;RST;CLKFreq;LogicConvention;Format
        QStringList list = s.split(';');
        if (list.size() != 6) break;

        // --- type
        if (list.at(0) != UiEmvAnalyzer::signalName) break;

        // --- name
        name = list.at(1);
        if (name.isNull()) break;

        // --- I/O signal ID
        int ioId = list.at(2).toInt(&ok);
        if (!ok) break;

        // --- RST signal ID
        int rstId = list.at(3).toInt(&ok);
        if (!ok) break;

        // --- CLK freq
        int clkFreq = list.at(4).toInt(&ok);
        if (!ok) break;

        // --- logic convention
        int lc = list.at(5).toInt(&ok);
        if (!ok) break;
        if (lc < 0 || lc >= Types::EmvLogicConvention_Num) break;
        Types::EmvLogicConvention logicConvention = (Types::EmvLogicConvention)lc;

        // --- data format
        Types::DataFormat format;
        int f = list.at(6).toInt(&ok);
        if (!ok) break;
        if (f < 0 || f >= Types::DataFormatNum) break;
        format = (Types::DataFormat)f;

        // Deallocation: caller of this function must delete the analyzer
        analyzer = new UiEmvAnalyzer();
        if (analyzer == NULL) break;

        analyzer->setSignalName(name);
        analyzer->setIoSignal(ioId);
        analyzer->setRstSignal(rstId);
        analyzer->setClkFreq(clkFreq);
        analyzer->setLogicConvention(logicConvention);
        analyzer->setDataFormat(format);

    } while (false);

    return analyzer;
}

/*!
    Paint event handler responsible for painting this widget.
*/
void UiEmvAnalyzer::paintEvent(QPaintEvent *event)
{
    (void)event;
    QPainter painter(this);

    int textMargin = 3;

    // -----------------
    // draw background
    // -----------------
    paintBackground(&painter);

    painter.setClipRect(plotX(), 0, width()-infoWidth(), height());

    CaptureDevice* device = DeviceManager::instance().activeDevice()
            ->captureDevice();
    int sampleRate = device->usedSampleRate();


    double from = 0;
    double to = 0;
    int fromIdx = 0;
    int toIdx = 0;

    int h = 10;

    QString ioShortTxt;
    QString ioLongTxt;

    if (mSelected) {
        QPen pen = painter.pen();
        pen.setColor(Qt::gray);
        painter.setPen(pen);
        QRectF ioRect(plotX()+4, 5, 100, 2*h);
        painter.drawText(ioRect, Qt::AlignLeft|Qt::AlignVCenter, "I/O");
    }


    QPen pen = painter.pen();
    pen.setColor(Configuration::instance().analyzerColor());
    painter.setPen(pen);

    int prevCharacterFromIdx = -1;
    double prevCharacterFrom = -1;

    for (int i = 0; i < mEmvItems.size(); i++) {
        EmvItem item = mEmvItems.at(i);

        fromIdx = item.startIdx;
        toIdx = item.stopIdx;

        typeAndValueAsString(item.type, item.itemValue, item.label, ioShortTxt,
                             ioLongTxt);

        int shortTextWidth = painter.fontMetrics().width(ioShortTxt);
        int longTextWidth = painter.fontMetrics().width(ioLongTxt);


        from = mTimeAxis->timeToPixelRelativeRef((double)fromIdx/sampleRate);

        // no need to draw when signal is out of plot area
        if (from > width()) continue;

        if (toIdx != -1) {
            to = mTimeAxis->timeToPixelRelativeRef((double)toIdx/sampleRate);
        }
        else  {

            // see if the long text version fits
            to = from + longTextWidth+textMargin*2;

            if (i+1 < mEmvItems.size()) {

                // get position for the start of the next item
                double tmp = mTimeAxis->timeToPixelRelativeRef(
                            (double)mEmvItems.at(i+1).startIdx/sampleRate);


                // if 'to' overlaps check if short text fits
                if (to > tmp) {

                    to = from + shortTextWidth+textMargin*2;

                    // 'to' overlaps next item -> limit to start of next item
                    if (to > tmp) {
                        to = tmp;
                    }

                }


            }
        }

        bool shouldPaintSignal = true;
        if (item.type == EmvItem::TYPE_CHARACTER_FRAME)
        {
            painter.save();
            painter.translate(0, 5);
            paintBinary(&painter, from, to, item.get<int>());
            painter.restore();

            if (prevCharacterFromIdx > 0)
            {
                painter.save();
                painter.translate(0, 25);
                paintByteInterval(&painter, prevCharacterFrom, from, fromIdx - prevCharacterFromIdx);
                painter.restore();
            }
            prevCharacterFromIdx = fromIdx;
            prevCharacterFrom = from;
        }
        else if (item.type == EmvItem::TYPE_COMMAND_MESSAGE)
        {
            shouldPaintSignal = false;

            painter.save();
            painter.translate(0, 87);
            paintCommandMessage(&painter, from, to, item.get<EmvCommandMessage>());
            painter.restore();
        }

        if (shouldPaintSignal)
        {
            painter.save();
            painter.translate(0, 40);
            paintSignal(&painter, from, to, h, ioShortTxt, ioLongTxt, item.dataDirection);
            painter.restore();
        }

    }

}

/*!
    Event handler called when this widget is being shown
*/
void UiEmvAnalyzer::showEvent(QShowEvent* event)
{
    (void)event;
    doLayout();
    setMinimumInfoWidth(calcMinimumWidth());
}

/*!
    Called when the info width has changed for this widget.
*/
void UiEmvAnalyzer::infoWidthChanged()
{
    doLayout();
}

/*!
    Position the child widgets.
*/
void UiEmvAnalyzer::doLayout()
{
    UiSimpleAbstractSignal::doLayout();

    QRect r = infoContentRect();
    int y = r.top();

    mIdLbl->move(r.left(), y);

    int x = mIdLbl->pos().x()+mIdLbl->width() + 10;
    mNameLbl->move(x, y);
    mEditName->move(x, y);

    mIoLbl->move(r.left(),
                 r.bottom()-mIoLbl->height()-mClkLbl->height());
    mRstLbl->move(r.left()+5+mIoLbl->width(),
                  r.bottom()-mIoLbl->height()-mClkLbl->height());

    mClkLbl->move(r.left(), r.bottom()-mClkLbl->height());
}

/*!
    Calculate and return the minimum width for this widget.
*/
int UiEmvAnalyzer::calcMinimumWidth()
{
    int w = mNameLbl->pos().x() + mNameLbl->minimumSizeHint().width();
    if (mEditName->isVisible()) {
        w = mEditName->pos().x() + mEditName->width();
    }

    int w2 = mRstLbl->pos().x()+mRstLbl->width();
    if (w2 > w) w = w2;

    w2 = mClkLbl->pos().x()+mClkLbl->width();
    if (w2 > w) w = w2;

    return w+infoContentMargin().right();
}

/*!
    Convert EMV \a type and data \a value to string representation. A short
    and long representation is returned in \a shortTxt and \a longTxt.
*/
void UiEmvAnalyzer::typeAndValueAsString(EmvItem::ItemType type,
                                         void* valuePtr,
                                         QString label,
                                         QString &shortTxt,
                                         QString &longTxt)
{


    switch(type) {
    case EmvItem::TYPE_CHARACTER_FRAME:
    {
        int value = 0;
        if (valuePtr != NULL)
            value = *(int*)valuePtr;
        shortTxt = formatValue(mFormat, value);
        if (label.length() > 0)
            longTxt = QString("%1: %2").arg(label).arg(formatValue(mFormat, value));
        else
            longTxt = formatValue(mFormat, value);
        break;
    }
    case EmvItem::TYPE_ERROR_GENERIC:
    {
        shortTxt = "ERR";
        longTxt = "Generic error";
        break;
    }
    case EmvItem::TYPE_ERROR_RATE:
        shortTxt = "ERR";
        longTxt = "Too low sample rate";
        break;
    case EmvItem::TYPE_ERROR_PARITY:
        shortTxt = "ERR";
        longTxt = "Bad parity bit";
        break;
    case EmvItem::TYPE_ERROR_TS:
    {
        int value = 0;
        if (valuePtr != NULL)
            value = *(int*)valuePtr;
        shortTxt = "ERR";
        longTxt = QString("Invalid T1 character %1, unknown logic convention").arg(formatValue(Types::DataFormatHex, value));
        break;
    }
    case EmvItem::TYPE_ERROR_T0:
    {
        int value = 0;
        if (valuePtr != NULL)
            value = *(int*)valuePtr;
        shortTxt = "ERR";
        longTxt = QString("Invalid T0 character %1").arg(formatValue(Types::DataFormatHex, value));
        break;
    }
    case EmvItem::TYPE_ERROR_PROTOCOL:
        shortTxt = "ERR";
        longTxt = QString("Indicated protocol not supported");
        break;
    case EmvItem::TYPE_ERROR_TB1:
    {
        int value = 0;
        if (valuePtr != NULL)
            value = *(int*)valuePtr;
        shortTxt = "ERR";
        longTxt = QString("Invalid TB1 character %1, expected 0x00").arg(formatValue(Types::DataFormatHex, value));
        break;
    }
    case EmvItem::TYPE_ERROR_DIRECTION_GUARD_TIME:
    {
        shortTxt = "ERR";
        longTxt = "Expected direction change but minimum guard time have not yet passed";
        break;
    }
    }

}

/*!
    Paint signal data.
*/
void UiEmvAnalyzer::paintSignal(QPainter* painter, double from, double to,
                                int h, QString &shortTxt, QString &longTxt, EmvItem::DataDirection dataDirection)
{

    int shortTextWidth = painter->fontMetrics().width(shortTxt);
    int longTextWidth = painter->fontMetrics().width(longTxt);

    painter->save();
    int yOffset = dataDirection == EmvItem::TTL_TO_ICC ? 0 : 15;
    painter->translate(0, yOffset);

    if (to-from > 4)
    {
        painter->drawLine(from, 0, from+2, -h);
        painter->drawLine(from, 0, from+2, h);

        painter->drawLine(from+2, -h, to-2, -h);
        painter->drawLine(from+2, h, to-2, h);

        painter->drawLine(to, 0, to-2, -h);
        painter->drawLine(to, 0, to-2, h);
    }
    else // drawing a vertical line when the allowed width is too small
    {
        painter->drawLine(from, -h, from, h);
    }

    // only draw the text if it fits between 'from' and 'to'
    QRectF textRect(from+1, -h, (to-from), 2*h);
    if (longTextWidth < (to-from)) {
        painter->drawText(textRect, Qt::AlignCenter, longTxt);
    }
    else if (shortTextWidth < (to-from)) {
        painter->drawText(textRect, Qt::AlignCenter, shortTxt);
    }

    painter->restore();
}

/*!
    Paint character byte as binary.
*/
void UiEmvAnalyzer::paintBinary(QPainter* painter, double from, double to, int value)
{
    double widthPerBit = (to-from) / 10;
    if (widthPerBit > 8)
    {
        QPen pen = painter->pen();
        pen.setColor(QColor(255, 255, 0));
        painter->setPen(pen);

        for (int bitIndex = 0; bitIndex < 8; ++bitIndex)
        {
            double bitFrom = from + (widthPerBit * (bitIndex + 1));
            QRectF textRect(bitFrom, -10, widthPerBit, 20);
            bool bitValue = false;
            if (mDeterminedLogicConvention == Types::EmvLogicConvention_InverseConvention)
            { // msb first
                bitValue = (value >> (7 - bitIndex)) & 1;
            }
            else if (mDeterminedLogicConvention == Types::EmvLogicConvention_DirectConvention)
            { // lsb first
                bitValue = (value >> bitIndex) & 1;
            }
            painter->drawText(textRect, Qt::AlignCenter, bitValue ? "1" : "0");
        }
    }
}

/*!
    Paint interval between start bits.
*/
void UiEmvAnalyzer::paintByteInterval(QPainter* painter, double from, double to, int interval)
{
    if (to-from > 4)
    {
        QColor color(0, 255, 255);
        CaptureDevice* device = DeviceManager::instance().activeDevice()
                ->captureDevice();
        double etus = (double)interval / device->usedSampleRate() / mCurrentEtu;
        if (etus < 16.0) // T=0 specific
        {
            color = QColor(100, 180, 180);
        }

        QPen pen = painter->pen();
        pen.setColor(color);
        painter->setPen(pen);

        painter->drawLine(from+2, -2, from+2, 2);
        painter->drawLine(to-2, -2, to-2, 2);
        painter->drawLine(from+2, 0, to-2, 0);

        QFont font = painter->font();
        font.setPixelSize(9);
        painter->setFont(font);

        QRectF textRect(from+3, -12, (to-from), 12);
        QString text = QString("%1 ETU").arg(etus, 0, 'f', 1);
        painter->drawText(textRect, Qt::AlignLeft, text);
    }
}

/*!
    Paint command message.
*/
void UiEmvAnalyzer::paintCommandMessage(QPainter* painter, double from, double to, EmvCommandMessage message)
{
    int h = 18;
    if (to-from > 4)
    {
        painter->drawLine(from, 0, from+2, -h);
        painter->drawLine(from, 0, from+2, h);

        painter->drawLine(from+2, -h, to-2, -h);
        painter->drawLine(from+2, h, to-2, h);

        painter->drawLine(to, 0, to-2, -h);
        painter->drawLine(to, 0, to-2, h);

        if (to-from > 100)
        {
            QRectF textRect(from+5, -h+3, (to-from-10), h*2-6);
            QString dataHex = "";
            QString dataAscii = "";
            for (int i = 0; i < message.P3; ++i)
            {
                if (dataHex.length() > 0)
                {
                    dataHex += " ";
                }
                dataHex += formatValue(Types::DataFormatHex, message.Data[i]);
                QChar c(message.Data[i]);
                if (c.isPrint())
                    dataAscii += c;
                else
                    dataAscii += ".";
            }
            QString text = QString("%1 (Case %9)   --   CLA: %2  INS: %3  P1: %4  P2: %5  P3: %6\nCommand Data: %7 (%8)")
                    .arg(message.Label)
                    .arg(formatValue(Types::DataFormatHex, message.Cla))
                    .arg(formatValue(Types::DataFormatHex, message.Ins))
                    .arg(message.P1)
                    .arg(message.P2)
                    .arg(message.P3)
                    .arg(dataHex)
                    .arg(dataAscii)
                    .arg(message.Case);
            painter->drawText(textRect, Qt::AlignLeft, text);
        }
    }
}



