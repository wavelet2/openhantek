#pragma once

#define HANTEK_CHANNELS               2 ///< Number of physical channels
#define HANTEK_SPECIAL_CHANNELS       2 ///< Number of special channels

namespace DSO {
        class ControlSamplerateLimits;

        //////////////////////////////////////////////////////////////////////////////
    /// \enum Slope                                                          dso.h
    /// \brief The slope that causes a trigger.
    enum class Slope {
        SLOPE_POSITIVE,                     ///< From lower to higher voltage
        SLOPE_NEGATIVE,                     ///< From higher to lower voltage
        SLOPE_COUNT                         ///< Total number of trigger slopes
    };

    //////////////////////////////////////////////////////////////////////////////
    /// \enum TriggerMode                                                    dso.h
    /// \brief The different triggering modes.
    enum class TriggerMode {
        TRIGGERMODE_UNDEFINED = -1,
        TRIGGERMODE_AUTO,                   ///< Automatic without trigger event
        TRIGGERMODE_NORMAL,                 ///< Normal mode
        TRIGGERMODE_SINGLE,                 ///< Stop after the first trigger event
        TRIGGERMODE_COUNT                   ///< The total number of modes
    };

    //////////////////////////////////////////////////////////////////////////////
    /// \enum Coupling                                                       dso.h
    /// \brief The coupling modes for the channels.
    enum class Coupling {
        COUPLING_AC,                        ///< Offset filtered out by condensator
        COUPLING_DC,                        ///< No filtering
        COUPLING_GND,                       ///< Channel is grounded
        COUPLING_COUNT                      ///< The total number of coupling modes
    };

    //////////////////////////////////////////////////////////////////////////////
    /// \struct dsoSettingsSamplerateTarget
    /// \brief Stores the target samplerate settings of the device.
    struct dsoSettingsSamplerateTarget {
        double samplerate; ///< The target samplerate set via setSamplerate
        double duration; ///< The target record time set via setRecordTime
        bool samplerateSet; ///< true means samplerate was set last, false duration
    };

    //////////////////////////////////////////////////////////////////////////////
    /// \struct dsoSettingsSamplerate
    /// \brief Stores the current samplerate settings of the device.
    struct dsoSettingsSamplerate {
        dsoSettingsSamplerateTarget target; ///< The target samplerate values
        ControlSamplerateLimits *limits; ///< The samplerate limits
        unsigned int downsampler; ///< The variable downsampling factor
        double current; ///< The current samplerate
    };

    //////////////////////////////////////////////////////////////////////////////
    /// \struct dsoSettingsTrigger
    /// \brief Stores the current trigger settings of the device.
    struct dsoSettingsTrigger {
        double level[HANTEK_CHANNELS]; ///< The trigger level for each channel in V
        double position; ///< The current pretrigger position
        unsigned int point; ///< The trigger position in Hantek coding
        TriggerMode mode; ///< The trigger mode
        Slope slope; ///< The trigger slope
        bool special; ///< true, if the trigger source is special
        unsigned int source; ///< The trigger source
    };

    //////////////////////////////////////////////////////////////////////////////
    /// \struct dsoSettingsVoltage
    /// \brief Stores the current amplification settings of the device.
    struct dsoSettingsVoltage {
        unsigned int gain; ///< The gain id
        double offset; ///< The screen offset for each channel
        double offsetReal; ///< The real offset for each channel (Due to quantization)
        bool used; ///< true, if the channel is used
    };

    //////////////////////////////////////////////////////////////////////////////
    /// \struct dsoSettings
    /// \brief Stores the current settings of the device.
    struct dsoSettings {
        dsoSettingsSamplerate samplerate; ///< The samplerate settings
        dsoSettingsVoltage voltage[HANTEK_CHANNELS]; ///< The amplification settings
        dsoSettingsTrigger trigger; ///< The trigger settings
        unsigned int recordLengthId; ///< The id in the record length array
        unsigned short int usedChannels; ///< Number of activated channels
    };
}