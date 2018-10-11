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
#ifndef UIEMVANALYZER_H
#define UIEMVANALYZER_H

#include <QWidget>

#include "analyzer/uianalyzer.h"
#include "capture/uicursor.h"

/*!
    \class EmvItem
    \brief Container class for EMV items.

    \ingroup Analyzer

    \internal

*/
class EmvItem {
public:

    /*!
        EMV item type
    */
    enum ItemType {
        TYPE_CHARACTER_FRAME,
        TYPE_ERROR_RATE,
        TYPE_ERROR_PARITY,
        TYPE_ERROR_TS,
        TYPE_ERROR_T0,
        TYPE_ERROR_PROTOCOL,
        TYPE_ERROR_TB1,
    };

    // default constructor needed in order to add this to QVector
    /*! Default constructor */
    EmvItem() {
    }

    /*! Constructs a new container */
    EmvItem(ItemType type, int itemValue, int startIdx, int stopIdx) {
        this->type = type;
        this->itemValue = itemValue;
        this->startIdx = startIdx;
        this->stopIdx = stopIdx;
    }

    /*! type */
    ItemType type;
    /*! item value */
    int itemValue;
    /*! item start index */
    int startIdx;
    /*! item stop index */
    int stopIdx;
};

class UiEmvAnalyzer : public UiAnalyzer
{
    Q_OBJECT
public:

    static const QString signalName;

    explicit UiEmvAnalyzer(QWidget *parent = 0);

    void setIoSignal(int id);
    int ioSignal() const {return mIoSignalId;}

    void setRstSignal(int id);
    int rstSignal() const {return mRstSignalId;}

    void setClkFreq(int freq);
    int clkFreq() const {return mClkFreq;}

    void setLogicConvention(Types::EmvLogicConvention convention) { mLogicConvention = convention; }
    Types::EmvLogicConvention logicConvention() const {return mLogicConvention;}

    void setProtocol(Types::EmvProtocol protocol) { mProtocol = protocol; }
    Types::EmvProtocol protocol() const {return mProtocol;}

    void setDataFormat(Types::DataFormat format) {mFormat = format;}
    Types::DataFormat dataFormat() const {return mFormat;}

    void analyze();
    void configure(QWidget* parent);

    QString toSettingsString() const;
    static UiEmvAnalyzer* fromSettingsString(const QString &settings);

signals:

public slots:

protected:
    void paintEvent(QPaintEvent *event);
    void showEvent(QShowEvent* event);


private:

    enum {
        SignalIdMarginRight = 10
    };

    int mIoSignalId;
    int mRstSignalId;
    int mClkFreq;
    double mInitialEtu;
    double mCurrentEtu;
    Types::EmvLogicConvention mLogicConvention;
    Types::EmvProtocol mProtocol;

    Types::DataFormat mFormat;

    QLabel* mIoLbl;
    QLabel* mRstLbl;
    QLabel* mClkLbl;

    QVector<EmvItem> mEmvItems;

    static int emvAnalyzerCounter;

    void infoWidthChanged();
    void doLayout();
    int calcMinimumWidth();

    void typeAndValueAsString(EmvItem::ItemType type,
                                 int value,
                                 QString &shortTxt,
                                 QString &longTxt);

    void paintSignal(QPainter* painter, double from, double to,
                     int h, QString &shortTxt, QString &longTxt);
};

#endif // UIEMVANALYZER_H
