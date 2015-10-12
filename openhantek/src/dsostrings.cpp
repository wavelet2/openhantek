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

#include "dsostrings.h"

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
	
	/// \brief Return string representation of the given graph format.
	/// \param format The ::GraphFormat that should be returned as string.
	/// \return The string that should be used in labels etc.
	QString graphFormatString(GraphFormat format) {
		switch(format) {
			case GRAPHFORMAT_TY:
				return QCoreApplication::tr("T - Y");
			case GRAPHFORMAT_XY:
				return QCoreApplication::tr("X - Y");
			default:
				return QString();
		}
	}
	
	/// \brief Return string representation of the given channel coupling.
	/// \param coupling The ::Coupling that should be returned as string.
	/// \return The string that should be used in labels etc.
	QString couplingString(Coupling coupling) {
		switch(coupling) {
			case COUPLING_AC:
				return QCoreApplication::tr("AC");
			case COUPLING_DC:
				return QCoreApplication::tr("DC");
			case COUPLING_GND:
				return QCoreApplication::tr("GND");
			default:
				return QString();
		}
	}
	
	/// \brief Return string representation of the given math mode.
	/// \param mode The ::MathMode that should be returned as string.
	/// \return The string that should be used in labels etc.
	QString mathModeString(MathMode mode) {
		switch(mode) {
			case MATHMODE_1ADD2:
				return QCoreApplication::tr("CH1 + CH2");
			case MATHMODE_1SUB2:
				return QCoreApplication::tr("CH1 - CH2");
			case MATHMODE_2SUB1:
				return QCoreApplication::tr("CH2 - CH1");
			default:
				return QString();
		}
	}
	
	/// \brief Return string representation of the given trigger mode.
	/// \param mode The ::TriggerMode that should be returned as string.
	/// \return The string that should be used in labels etc.
	QString triggerModeString(TriggerMode mode) {
		switch(mode) {
			case TRIGGERMODE_AUTO:
				return QCoreApplication::tr("Auto");
			case TRIGGERMODE_NORMAL:
				return QCoreApplication::tr("Normal");
			case TRIGGERMODE_SINGLE:
				return QCoreApplication::tr("Single");
			default:
				return QString();
		}
	}
	
	/// \brief Return string representation of the given trigger slope.
	/// \param slope The ::Slope that should be returned as string.
	/// \return The string that should be used in labels etc.
	QString slopeString(Slope slope) {
		switch(slope) {
			case SLOPE_POSITIVE:
				return QString::fromUtf8("\u2197");
			case SLOPE_NEGATIVE:
				return QString::fromUtf8("\u2198");
			default:
				return QString();
		}
	}
	
	/// \brief Return string representation of the given dft window function.
	/// \param window The ::WindowFunction that should be returned as string.
	/// \return The string that should be used in labels etc.
	QString windowFunctionString(WindowFunction window) {
		switch(window) {
			case WINDOW_RECTANGULAR:
				return QCoreApplication::tr("Rectangular");
			case WINDOW_HAMMING:
				return QCoreApplication::tr("Hamming");
			case WINDOW_HANN:
				return QCoreApplication::tr("Hann");
			case WINDOW_COSINE:
				return QCoreApplication::tr("Cosine");
			case WINDOW_LANCZOS:
				return QCoreApplication::tr("Lanczos");
			case WINDOW_BARTLETT:
				return QCoreApplication::tr("Bartlett");
			case WINDOW_TRIANGULAR:
				return QCoreApplication::tr("Triangular");
			case WINDOW_GAUSS:
				return QCoreApplication::tr("Gauss");
			case WINDOW_BARTLETTHANN:
				return QCoreApplication::tr("Bartlett-Hann");
			case WINDOW_BLACKMAN:
				return QCoreApplication::tr("Blackman");
			//case WINDOW_KAISER:
			//	return QCoreApplication::tr("Kaiser");
			case WINDOW_NUTTALL:
				return QCoreApplication::tr("Nuttall");
			case WINDOW_BLACKMANHARRIS:
				return QCoreApplication::tr("Blackman-Harris");
			case WINDOW_BLACKMANNUTTALL:
				return QCoreApplication::tr("Blackman-Nuttall");
			case WINDOW_FLATTOP:
				return QCoreApplication::tr("Flat top");
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
