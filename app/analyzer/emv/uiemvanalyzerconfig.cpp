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
#include "uiemvanalyzerconfig.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QMessageBox>

#include "common/inputhelper.h"


/*!
    \class UiEmvAnalyzerConfig
    \brief Dialog window used to configure the EMV analyzer.

    \ingroup Analyzer

*/


/*!
    Constructs the UiEmvAnalyzerConfig with the given \a parent.
*/
UiEmvAnalyzerConfig::UiEmvAnalyzerConfig(QWidget *parent) :
    UiAnalyzerConfig(parent)
{
    setWindowTitle(tr("EMV Analyzer"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Deallocation: Re-parented when calling verticalLayout->addLayout
    QFormLayout* formLayout = new QFormLayout;

    mEmvIoSignalBox = InputHelper::createSignalBox(this, 0);
    formLayout->addRow(tr("I/O: "), mEmvIoSignalBox);

    mEmvRstSignalBox = InputHelper::createSignalBox(this, 2);
    formLayout->addRow(tr("RST: "), mEmvRstSignalBox);

    mEmvClkFreqBox = InputHelper::createEmvClkFreqBox(this, 1);
    formLayout->addRow(tr("Clock frequency (Hz): "), mEmvClkFreqBox);

    mFormatBox = InputHelper::createFormatBox(this, Types::DataFormatHex);
    formLayout->addRow(tr("Data format: "), mFormatBox);

    // Deallocation: Ownership changed when calling setLayout
    QVBoxLayout* verticalLayout = new QVBoxLayout();

    // Deallocation: "Qt Object trees" (See UiMainWindow)
    QDialogButtonBox* bottonBox = new QDialogButtonBox(
                QDialogButtonBox::Ok,
                Qt::Horizontal,
                this);
    bottonBox->setCenterButtons(true);

    connect(bottonBox, SIGNAL(accepted()), this, SLOT(verifyChoice()));

    verticalLayout->addLayout(formLayout);
    verticalLayout->addWidget(bottonBox);


    setLayout(verticalLayout);
}

/*!
    Set the I/O signal ID to \a id.
*/
void UiEmvAnalyzerConfig::setIoSignal(int id)
{
    InputHelper::setInt(mEmvIoSignalBox, id);
}

/*!
    Returns the I/O signal ID.
*/
int UiEmvAnalyzerConfig::ioSignal()
{
    return InputHelper::intValue(mEmvIoSignalBox);
}

/*!
    Set the RST signal ID to \a id.
*/
void UiEmvAnalyzerConfig::setRstSignal(int id)
{
    InputHelper::setInt(mEmvRstSignalBox, id);
}

/*!
    Returns the RST signal ID.
*/
int UiEmvAnalyzerConfig::rstSignal()
{
    return InputHelper::intValue(mEmvRstSignalBox);
}

/*!
    Set the CLK frequency \a freq Hz.
*/
void UiEmvAnalyzerConfig::setClkFreq(int freq)
{
    mEmvClkFreqBox->setText(QString("%1").arg(freq));
}

/*!
    Returns the CLK signal ID.
*/
int UiEmvAnalyzerConfig::clkFreq()
{
    return InputHelper::intValue(mEmvClkFreqBox);
}

/*!
    Returns the data format.
*/
Types::DataFormat UiEmvAnalyzerConfig::dataFormat()
{
    int f = InputHelper::intValue(mFormatBox);
    return (Types::DataFormat)f;
}

/*!
    Set the data format to \a format.
*/
void UiEmvAnalyzerConfig::setDataFormat(Types::DataFormat format)
{
    InputHelper::setInt(mFormatBox, (int)format);
}

/*!
    Verify the choices.
*/
void UiEmvAnalyzerConfig::verifyChoice()
{
    bool unique = true;
    int s[2];

    s[0] = InputHelper::intValue(mEmvIoSignalBox);
    s[1] = InputHelper::intValue(mEmvRstSignalBox);

    for (int i = 0; i < 2; i++) {
        for (int j= 0; j < 2; j++) {
            if (i == j) continue;

            if (s[i] == s[j]) {
                unique = false;
                break;
            }

        }
    }

    int freq = InputHelper::intValue(mEmvClkFreqBox);
    bool validFreq = freq >= 1000000 && freq <= 5000000;

    if (!unique)
    {
        QMessageBox::warning(
                    this,
                    tr("Invalid choice"),
                    tr("Signals must be unique"));
    }
    if (!validFreq)
    {
        QMessageBox::warning(
                    this,
                    tr("Invalid frequency"),
                    tr("Clock must be in range 1 MHz - 5 MHz"));
    }
    else
    {
        accept();
    }

}
