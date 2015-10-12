////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file dso.h
/// \brief Defines various constants, enums and functions for DSO settings.
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


#ifndef DSO_H
#define DSO_H


#include <QString>
#include "dsocontrol.h"

#define MARKER_COUNT                  2 ///< Number of markers


////////////////////////////////////////////////////////////////////////////////
/// \namespace Dso                                                         dso.h
/// \brief All DSO specific things for different modes and so on.
namespace DsoStrings {
    using namespace Dso;
	QString channelModeString(ChannelMode mode);
	QString graphFormatString(GraphFormat format);
	QString couplingString(Coupling coupling);
	QString mathModeString(MathMode mode);
	QString triggerModeString(TriggerMode mode);
	QString slopeString(Slope slope);
	QString windowFunctionString(WindowFunction window);
	QString interpolationModeString(InterpolationMode interpolation);
}


#endif
