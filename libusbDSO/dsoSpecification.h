#pragma once

/// \todo Make channels configurable

#include <vector>

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
        dsoSpecificationSamplerate() {
            // Use DSO-2090 specification as default
            single.base = 50e6;
            single.max = 50e6;
            single.recordLengths.push_back(0);
            multi.base = 100e6;
            multi.max = 100e6;
            multi.recordLengths.push_back(0);
        }
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
        dsoSpecificationSamplerate samplerate; ///< The samplerate specifications
        std::vector<unsigned int> bufferDividers; ///< Samplerate dividers for record lengths
        std::vector<double> gainSteps; ///< Available voltage steps in V/screenheight
        unsigned char sampleSize  = 8; ///< Number of bits per sample. Default: 8bit ADC

        unsigned channels         = 0;
        unsigned channels_special = 0;
        std::vector<std::string> specialTriggerSources; ///< Names of the special trigger sources

        /// The index of the selected gain on the hardware
        std::vector<unsigned char> gainIndex;

        // Calibration
        struct channelLimits {
            /// The sample values at the top of the screen
            std::vector<unsigned short int> voltage;

            /// Calibration data for the channel offsets
            typedef std::array<unsigned short int,OFFSET_COUNT> GainStep;
            typedef std::array<GainStep, 9> Offsets;
            Offsets offset;

            channelLimits() {
                for(GainStep& gainStep: offset) {
                    gainStep[OFFSET_START] = 0x0000;
                    gainStep[OFFSET_END] = 0xffff;
                }
            }
        };
        std::vector<channelLimits> limits;

        dsoSpecification() {}
    };
}
