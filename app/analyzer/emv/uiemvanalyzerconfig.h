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
#ifndef UIEMVANALYZERCONFIG_H
#define UIEMVANALYZERCONFIG_H

#include <QWidget>
#include <QComboBox>

#include "analyzer/uianalyzerconfig.h"
#include "common/types.h"

class UiEmvAnalyzerConfig : public UiAnalyzerConfig
{
    Q_OBJECT
public:
    explicit UiEmvAnalyzerConfig(QWidget *parent = 0);

    void setIoSignal(int id);
    int ioSignal();

    void setRstSignal(int id);
    int rstSignal();

    void setClkFreq(int freq);
    int clkFreq();

    void setLogicConvention(Types::EmvLogicConvention convention);
    Types::EmvLogicConvention logicConvention();

    Types::DataFormat dataFormat();
    void setDataFormat(Types::DataFormat format);

signals:

public slots:

private slots:
    void verifyChoice();

private:

    QComboBox* mEmvIoSignalBox;
    QComboBox* mEmvRstSignalBox;
    QLineEdit* mEmvClkFreqBox;
    QComboBox* mEmvLogicConventionBox;
    QComboBox* mFormatBox;

};

#endif // UIEMVANALYZERCONFIG_H
