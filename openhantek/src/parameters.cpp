////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  dso.cpp
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

#include <QCoreApplication>
#include "parameters.h"

namespace DsoStrings {
    /// \brief Return string representation of the given channel mode.
    /// \param mode The ::ChannelMode that should be returned as string.
    /// \return The string that should be used in labels etc., empty when invalid.
    QString channelModeString(ChannelMode mode) {
        switch(mode) {
            case CHANNELMODE_VOLTAGE:
                return QCoreApplication::tr("Voltage");
            case CHANNELMODE_SPECTRUM:
                return QCoreApplication::tr("Spectrum");
            default:
                return QString();
        }
    }

    /// \brief Return string representation of the given graph interpolation mode.
    /// \param interpolation The ::InterpolationMode that should be returned as string.
    /// \return The string that should be used in labels etc.
    QString interpolationModeString(InterpolationMode interpolation) {
        switch(interpolation) {
            case INTERPOLATION_OFF:
                return QCoreApplication::tr("Off");
            case INTERPOLATION_LINEAR:
                return QCoreApplication::tr("Linear");
            case INTERPOLATION_SINC:
                return QCoreApplication::tr("Sinc");
            default:
                return QString();
        }
    }
}
