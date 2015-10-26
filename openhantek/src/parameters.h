#pragma once

//////////////////////////////////////////////////////////////////////////////
/// \enum ChannelMode                                                    dso.h
/// \brief The channel display modes.
enum ChannelMode {
    CHANNELMODE_VOLTAGE,                ///< Standard voltage view
    CHANNELMODE_SPECTRUM,               ///< Spectrum view
    CHANNELMODE_COUNT                   ///< The total number of modes
};


////////////////////////////////////////////////////////////////////////////////
/// \enum InterpolationMode                                                dso.h
/// \brief The different interpolation modes for the graphs.
enum InterpolationMode {
    INTERPOLATION_OFF = 0,              ///< Just dots for each sample
    INTERPOLATION_LINEAR,               ///< Sample dots connected by lines
    INTERPOLATION_SINC,                 ///< Smooth graph through the dots
    INTERPOLATION_COUNT                 ///< Total number of interpolation modes
};

#include <QString>
namespace DsoStrings {
    QString channelModeString(ChannelMode mode);
    QString interpolationModeString(InterpolationMode interpolation);
}
