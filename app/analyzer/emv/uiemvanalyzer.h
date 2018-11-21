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

#define MAX_NOTES 64

struct EmvCommandMessage
{
    quint8 Cla;
    quint8 Ins;
    quint8 P1;
    quint8 P2;
    quint8 P3;
    quint8* Data;

    quint8 Sw1;
    quint8 Sw2;

    QString Label;
    quint8 Case;
    quint8 Licc;
    quint8* ResponseData;

    int NoteCount;
    int NoteKey[MAX_NOTES];
    QString NoteValue[MAX_NOTES];

    EmvCommandMessage()
        : Cla(0)
        , Ins(0)
        , P1(0)
        , P2(0)
        , P3(0)
        , Data(NULL)
        , Sw1(0xff)
        , Sw2(0xff)
        , Case(0)
        , Licc(0)
        , ResponseData(NULL)
        , NoteCount(0)
    {
    }

    ~EmvCommandMessage()
    {
        if (Data != NULL)
        {
            free(Data);
            Data = NULL;
        }
        if (ResponseData != NULL)
        {
            free(ResponseData);
            ResponseData = NULL;
        }
    }

    EmvCommandMessage(const EmvCommandMessage& other)
    {
        this->Cla = other.Cla;
        this->Ins = other.Ins;
        this->P1 = other.P1;
        this->P2 = other.P2;
        this->P3 = other.P3;
        if (this->P3 > 0)
        {
            this->Data = (quint8*)malloc(this->P3);
            memcpy(this->Data, other.Data, this->P3);
        }
        else
        {
            this->Data = NULL;
        }
        this->Label = other.Label;
        this->Sw1 = other.Sw1;
        this->Sw2 = other.Sw2;
        this->Case = other.Case;
        this->Licc = other.Licc;
        if (this->Licc > 0)
        {
            this->ResponseData = (quint8*)malloc(this->Licc);
            memcpy(this->ResponseData, other.ResponseData, this->Licc);
        }
        else
        {
            this->ResponseData = NULL;
        }
        this->NoteCount = other.NoteCount;
        memcpy(&this->NoteKey, &other.NoteKey, sizeof(other.NoteKey));
        memcpy(&this->NoteValue, &other.NoteValue, sizeof(other.NoteValue));
    }

    void WriteToStream(QDataStream& stream)
    {
        stream << Cla;
        stream << Ins;
        stream << P1;
        stream << P2;
        stream << P3;
        if (P3 > 0)
        {
            stream.writeRawData((const char*)Data, P3);
        }
        stream << Label;
        stream << Sw1;
        stream << Sw2;
        stream << Case;
        stream << Licc;
        if (Licc > 0)
        {
            stream.writeRawData((const char*)ResponseData, Licc);
        }
        stream << NoteCount;
        for (int i = 0; i < NoteCount; ++i)
        {
            stream << NoteKey[i];
            stream << NoteValue[i];
        }
    }

    void ReadFromStream(QDataStream& stream)
    {
        stream >> Cla;
        stream >> Ins;
        stream >> P1;
        stream >> P2;
        stream >> P3;
        if (P3 > 0)
        {
            if (Data != 0)
                free(Data);
            Data = (quint8*)malloc(P3);
            stream.readRawData((char*)Data, P3);
        }
        stream >> Label;
        stream >> Sw1;
        stream >> Sw2;
        stream >> Case;
        stream >> Licc;
        if (Licc > 0)
        {
            if (ResponseData != 0)
                free(ResponseData);
            ResponseData = (quint8*)malloc(Licc);
            stream.readRawData((char*)ResponseData, Licc);
        }
        stream >> NoteCount;
        for (int i = 0; i < NoteCount; ++i)
        {
            stream >> NoteKey[i];
            stream >> NoteValue[i];
        }
    }

    void AddNote(int pos, QString note)
    {
        if (NoteCount >= MAX_NOTES)
            return;

        NoteKey[NoteCount] = pos;
        NoteValue[NoteCount] = note;
        NoteCount += 1;
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

        TYPE_ERROR_GENERIC,
        TYPE_ERROR_RATE,
        TYPE_ERROR_PARITY,
        TYPE_ERROR_TS,
        TYPE_ERROR_T0,
        TYPE_ERROR_PROTOCOL,
        TYPE_ERROR_TB1,
        TYPE_ERROR_DIRECTION_GUARD_TIME,
    };

    enum DataDirection {
        UNKNOWN,
        TTL_TO_ICC,
        ICC_TO_TTL,
    };

    template<typename T>
    static EmvItem Create(ItemType type, T& value, QString label, int startIdx, int stopIdx, DataDirection dataDirection)
    {
        QByteArray bytes;
        QDataStream stream(&bytes, QIODevice::WriteOnly);
        value.WriteToStream(stream);
        return EmvItem(type, bytes.data(), bytes.size(), label, startIdx, stopIdx, dataDirection);
    }

    // default constructor needed in order to add this to QVector
    /*! Default constructor */
    EmvItem() {
        this->itemValueSize = 0;
        this->itemValue = NULL;
        this->dataDirection = UNKNOWN;
    }

    EmvItem(ItemType type, int value, QString label, int startIdx, int stopIdx, DataDirection dataDirection)
    {
        this->type = type;
        this->itemValueSize = sizeof(value);
        this->itemValue = malloc(this->itemValueSize);
        memcpy(this->itemValue, &value, this->itemValueSize);
        this->label = label;
        this->startIdx = startIdx;
        this->stopIdx = stopIdx;
        this->dataDirection = dataDirection;
    }

    /*! Constructs a new container */
    EmvItem(ItemType type, void* itemValuePtr, size_t itemValueSize, QString label, int startIdx, int stopIdx, DataDirection dataDirection) {
        this->type = type;
        this->itemValueSize = itemValueSize;
        this->itemValue = malloc(itemValueSize);
        memcpy(this->itemValue, itemValuePtr, itemValueSize);
        this->label = label;
        this->startIdx = startIdx;
        this->stopIdx = stopIdx;
        this->dataDirection = dataDirection;
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
        this->dataDirection = other.dataDirection;
    }

    template<typename T>
    T get() const
    {
        return *(T*)itemValue;
    }

    template<typename T>
    T getComplex() const
    {
        T t;
        QByteArray bytes((const char*)itemValue, itemValueSize);
        QDataStream stream(bytes);
        t.ReadFromStream(stream);
        return t;
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
    /*! which direction the item had on the data line */
    DataDirection dataDirection;
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
                     int h, QString &shortTxt, QString &longTxt, EmvItem::DataDirection dataDirection);
    void paintBinary(QPainter* painter, double from, double to, int value);
    void paintByteInterval(QPainter* painter, double from, double to, int interval);
    void paintCommandMessage(QPainter* painter, double from, double to, EmvCommandMessage message);

};

#endif // UIEMVANALYZER_H
