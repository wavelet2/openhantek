#pragma once

/// \todo Make channels configurable

namespace DSO {
    //////////////////////////////////////////////////////////////////////////////
    /// \struct ControlSamplerateLimits
    /// \brief Stores the samplerate limits for calculations.
    struct ControlSamplerateLimits {
        double base; ///< The base for sample rate calculations
        double max; ///< The maximum sample rate
        unsigned maxDownsampler; ///< The maximum downsampling ratio
        std::vector<unsigned> recordLengths; ///< Available record lengths, UINT_MAX means rolling
    };

    //////////////////////////////////////////////////////////////////////////////
    /// \struct dsoSpecificationSamplerate
    /// \brief Stores the samplerate limits.
    struct dsoSpecificationSamplerate {
        ControlSamplerateLimits single; ///< The limits for single channel mode
        ControlSamplerateLimits multi; ///< The limits for multi channel mode
    };

    //////////////////////////////////////////////////////////////////////////////
    /// \enum LevelOffset
    /// \brief The array indicies for the CalibrationData.
    enum LevelOffset {
        OFFSET_START, ///< The channel level at the bottom of the scope
        OFFSET_END, ///< The channel level at the top of the scope
        OFFSET_COUNT
    };

    //////////////////////////////////////////////////////////////////////////////
    /// \struct dsoSpecification
    /// \brief Stores the specifications of the currently connected device.
    struct dsoSpecification {
        // Limits
        dsoSpecificationSamplerate samplerate; ///< The samplerate specifications
        std::vector<unsigned int> bufferDividers; ///< Samplerate dividers for record lengths
        std::vector<double> gainSteps; ///< Available voltage steps in V/screenheight
        unsigned char sampleSize; ///< Number of bits per sample

        const unsigned channels = HANTEK_CHANNELS;
        std::vector<std::string> specialTriggerSources = {"EXT", "EXT/10"}; ///< Names of the special trigger sources

        /// The index of the selected gain on the hardware
        std::vector<unsigned char> gainIndex;

        // Calibration
        struct channelLimits {
            /// The sample values at the top of the screen
            std::vector<unsigned short int> voltage;
            /// Calibration data for the channel offsets \todo Should probably be a vector too
            unsigned short int offset[9][OFFSET_COUNT];
        } limits[HANTEK_CHANNELS];
    };
}