////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  exporter.cpp
//
//  Copyright (C) 2010  Oliver Haag
//  oliver.haag@gmail.com
//
//  This program is free software: you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along with
//  this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////


#include <cmath>

#include <QFile>
#include <QImage>
#include <QMutex>
#include <QPainter>
#include <QPixmap>
#include <QPrintDialog>
#include <QPrinter>
#include <QTextStream>


#include "exporter.h"
#include "dataAnalyzer.h"
#include "dataAnalyzerSettings.h"
#include "dsostrings.h"
#include "unitToString.h"

////////////////////////////////////////////////////////////////////////////////
// class HorizontalDock
/// \brief Initializes the printer object.
Exporter::Exporter(DSOAnalyser::OpenHantekSettingsScope* scope, DSOAnalyser::DataAnalyzer *dataAnalyzer, QWidget *parent)
    : QObject(parent), scope(scope), dataAnalyzer(dataAnalyzer) {
}

/// \brief Set the filename of the output file (Not used for printing).
void Exporter::setFilename(QString filename) {
    if(!filename.isEmpty())
        m_filename = filename;
}

void Exporter::print() {
    // We need a QPrinter for printing, pdf- and ps-export
    QPrinter* paintDevice = new QPrinter(QPrinter::HighResolution);
    paintDevice->setOrientation(m_zoom ? QPrinter::Portrait : QPrinter::Landscape);
    paintDevice->setPageMargins(20, 20, 20, 20, QPrinter::Millimeter);

    // Show the printing dialog
    QPrintDialog dialog(paintDevice, static_cast<QWidget *>(this->parent()));
    dialog.setWindowTitle(tr("Print oscillograph"));
    if(dialog.exec() != QDialog::Accepted) {
        delete paintDevice;
        return;
    }

    paintDevice->setOutputFileName(m_filename);
    this->dataAnalyzer->mutex().lock();
    draw(paintDevice, &(m_colors.print), true);
    this->dataAnalyzer->mutex().unlock();
    delete paintDevice;
}

void Exporter::exportToImage() {
    QPixmap* paintDevice = new QPixmap(m_size);
    paintDevice->fill(m_colors.screen.background);
    this->dataAnalyzer->mutex().lock();
    draw(paintDevice, &(m_colors.screen), false);
    this->dataAnalyzer->mutex().unlock();
    paintDevice->save(m_filename);
    delete paintDevice;
}

/// \brief Print the document (May be a file too)
void Exporter::draw(QPaintDevice *paintDevice, OpenHantekSettingsColorValues *colorValues, bool forPrint) {

    // Create a painter for our device
    QPainter painter(paintDevice);

    // Get line height
    QFont font;
    QFontMetrics fontMetrics(font, paintDevice);
    double lineHeight = fontMetrics.height();

    painter.setBrush(Qt::SolidPattern);

    // Draw the settings table
    double stretchBase = (double) (paintDevice->width() - lineHeight * 10) / 4;

    // Print trigger details
    painter.setPen(colorValues->voltage[scope->trigger.source]);
    QString levelString = UnitToString::valueToString(scope->voltage[scope->trigger.source].trigger, UnitToString::UNIT_VOLTS, 3);
    QString pretriggerString = tr("%L1%").arg((int) (scope->trigger.position * 100 + 0.5));
    painter.drawText(QRectF(0, 0, lineHeight * 10, lineHeight),
                        tr("%1  %2  %3  %4").arg(
                        QString::fromStdString(scope->voltage[scope->trigger.source].name),
                        DsoStrings::slopeString(scope->trigger.slope),
                        levelString,
                        pretriggerString));

    // Print sample count
    painter.setPen(colorValues->text);
    painter.drawText(QRectF(lineHeight * 10, 0, stretchBase, lineHeight), tr("%1 S").arg(this->dataAnalyzer->sampleCount()), QTextOption(Qt::AlignRight));
    // Print samplerate
    painter.drawText(QRectF(lineHeight * 10 + stretchBase, 0, stretchBase, lineHeight), UnitToString::valueToString(scope->horizontal.samplerate, UnitToString::UNIT_SAMPLES) + tr("/s"), QTextOption(Qt::AlignRight));
    // Print timebase
    painter.drawText(QRectF(lineHeight * 10 + stretchBase * 2, 0, stretchBase, lineHeight), UnitToString::valueToString(scope->horizontal.timebase, UnitToString::UNIT_SECONDS, 0) + tr("/div"), QTextOption(Qt::AlignRight));
    // Print frequencybase
    painter.drawText(QRectF(lineHeight * 10 + stretchBase * 3, 0, stretchBase, lineHeight), UnitToString::valueToString(scope->horizontal.frequencybase, UnitToString::UNIT_HERTZ, 0) + tr("/div"), QTextOption(Qt::AlignRight));

    // Draw the measurement table
    stretchBase = (double) (paintDevice->width() - lineHeight * 6) / 10;
    int channelCount = 0;
    for(int channel = scope->voltage.size() - 1; channel >= 0; channel--) {
        if((scope->voltage[channel].used || scope->spectrum[channel].used) && this->dataAnalyzer->data(channel)) {
            ++channelCount;
            double top = (double) paintDevice->height() - channelCount * lineHeight;

            // Print label
            painter.setPen(colorValues->voltage[channel]);
            painter.drawText(QRectF(0, top, lineHeight * 4, lineHeight),
                                QString::fromStdString(scope->voltage[channel].name));
            // Print coupling/math mode
            if((unsigned int) channel < scope->physicalChannels)
                painter.drawText(QRectF(lineHeight * 4, top, lineHeight * 2, lineHeight),
                                    DsoStrings::couplingString((DSO::Coupling) scope->voltage[channel].misc));
            else
                painter.drawText(QRectF(lineHeight * 4, top, lineHeight * 2, lineHeight),
                                    DsoStrings::mathModeString((DSOAnalyser::MathMode) scope->voltage[channel].misc));

            // Print voltage gain
            painter.drawText(QRectF(lineHeight * 6, top, stretchBase * 2, lineHeight), UnitToString::valueToString(scope->voltage[channel].gain, UnitToString::UNIT_VOLTS, 0) + tr("/div"), QTextOption(Qt::AlignRight));
            // Print spectrum magnitude
            painter.setPen(colorValues->spectrum[channel]);
            painter.drawText(QRectF(lineHeight * 6 + stretchBase * 2, top, stretchBase * 2, lineHeight), UnitToString::valueToString(scope->spectrum[channel].magnitude, UnitToString::UNIT_DECIBEL, 0) + tr("/div"), QTextOption(Qt::AlignRight));

            // Amplitude string representation (4 significant digits)
            painter.setPen(colorValues->text);
            painter.drawText(QRectF(lineHeight * 6 + stretchBase * 4, top, stretchBase * 3, lineHeight), UnitToString::valueToString(this->dataAnalyzer->data(channel)->amplitude, UnitToString::UNIT_VOLTS, 4), QTextOption(Qt::AlignRight));
            // Frequency string representation (5 significant digits)
            painter.drawText(QRectF(lineHeight * 6 + stretchBase * 7, top, stretchBase * 3, lineHeight), UnitToString::valueToString(this->dataAnalyzer->data(channel)->frequency, UnitToString::UNIT_HERTZ, 5), QTextOption(Qt::AlignRight));
        }
    }

    // Draw the marker table
    double scopeHeight;
    stretchBase = (double) (paintDevice->width() - lineHeight * 10) / 4;
    painter.setPen(colorValues->text);

    // Calculate variables needed for zoomed scope
    double divs = fabs(scope->horizontal.marker[1] - scope->horizontal.marker[0]);
    double time = divs * scope->horizontal.timebase;
    double zoomFactor = DIVS_TIME / divs;
    double zoomOffset = (scope->horizontal.marker[0] + scope->horizontal.marker[1]) / 2;

    if(m_zoom) {
        scopeHeight = (double) (paintDevice->height() - (channelCount + 5) * lineHeight) / 2;
        double top = 2.5 * lineHeight + scopeHeight;

        painter.drawText(QRectF(0, top, stretchBase, lineHeight), tr("Zoom x%L1").arg(DIVS_TIME / divs, -1, 'g', 3));

        painter.drawText(QRectF(lineHeight * 10, top, stretchBase, lineHeight), UnitToString::valueToString(time, UnitToString::UNIT_SECONDS, 4), QTextOption(Qt::AlignRight));
        painter.drawText(QRectF(lineHeight * 10 + stretchBase, top, stretchBase, lineHeight), UnitToString::valueToString(1.0 / time, UnitToString::UNIT_HERTZ, 4), QTextOption(Qt::AlignRight));

        painter.drawText(QRectF(lineHeight * 10 + stretchBase * 2, top, stretchBase, lineHeight), UnitToString::valueToString(time / DIVS_TIME, UnitToString::UNIT_SECONDS, 3) + tr("/div"), QTextOption(Qt::AlignRight));
        painter.drawText(QRectF(lineHeight * 10 + stretchBase * 3, top, stretchBase, lineHeight), UnitToString::valueToString(divs  * scope->horizontal.frequencybase / DIVS_TIME, UnitToString::UNIT_HERTZ, 3) + tr("/div"), QTextOption(Qt::AlignRight));
    }
    else {
        scopeHeight = (double) paintDevice->height() - (channelCount + 4) * lineHeight;
        double top = 2.5 * lineHeight + scopeHeight;

        painter.drawText(QRectF(0, top, stretchBase, lineHeight), tr("Marker 1/2"));

        painter.drawText(QRectF(lineHeight * 10, top, stretchBase * 2, lineHeight), UnitToString::valueToString(time, UnitToString::UNIT_SECONDS, 4), QTextOption(Qt::AlignRight));
        painter.drawText(QRectF(lineHeight * 10 + stretchBase * 2, top, stretchBase * 2, lineHeight), UnitToString::valueToString(1.0 / time, UnitToString::UNIT_HERTZ, 4), QTextOption(Qt::AlignRight));
    }

    // Set DIVS_TIME x DIVS_VOLTAGE matrix for oscillograph
    painter.setMatrix(QMatrix((paintDevice->width() - 1) / DIVS_TIME, 0, 0, -(scopeHeight - 1) / DIVS_VOLTAGE, (double) (paintDevice->width() - 1) / 2, (scopeHeight - 1) / 2 + lineHeight * 1.5), false);

    // Draw the graphs
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(Qt::NoBrush);

    for(unsigned zoomed = 0; zoomed < (m_zoom ? 2 : 1); ++zoomed) {
        switch(scope->horizontal.format) {
            case DSOAnalyser::GRAPHFORMAT_TY:
                // Add graphs for channels
                for(unsigned channel = 0 ; channel < scope->voltage.size(); ++channel) {
                    if(scope->voltage[channel].used && this->dataAnalyzer->data(channel)) {
                        painter.setPen(colorValues->voltage[channel]);

                        // What's the horizontal distance between sampling points?
                        double horizontalFactor = this->dataAnalyzer->data(channel)->samples.voltage.interval / scope->horizontal.timebase;
                        // How many samples are visible?
                        double centerPosition, centerOffset;
                        if(zoomed) {
                            centerPosition = (zoomOffset + DIVS_TIME / 2) / horizontalFactor;
                            centerOffset = DIVS_TIME / horizontalFactor / zoomFactor / 2;
                        }
                        else {
                            centerPosition = DIVS_TIME / 2 / horizontalFactor;
                            centerOffset = DIVS_TIME / horizontalFactor / 2;
                        }
                        unsigned int firstPosition = qMax((int) (centerPosition - centerOffset), 0);
                        unsigned int lastPosition = qMin((int) (centerPosition + centerOffset), (int) this->dataAnalyzer->data(channel)->samples.voltage.sample.size() - 1);

                        // Draw graph
                        QPointF *graph = new QPointF[lastPosition - firstPosition + 1];

                        for(unsigned int position = firstPosition; position <= lastPosition; ++position)
                            graph[position - firstPosition] = QPointF(position * horizontalFactor - DIVS_TIME / 2, this->dataAnalyzer->data(channel)->samples.voltage.sample[position] / scope->voltage[channel].gain + scope->voltage[channel].offset);

                        painter.drawPolyline(graph, lastPosition - firstPosition + 1);
                        delete[] graph;
                    }
                }

                // Add spectrum graphs
                for (unsigned channel = 0; channel < scope->spectrum.size(); ++channel) {
                    if(scope->spectrum[channel].used && this->dataAnalyzer->data(channel)) {
                        painter.setPen(colorValues->spectrum[channel]);

                        // What's the horizontal distance between sampling points?
                        double horizontalFactor = this->dataAnalyzer->data(channel)->samples.spectrum.interval / scope->horizontal.frequencybase;
                        // How many samples are visible?
                        double centerPosition, centerOffset;
                        if(zoomed) {
                            centerPosition = (zoomOffset + DIVS_TIME / 2) / horizontalFactor;
                            centerOffset = DIVS_TIME / horizontalFactor / zoomFactor / 2;
                        }
                        else {
                            centerPosition = DIVS_TIME / 2 / horizontalFactor;
                            centerOffset = DIVS_TIME / horizontalFactor / 2;
                        }
                        unsigned int firstPosition = qMax((int) (centerPosition - centerOffset), 0);
                        unsigned int lastPosition = qMin((int) (centerPosition + centerOffset), (int) this->dataAnalyzer->data(channel)->samples.spectrum.sample.size() - 1);

                        // Draw graph
                        QPointF *graph = new QPointF[lastPosition - firstPosition + 1];

                        for(unsigned int position = firstPosition; position <= lastPosition; ++position)
                            graph[position - firstPosition] = QPointF(position * horizontalFactor - DIVS_TIME / 2, this->dataAnalyzer->data(channel)->samples.spectrum.sample[position] / scope->spectrum[channel].magnitude + scope->spectrum[channel].offset);

                        painter.drawPolyline(graph, lastPosition - firstPosition + 1);
                        delete[] graph;
                    }
                }
                break;

            case DSOAnalyser::GRAPHFORMAT_XY:
                break;

            default:
                break;
        }

        // Set DIVS_TIME / zoomFactor x DIVS_VOLTAGE matrix for zoomed oscillograph
        painter.setMatrix(QMatrix((paintDevice->width() - 1) / DIVS_TIME * zoomFactor, 0, 0, -(scopeHeight - 1) / DIVS_VOLTAGE, (double) (paintDevice->width() - 1) / 2 - zoomOffset * zoomFactor * (paintDevice->width() - 1) / DIVS_TIME, (scopeHeight - 1) * 1.5 + lineHeight * 4), false);
    }

    // Draw grids
    painter.setRenderHint(QPainter::Antialiasing, false);
    for(int zoomed = 0; zoomed < (m_zoom ? 2 : 1); ++zoomed) {
        // Set DIVS_TIME x DIVS_VOLTAGE matrix for oscillograph
        painter.setMatrix(QMatrix((paintDevice->width() - 1) / DIVS_TIME, 0, 0, -(scopeHeight - 1) / DIVS_VOLTAGE, (double) (paintDevice->width() - 1) / 2, (scopeHeight - 1) * (zoomed + 0.5) + lineHeight * 1.5 + lineHeight * 2.5 * zoomed), false);

        // Grid lines
        painter.setPen(colorValues->grid);

        if(forPrint) {
            // Draw vertical lines
            for(int div = 1; div < DIVS_TIME / 2; ++div) {
                for(int dot = 1; dot < DIVS_VOLTAGE / 2 * 5; ++dot) {
                    painter.drawLine(QPointF((double) -div - 0.02, (double) -dot / 5), QPointF((double) -div + 0.02, (double) -dot / 5));
                    painter.drawLine(QPointF((double) -div - 0.02, (double) dot / 5), QPointF((double) -div + 0.02, (double) dot / 5));
                    painter.drawLine(QPointF((double) div - 0.02, (double) -dot / 5), QPointF((double) div + 0.02, (double) -dot / 5));
                    painter.drawLine(QPointF((double) div - 0.02, (double) dot / 5), QPointF((double) div + 0.02, (double) dot / 5));
                }
            }
            // Draw horizontal lines
            for(int div = 1; div < DIVS_VOLTAGE / 2; ++div) {
                for(int dot = 1; dot < DIVS_TIME / 2 * 5; ++dot) {
                    painter.drawLine(QPointF((double) -dot / 5, (double) -div - 0.02), QPointF((double) -dot / 5, (double) -div + 0.02));
                    painter.drawLine(QPointF((double) dot / 5, (double) -div - 0.02), QPointF((double) dot / 5, (double) -div + 0.02));
                    painter.drawLine(QPointF((double) -dot / 5, (double) div - 0.02), QPointF((double) -dot / 5, (double) div + 0.02));
                    painter.drawLine(QPointF((double) dot / 5, (double) div - 0.02), QPointF((double) dot / 5, (double) div + 0.02));
                }
            }
        }
        else {
            // Draw vertical lines
            for(int div = 1; div < DIVS_TIME / 2; ++div) {
                for(int dot = 1; dot < DIVS_VOLTAGE / 2 * 5; ++dot) {
                    painter.drawPoint(QPointF(-div, (double) -dot / 5));
                    painter.drawPoint(QPointF(-div, (double) dot / 5));
                    painter.drawPoint(QPointF(div, (double) -dot / 5));
                    painter.drawPoint(QPointF(div, (double) dot / 5));
                }
            }
            // Draw horizontal lines
            for(int div = 1; div < DIVS_VOLTAGE / 2; ++div) {
                for(int dot = 1; dot < DIVS_TIME / 2 * 5; ++dot) {
                    if(dot % 5 == 0)
                        continue;                       // Already done by vertical lines
                    painter.drawPoint(QPointF((double) -dot / 5, -div));
                    painter.drawPoint(QPointF((double) dot / 5, -div));
                    painter.drawPoint(QPointF((double) -dot / 5, div));
                    painter.drawPoint(QPointF((double) dot / 5, div));
                }
            }
        }

        // Axes
        painter.setPen(colorValues->axes);
        painter.drawLine(QPointF(-DIVS_TIME / 2, 0), QPointF(DIVS_TIME / 2, 0));
        painter.drawLine(QPointF(0, -DIVS_VOLTAGE / 2), QPointF(0, DIVS_VOLTAGE / 2));
        for(double div = 0.2; div <= DIVS_TIME / 2; div += 0.2) {
            painter.drawLine(QPointF(div, -0.05), QPointF(div, 0.05));
            painter.drawLine(QPointF(-div, -0.05), QPointF(-div, 0.05));
        }
        for(double div = 0.2; div <= DIVS_VOLTAGE / 2; div += 0.2) {
            painter.drawLine(QPointF(-0.05, div), QPointF(0.05, div));
            painter.drawLine(QPointF(-0.05, -div), QPointF(0.05, -div));
        }

        // Borders
        painter.setPen(colorValues->border);
        painter.drawRect(QRectF(-DIVS_TIME / 2, -DIVS_VOLTAGE / 2, DIVS_TIME, DIVS_VOLTAGE));
    }

    painter.end();
}

void Exporter::exportToCSV() {
    QFile csvFile(m_filename);
    if(!csvFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream csvStream(&csvFile);

    for(unsigned channel = 0 ; channel < scope->voltage.size(); ++channel) {
        if(this->dataAnalyzer->data(channel)) {
            if(scope->voltage[channel].used) {
                // Start with channel name and the sample interval
                csvStream << "\"" << QString::fromStdString(scope->voltage[channel].name) << "\"," << this->dataAnalyzer->data(channel)->samples.voltage.interval;

                // And now all sample values in volts
                for(unsigned int position = 0; position < this->dataAnalyzer->data(channel)->samples.voltage.sample.size(); ++position)
                    csvStream << "," << this->dataAnalyzer->data(channel)->samples.voltage.sample[position];

                // Finally a newline
                csvStream << '\n';
            }

            if(scope->spectrum[channel].used) {
                // Start with channel name and the sample interval
                csvStream << "\"" << QString::fromStdString(scope->spectrum[channel].name) << "\"," << this->dataAnalyzer->data(channel)->samples.spectrum.interval;

                // And now all magnitudes in dB
                for(unsigned int position = 0; position < this->dataAnalyzer->data(channel)->samples.spectrum.sample.size(); ++position)
                    csvStream << "," << this->dataAnalyzer->data(channel)->samples.spectrum.sample[position];

                // Finally a newline
                csvStream << '\n';
            }
        }
    }
    csvFile.close();
}
