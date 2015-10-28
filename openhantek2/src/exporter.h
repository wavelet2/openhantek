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

#include "dataAnalyzer.h"
#include "scopecolors.h"

class QPaintDevice;

////////////////////////////////////////////////////////////////////////////////
///
/// \brief Exports the oscilloscope screen to a file or prints it.
class Exporter : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString filename READ getFilename WRITE setFilename)
    Q_PROPERTY(QSize imagesize READ getImagesize WRITE setImagesize)
    Q_PROPERTY(bool zoom READ getZoom WRITE setZoom)
    Q_PROPERTY(ScopeColors* screenColors READ getScreenColors CONSTANT)
    Q_PROPERTY(ScopeColors* printColors READ getPrintColors CONSTANT)

    public:
        Exporter(DSOAnalyser::OpenHantekSettingsScope* scope, QWidget *parent = 0);

        void createDataCopy(DSOAnalyser::DataAnalyzer *dataAnalyzer);
        void setFilename(QString filename);
        QString getFilename() const { return m_filename; }
        QSize getImagesize() const { return m_size; }
        void setImagesize(const QSize& size) { m_size = size; }
        bool getZoom() const { return m_zoom; }
        void setZoom(const bool zoom) { m_zoom = zoom; }
        ScopeColors* getScreenColors() { return &m_screen; }
        ScopeColors* getPrintColors() { return &m_print; }

        Q_INVOKABLE void print();
        Q_INVOKABLE void exportToImage();
        Q_INVOKABLE void exportToCSV();
    private:
        void draw(QPaintDevice *paintDevice, const ScopeColors& colorValues, bool forPrint);

        DSOAnalyser::OpenHantekSettingsScope *scope;
        std::vector<DSOAnalyser::AnalyzedData> m_analyzedData;

        QString m_filename;
        QSize m_size = QSize(150,150);
        bool m_zoom = false;
        double DIVS_TIME    = 10.0; ///< Number of horizontal screen divs
        double DIVS_VOLTAGE = 8.0; ///< Number of vertical screen divs
        unsigned DIVS_SUB     = 5; ///< Number of sub-divisions per div

        ScopeColors m_screen; ///< Colors for the screen
        ScopeColors m_print; ///< Colors for printout
};
