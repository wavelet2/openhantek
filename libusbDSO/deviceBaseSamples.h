////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//
/// \copyright (c) 2008, 2009 Oleg Khudyakov <prcoder@potrebitel.ru>
/// \copyright (c) 2010 - 2012 Oliver Haag <oliver.haag@gmail.com>
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

#include <functional>
#include <vector>
#include <mutex>
#include <climits>

#include "utils/containerStream.h"
#include "dsoSettings.h"
#include "dsoSpecification.h"
#include "errorcodes.h"
#include "deviceDescriptionEntry.h"

#include "deviceBaseCommands.h"
#include "deviceBaseSpecifications.h"

namespace DSO {

//////////////////////////////////////////////////////////////////////////////
/// \brief Part of the base class for an DSO device implementation. All sample
///        related parts are implemented here.
class DeviceBaseSamples : public DeviceBaseCommands, public DeviceBaseSpecifications {
public:
    DeviceBaseSamples(const DSODeviceDescription& model) : DeviceBaseSpecifications(model), _samples(_specification.channels) {}

    /// \brief Get available record lengths for this oscilloscope.
    /// \return The number of physical channels, empty list for continuous.
    std::vector<unsigned> *getAvailableRecordLengths();

    /// \brief Get minimum samplerate for this oscilloscope.
    /// \return The minimum samplerate for the current configuration in S/s.
    double getMinSamplerate();

    /// \brief Get maximum samplerate for this oscilloscope.
    /// \return The maximum samplerate for the current configuration in S/s.
    double getMaxSamplerate();

    /// \brief Get the count of samples that are expected returned by the scope.
    /// \param fastRate Is set to the state of the fast rate mode when provided.
    /// \return The total number of samples the scope should return.
    unsigned int getSampleCount(bool *fastRate = 0);

    /// \brief Restore the samplerate/timebase targets after divider updates.
    void restoreTargets();

    /// \brief Update the minimum and maximum supported samplerate.
    void updateSamplerateLimits();

    /// \brief Sets the size of the oscilloscopes sample buffer.
    /// \param index The record length index that should be set.
    /// \return The record length that has been set, 0 on error.
    unsigned int setRecordLength(unsigned int size);

    /// \brief Sets the samplerate of the oscilloscope.
    /// \param samplerate The samplerate that should be met (S/s), 0.0 to restore current samplerate.
    /// \return The samplerate that has been set, 0.0 on error.
    double setSamplerate(double samplerate = 0.0);

    /// \brief Sets the time duration of one aquisition by adapting the samplerate.
    /// \param duration The record time duration that should be met (s), 0.0 to restore current record time.
    /// \return The record time duration that has been set, 0.0 on error.
    double setRecordTime(double duration = 0.0);

    /// \brief Calculated the nearest samplerate supported by the oscilloscope.
    /// \param samplerate The target samplerate, that should be met as good as possible.
    /// \param fastRate true, if the fast rate mode is enabled.
    /// \param maximum The target samplerate is the maximum allowed when true, the minimum otherwise.
    /// \param downsampler Pointer to where the selected downsampling factor should be written.
    /// \return The nearest samplerate supported, 0.0 on error.
    virtual double computeBestSamplerate(double samplerate, bool fastRate, bool maximum, unsigned int *downsampler) = 0;

    /// \brief Sets the size of the sample buffer without updating dependencies.
    /// \param index The record length index that should be set.
    /// \return The record length that has been set, 0 on error.
    virtual unsigned int updateRecordLength(unsigned int index) = 0;

    /// \brief Sets the samplerate based on the parameters calculated by Control::computeBestSamplerate.
    /// \param downsampler The downsampling factor.
    /// \param fastRate true, if one channel uses all buffers.
    /// \return The downsampling factor that has been set.
    virtual unsigned int updateSamplerate(unsigned int downsampler, bool fastRate) = 0;

    /// \brief Start sampling process.
    void startSampling();

    /// \brief Stop sampling process.
    void stopSampling();

    inline bool isRollingMode() { return _settings.samplerate.limits->recordLengths[_settings.recordLengthId] == UINT_MAX; }

    /// Implement this and return the communication packet size in bytes, e.g. 64 on FullSpeed USB
    virtual int getCommunicationPacketSize() = 0;
    virtual double setPretriggerPosition(double position) = 0;

protected:
    std::vector<std::vector<double>> _samples; ///< Sample data vectors sent to the data analyzer
    unsigned int _previousSampleCount = 0;     ///< The expected total number of samples at the last check before sampling started
    std::mutex _samplesMutex;                  ///< Mutex for the sample data
    bool _sampling               = false;      ///< true, if the oscilloscope is taking samples

    /// \brief Called by readSamples(). Converts samples.
    void processSamples(unsigned char* data, int dataLength, unsigned totalSampleCount, bool fastRate);
public:
    /**
     * This section contains callback methods. Register your function or class method to get notified
     * of events.
     */

    /// The oscilloscope started sampling/waiting for trigger
    std::function<void(void)> _samplingStarted = [](){};
    /// The oscilloscope stopped sampling/waiting for trigger
    std::function<void(void)> _samplingStopped = [](){};

    /// New sample data is available as channel[data] vectors, the samplerate, rollMode flag
    /// and mutex to block the communication thread if necessary to parse the incoming data.
    std::function<void(const std::vector<std::vector<double> > *,double,bool,std::mutex&)> _samplesAvailable
        = [](const std::vector<std::vector<double> > *,double,bool,std::mutex&){};

    /// The available record lengths, empty list for continuous
    std::function<void(const std::vector<unsigned> &)> _availableRecordLengthsChanged = [](const std::vector<unsigned> &){};
    /// The record length has changed
    std::function<void(unsigned long)> _recordLengthChanged = [](unsigned long){};

    /// The minimum or maximum samplerate has changed
    std::function<void(double,double)> _samplerateLimitsChanged = [](double, double){};

    /// The record time duration has changed
    std::function<void(double)> _recordTimeChanged = [](double){};
    /// The samplerate has changed
    std::function<void(double)> _samplerateChanged = [](double){};
};

}
