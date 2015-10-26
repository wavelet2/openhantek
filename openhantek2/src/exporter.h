////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \brief Declares the Exporter class.
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

#pragma once
#include <QObject>
#include <QSize>
#include <QColor>

namespace DSOAnalyser {
    class DataAnalyzer;
    class OpenHantekSettingsScope;
}

class QPaintDevice;

////////////////////////////////////////////////////////////////////////////////
///
/// \brief Exports the oscilloscope screen to a file or prints it.
class Exporter : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString filename READ getFilename WRITE setFilename)
    Q_PROPERTY(QSize imagesize READ getImagesize WRITE setImagesize)
    Q_PROPERTY(bool zoom READ getZoom WRITE setZoom)

    public:
        Exporter(DSOAnalyser::OpenHantekSettingsScope* scope, DSOAnalyser::DataAnalyzer *dataAnalyzer, QWidget *parent = 0);

        void setFilename(QString filename);
        QString getFilename() const { return m_filename; }
        QSize getImagesize() const { return m_size; }
        void setImagesize(const QSize& size) { m_size = size; }
        bool getZoom() const { return m_zoom; }
        void setZoom(const bool zoom) { m_zoom = zoom; }

        Q_INVOKABLE void print();
        Q_INVOKABLE void exportToImage();
        Q_INVOKABLE void exportToCSV();

        ////////////////////////////////////////////////////////////////////////////////
        /// \struct OpenHantekSettingsColorValues                                    settings.h
        /// \brief Holds the color values for the oscilloscope screen.
        struct OpenHantekSettingsColorValues {
            QColor axes; ///< X- and Y-axis and subdiv lines on them
            QColor background; ///< The scope background
            QColor border; ///< The border of the scope screen
            QColor grid; ///< The color of the grid
            QColor markers; ///< The color of the markers
            QList<QColor> spectrum; ///< The colors of the spectrum graphs
            QColor text; ///< The default text color
            QList<QColor> voltage; ///< The colors of the voltage graphs
        };

        ////////////////////////////////////////////////////////////////////////////////
        /// \struct OpenHantekSettingsViewColor                                      settings.h
        /// \brief Holds the settings for the used colors on the screen and on paper.
        struct OpenHantekSettingsViewColor {
            OpenHantekSettingsColorValues screen; ///< Colors for the screen
            OpenHantekSettingsColorValues print; ///< Colors for printout
        };

    private:
        void draw(QPaintDevice *paintDevice, OpenHantekSettingsColorValues *colorValues, bool forPrint);

        DSOAnalyser::OpenHantekSettingsScope *scope;
        DSOAnalyser::DataAnalyzer *dataAnalyzer;

        QString m_filename;
        QSize m_size = QSize(150,150);
        bool m_zoom = false;
        double DIVS_TIME    = 10.0; ///< Number of horizontal screen divs
        double DIVS_VOLTAGE = 8.0; ///< Number of vertical screen divs
        unsigned DIVS_SUB     = 5; ///< Number of sub-divisions per div

        OpenHantekSettingsViewColor m_colors;
};
