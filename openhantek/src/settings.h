////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \brief Declares the OpenHantekSettings class.
//
//  Copyright (C) 2010, 2011  Oliver Haag
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


#ifndef SETTINGS_H
#define SETTINGS_H


#include <QColor>
#include <QList>
#include <QObject>
#include <QPoint>
#include <QSize>
#include <QString>

#include "dsostrings.h"
#include "parameters.h"
#include "dataAnalyzerSettings.h"

////////////////////////////////////////////////////////////////////////////////
/// \struct OpenHantekSettingsOptionsWindowPanel                             settings.h
/// \brief Holds the position and state of a docking window or toolbar.
struct OpenHantekSettingsOptionsWindowPanel {
    bool floating; ///< true, if the panel is floating
    QPoint position; ///< Position of the panel
    bool visible; ///< true, if the panel is shown
};

////////////////////////////////////////////////////////////////////////////////
/// \struct OpenHantekSettingsOptionsWindowDock                              settings.h
/// \brief Holds the layout of the docking windows.
struct OpenHantekSettingsOptionsWindowDock {
    OpenHantekSettingsOptionsWindowPanel horizontal; ///< "Horizontal" docking window
    OpenHantekSettingsOptionsWindowPanel spectrum; ///< "Spectrum" docking window
    OpenHantekSettingsOptionsWindowPanel trigger; ///< "Trigger" docking window
    OpenHantekSettingsOptionsWindowPanel voltage; ///< "Voltage" docking window
};

////////////////////////////////////////////////////////////////////////////////
/// \struct OpenHantekSettingsOptionsWindowToolbar                           settings.h
/// \brief Holds the layout of the toolbars.
struct OpenHantekSettingsOptionsWindowToolbar {
    OpenHantekSettingsOptionsWindowPanel file; ///< "File" toolbar
    OpenHantekSettingsOptionsWindowPanel oscilloscope; ///< "Oscilloscope" toolbar
    OpenHantekSettingsOptionsWindowPanel view; ///< The "View" toolbar
};

////////////////////////////////////////////////////////////////////////////////
/// \struct OpenHantekSettingsOptionsWindow                                  settings.h
/// \brief Holds the layout of the main window.
struct OpenHantekSettingsOptionsWindow {
    QPoint position; ///< Position of the main window
    QSize size; ///< Size of the main window
    OpenHantekSettingsOptionsWindowDock dock; ///< Docking windows
    OpenHantekSettingsOptionsWindowToolbar toolbar; ///< Toolbars
};

////////////////////////////////////////////////////////////////////////////////
/// \struct OpenHantekSettingsOptions                                        settings.h
/// \brief Holds the general options of the program.
struct OpenHantekSettingsOptions {
    bool alwaysSave; ///< Always save the settings on exit
    QSize imageSize; ///< Size of exported images in pixels
    OpenHantekSettingsOptionsWindow window; ///< Window layout
};

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

////////////////////////////////////////////////////////////////////////////////
/// \struct OpenHantekSettingsView                                           settings.h
/// \brief Holds all view settings.
struct OpenHantekSettingsView {
    OpenHantekSettingsViewColor color; ///< Used colors
    bool antialiasing; ///< Antialiasing for the graphs
    bool digitalPhosphor; ///< true slowly fades out the previous graphs
    int digitalPhosphorDepth; ///< Number of channels shown at one time
    InterpolationMode interpolation; ///< Interpolation mode for the graph
    bool screenColorImages; ///< true exports images with screen colors
    bool zoom; ///< true if the magnified scope is enabled
};

////////////////////////////////////////////////////////////////////////////////
///
/// \brief Holds the settings of the program.
class OpenHantekSettings : public QObject {
    Q_OBJECT

    public:
        OpenHantekSettings(QWidget *parent = 0);
        ~OpenHantekSettings();

        void setChannelCount(unsigned int channels);

        OpenHantekSettingsOptions options; ///< General options of the program
        DSOAnalyser::OpenHantekSettingsScope scope; ///< All oscilloscope related settings
        OpenHantekSettingsView view; ///< All view related settings

    public slots:
        // Saving to and loading from configuration files
        int load(const QString &fileName = QString());
        int save(const QString &fileName = QString());
};


#endif
