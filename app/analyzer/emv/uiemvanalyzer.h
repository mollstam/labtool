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

struct EmvCommandMessage
{
    quint8 Cla;
    quint8 Ins;
    quint8 P1;
    quint8 P2;
    quint8 Lc;
    quint8* Data;
    quint8 Le;

    const char* Label;
    quint8 Case;

    EmvCommandMessage()
        : Cla(0), Ins(0), P1(0), P2(0), Lc(0), Data(NULL), Le(0), Label(NULL), Case(0)
    {
    }

    ~EmvCommandMessage()
    {
        if (Data != NULL)
        {
            free(Data);
            Data = NULL;
        }
    }

    EmvCommandMessage(const EmvCommandMessage& other)
    {
        this->Cla = other.Cla;
        this->Ins = other.Ins;
        this->P1 = other.P1;
        this->P2 = other.P2;
        this->Lc = other.Lc;
        if (this->Lc > 0)
        {
            this->Data = (quint8*)malloc(this->Lc);
            memcpy(this->Data, other.Data, this->Lc);
        }
        else
        {
            this->Data = NULL;
        }
        this->Le = other.Le;
        this->Label = other.Label;
        this->Case = other.Case;
    }
};

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
        TYPE_COMMAND_MESSAGE,

        TYPE_ERROR_RATE,
        TYPE_ERROR_PARITY,
        TYPE_ERROR_TS,
        TYPE_ERROR_T0,
        TYPE_ERROR_PROTOCOL,
        TYPE_ERROR_TB1,
    };

    template<typename T>
    static EmvItem Create(ItemType type, T& value, QString label, int startIdx, int stopIdx, bool fromTtl)
    {
        return EmvItem(type, &value, sizeof(value), label, startIdx, stopIdx, fromTtl);
    }

    // default constructor needed in order to add this to QVector
    /*! Default constructor */
    EmvItem() {
        this->itemValueSize = 0;
        this->itemValue = NULL;
        this->fromTtl = true;
    }

    EmvItem(ItemType type, int value, QString label, int startIdx, int stopIdx, bool fromTtl)
    {
        this->type = type;
        this->itemValueSize = sizeof(value);
        this->itemValue = malloc(this->itemValueSize);
        memcpy(this->itemValue, &value, this->itemValueSize);
        this->label = label;
        this->startIdx = startIdx;
        this->stopIdx = stopIdx;
        this->fromTtl = fromTtl;
    }

    /*! Constructs a new container */
    EmvItem(ItemType type, void* itemValuePtr, size_t itemValueSize, QString label, int startIdx, int stopIdx, bool fromTtl) {
        this->type = type;
        this->itemValueSize = itemValueSize;
        this->itemValue = malloc(itemValueSize);
        memcpy(this->itemValue, itemValuePtr, itemValueSize);
        this->label = label;
        this->startIdx = startIdx;
        this->stopIdx = stopIdx;
        this->fromTtl = fromTtl;
    }

    ~EmvItem()
    {
        if (this->itemValue)
        {
            free(this->itemValue);
            this->itemValueSize = 0;
            this->itemValue = NULL;
        }
    }

    EmvItem(const EmvItem& other)
    {
        this->type = other.type;
        if (other.itemValue != NULL)
        {
            this->itemValueSize = other.itemValueSize;
            this->itemValue = malloc(other.itemValueSize);
            memcpy(this->itemValue, other.itemValue, other.itemValueSize);
        }
        else
        {
            this->itemValue = NULL;
            this->itemValueSize = 0;
        }
        this->label = other.label;
        this->startIdx = other.startIdx;
        this->stopIdx = other.stopIdx;
        this->fromTtl = other.fromTtl;
    }

    template<typename T>
    T get() const
    {
        return *(T*)itemValue;
    }

    /*! type */
    ItemType type;
    /*! item value */
    void* itemValue;
    size_t itemValueSize;
    /*! item label */
    QString label;
    /*! item start index */
    int startIdx;
    /*! item stop index */
    int stopIdx;
    /*! if true from the TTL to ICC, otherwise from ICC to TTL */
    bool fromTtl;
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
    Types::EmvLogicConvention mDeterminedLogicConvention; // may not be auto
    Types::EmvProtocol mProtocol;
    Types::EmvProtocol mDeterminedProtocol; // may to be auto

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
                                 void* valuePtr,
                                 QString label,
                                 QString &shortTxt,
                                 QString &longTxt);

    void paintSignal(QPainter* painter, double from, double to,
                     int h, QString &shortTxt, QString &longTxt);
    void paintBinary(QPainter* painter, double from, double to, int value);
    void paintByteInterval(QPainter* painter, double from, double to, int interval);
    void paintCommandMessage(QPainter* painter, double from, double to, EmvCommandMessage message);

};

#endif // UIEMVANALYZER_H
