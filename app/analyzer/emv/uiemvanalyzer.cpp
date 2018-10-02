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

#include "uiemvanalyzerconfig.h"
#include "common/configuration.h"
#include "common/stringutil.h"
#include "capture/cursormanager.h"
#include "device/devicemanager.h"

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

    setFixedHeight(60);
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

    bool done = false;
    int pos = 0;

    int startIdx = -1;

    while (!done) {

        // reached end of data
        if (pos >= ioData->size()) break;

        // TODO actually implement analyzer logic

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
    dialog.setDataFormat(mFormat);

    dialog.exec();

    setIoSignal(dialog.ioSignal());
    setRstSignal(dialog.rstSignal());
    setClkFreq(dialog.clkFreq());
    setDataFormat(dialog.dataFormat());

    analyze();
    update();
}

/*!
    Returns a string representation of this analyzer.
*/
QString UiEmvAnalyzer::toSettingsString() const
{
    // type;name;IO;RST;CLK;Format

    QString str;
    str.append(UiEmvAnalyzer::signalName);str.append(";");
    str.append(getName());str.append(";");
    str.append(QString("%1;").arg(ioSignal()));
    str.append(QString("%1;").arg(rstSignal()));
    str.append(QString("%1;").arg(clkFreq()));
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
        // type;name;IO;RST;CLK;Format
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

        // --- CLK signal ID
        int clkFreq = list.at(4).toInt(&ok);
        if (!ok) break;

        // --- data format
        Types::DataFormat format;
        int f = list.at(5).toInt(&ok);
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

    int h = height() / 6;

    QString ioShortTxt;
    QString ioLongTxt;

    if (mSelected) {
        QPen pen = painter.pen();
        pen.setColor(Qt::gray);
        painter.setPen(pen);
        QRectF ioRect(plotX()+4, height()/4-h, 100, 2*h);
        painter.drawText(ioRect, Qt::AlignLeft|Qt::AlignVCenter, "I/O");
    }


    QPen pen = painter.pen();
    pen.setColor(Configuration::instance().analyzerColor());
    painter.setPen(pen);

    for (int i = 0; i < mEmvItems.size(); i++) {
        EmvItem item = mEmvItems.at(i);

        fromIdx = item.startIdx;
        toIdx = item.stopIdx;

        typeAndValueAsString(item.type, item.itemValue, ioShortTxt,
                             ioLongTxt);

        int shortTextWidth = painter.fontMetrics().width(ioShortTxt);
        int longTextWidth = painter.fontMetrics().width(ioLongTxt);


        from = mTimeAxis->timeToPixelRelativeRef((double)fromIdx/sampleRate);

        // no need to draw when signal is out of plot area
        if (from > width()) break;

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


        painter.save();
        painter.translate(0, height()/4);
        paintSignal(&painter, from, to, h, ioShortTxt, ioLongTxt);
        painter.restore();

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

    int x = mIdLbl->pos().x()+mIdLbl->width() + SignalIdMarginRight;
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
                                         int value,
                                         QString &shortTxt,
                                         QString &longTxt)
{


    switch(type) {
    case EmvItem::TYPE_CHARACTER_FRAME:
        shortTxt = formatValue(mFormat, value);
        longTxt = formatValue(mFormat, value);
        break;
    }

}

/*!
    Paint signal data.
*/
void UiEmvAnalyzer::paintSignal(QPainter* painter, double from, double to,
                                int h, QString &shortTxt, QString &longTxt)
{

    int shortTextWidth = painter->fontMetrics().width(shortTxt);
    int longTextWidth = painter->fontMetrics().width(longTxt);

    if (to-from > 4) {
        painter->drawLine(from, 0, from+2, -h);
        painter->drawLine(from, 0, from+2, h);

        painter->drawLine(from+2, -h, to-2, -h);
        painter->drawLine(from+2, h, to-2, h);

        painter->drawLine(to, 0, to-2, -h);
        painter->drawLine(to, 0, to-2, h);
    }

    // drawing a vertical line when the allowed width is too small
    else {
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

}
