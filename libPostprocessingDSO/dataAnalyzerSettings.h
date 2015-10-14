#pragma once

#include "dsoSettings.h"

namespace DSOAnalyser {

#define MARKER_COUNT                  2 ///< Number of markers

//////////////////////////////////////////////////////////////////////////////
/// \enum WindowFunction                                                 dso.h
/// \brief The supported window functions.
/// These are needed for spectrum analysis and are applied to the sample values
/// before calculating the DFT.
enum WindowFunction {
    WINDOW_UNDEFINED = -1,
    WINDOW_RECTANGULAR = 0,                 ///< Rectangular window (aka Dirichlet)
    WINDOW_HAMMING,                     ///< Hamming window
    WINDOW_HANN,                        ///< Hann window
    WINDOW_COSINE,                      ///< Cosine window (aka Sine)
    WINDOW_LANCZOS,                     ///< Lanczos window (aka Sinc)
    WINDOW_BARTLETT,                    ///< Bartlett window (Endpoints == 0)
    WINDOW_TRIANGULAR,                  ///< Triangular window (Endpoints != 0)
    WINDOW_GAUSS,                       ///< Gauss window (simga = 0.4)
    WINDOW_BARTLETTHANN,                ///< Bartlett-Hann window
    WINDOW_BLACKMAN,                    ///< Blackman window (alpha = 0.16)
    //WINDOW_KAISER,                      ///< Kaiser window (alpha = 3.0)
    WINDOW_NUTTALL,                     ///< Nuttall window, cont. first deriv.
    WINDOW_BLACKMANHARRIS,              ///< Blackman-Harris window
    WINDOW_BLACKMANNUTTALL,             ///< Blackman-Nuttall window
    WINDOW_FLATTOP,                     ///< Flat top window
    WINDOW_COUNT                        ///< Total number of window functions
};

//////////////////////////////////////////////////////////////////////////////
/// \enum GraphFormat                                                    dso.h
/// \brief The possible viewing formats for the graphs on the scope.
enum GraphFormat {
    GRAPHFORMAT_TY,                     ///< The standard mode
    GRAPHFORMAT_XY,                     ///< CH1 on X-axis, CH2 on Y-axis
    GRAPHFORMAT_COUNT                   ///< The total number of formats
};

//////////////////////////////////////////////////////////////////////////////
/// \enum MathMode                                                       dso.h
/// \brief The different math modes for the math-channel.
enum MathMode {
    MATHMODE_1ADD2,                     ///< Add the values of the channels
    MATHMODE_1SUB2,                     ///< Subtract CH2 from CH1
    MATHMODE_2SUB1,                     ///< Subtract CH1 from CH2
    MATHMODE_COUNT                      ///< The total number of math modes
};

////////////////////////////////////////////////////////////////////////////////
/// \struct OpenHantekSettingsScopeHorizontal                                settings.h
/// \brief Holds the settings for the horizontal axis.
struct OpenHantekSettingsScopeHorizontal {
    GraphFormat format; ///< Graph drawing mode of the scope
    double frequencybase; ///< Frequencybase in Hz/div
    double marker[MARKER_COUNT]; ///< Marker positions in div
    bool marker_visible[MARKER_COUNT];
    double timebase; ///< Timebase in s/div
    unsigned int recordLength; ///< Sample count
    double samplerate; ///< The samplerate of the oscilloscope in S
    bool samplerateSet; ///< The samplerate was set by the user, not the timebase
};

////////////////////////////////////////////////////////////////////////////////
/// \struct OpenHantekSettingsScopeTrigger                                   settings.h
/// \brief Holds the settings for the trigger.
struct OpenHantekSettingsScopeTrigger {
    bool filter; ///< Not sure what this is good for...
    DSO::TriggerMode mode; ///< Automatic, normal or single trigger
    double position; ///< Horizontal position for pretrigger
    DSO::Slope slope; ///< Rising or falling edge causes trigger
    unsigned int source; ///< Channel that is used as trigger source
    bool special; ///< true if the trigger source is not a standard channel
};

////////////////////////////////////////////////////////////////////////////////
/// \struct OpenHantekSettingsScopeSpectrum                                  settings.h
/// \brief Holds the settings for the spectrum analysis.
struct OpenHantekSettingsScopeSpectrum {
    double magnitude; ///< The vertical resolution in dB/div
    std::string name; ///< Name of this channel
    double offset; ///< Vertical offset in divs
    bool used; ///< true if the spectrum is turned on
};

////////////////////////////////////////////////////////////////////////////////
/// \struct OpenHantekSettingsScopeVoltage                                   settings.h
/// \brief Holds the settings for the normal voltage graphs.
struct OpenHantekSettingsScopeVoltage {
    double gain; ///< The vertical resolution in V/div
    int misc; ///< Different enums, coupling for real- and mode for math-channels
    std::string name; ///< Name of this channel
    double offset; ///< Vertical offset in divs
    double trigger; ///< Trigger level in V
    bool used; ///< true if this channel is enabled
};

////////////////////////////////////////////////////////////////////////////////
/// \struct OpenHantekSettingsScope                                          settings.h
/// \brief Holds the settings for the oscilloscope.
struct OpenHantekSettingsScope {
    OpenHantekSettingsScopeHorizontal horizontal; ///< Settings for the horizontal axis
    OpenHantekSettingsScopeTrigger trigger; ///< Settings for the trigger
    std::vector<OpenHantekSettingsScopeSpectrum> spectrum; ///< Spectrum analysis settings
    std::vector<OpenHantekSettingsScopeVoltage> voltage; ///< Settings for the normal graphs

    unsigned int physicalChannels; ///< Number of real channels (No math etc.)
    WindowFunction spectrumWindow; ///< Window function for DFT
    double spectrumReference; ///< Reference level for spectrum in dBm
    double spectrumLimit; ///< Minimum magnitude of the spectrum (Avoids peaks)
};

}