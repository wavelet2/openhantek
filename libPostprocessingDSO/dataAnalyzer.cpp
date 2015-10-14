////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  dataanalyzer.cpp
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

#include <cmath>
#include <fftw3.h>
#include "dataAnalyzer.h"
#include "deviceBase.h"
#include "errorcodes.h"
#include "utils/timestampDebug.h"

namespace DSOAnalyser {

DataAnalyzer::DataAnalyzer(DSO::DeviceBase* device, OpenHantekSettingsScope *analyserSettings)
    : _analyserSettings(analyserSettings), _analyseIsRunning(false), _device(device) {
        // lock analyse thread mutex
        _mutex.lock();

        // Connect to device
        using namespace std::placeholders;
        _device->_samplesAvailable = std::bind(&DataAnalyzer::data_from_device, this, _1, _2, _3, _4);

        // Create thread
        _thread = std::unique_ptr<std::thread>(new std::thread(&DataAnalyzer::analyseThread,std::ref(*this)));
    }

DataAnalyzer::~DataAnalyzer() {
    _device->_samplesAvailable = [](const std::vector<std::vector<double> > *,double,bool,std::mutex&){};
}

/// \brief Returns the analyzed data.
/// \param channel Channel, whose data should be returned.
/// \return Analyzed data as AnalyzedData struct.
AnalyzedData const *DataAnalyzer::data(unsigned channel) const {
    if(channel >= this->analyzedData.size())
        return 0;

    return &this->analyzedData[channel];
}

/// \brief Returns the sample count of the analyzed data.
/// \return The maximum sample count of the last analyzed data.
unsigned DataAnalyzer::sampleCount() {
    return _maxSamples;
}

/// \brief Returns the mutex for the data.
/// \return Mutex for the analyzed data.
std::mutex& DataAnalyzer::mutex() {
    return _mutex;
}

void DataAnalyzer::analyseSamples() {
    unsigned maxSamples = 0;
    unsigned channelCount = (unsigned) _analyserSettings->voltage.size();

    // Adapt the number of channels for analyzed data
    this->analyzedData.resize(channelCount);

    for(unsigned channel = 0; channel < channelCount; ++channel) {
         AnalyzedData *const channelData = &this->analyzedData[channel];

         bool validData =
            ( // ...if we got data for this channel...
                channel < _analyserSettings->physicalChannels &&
                channel < (unsigned) _incomingData.size() &&
                !_incomingData.at(channel).empty())
            ||
            ( // ...or if it's a math channel that can be calculated
                channel >= _analyserSettings->physicalChannels &&
                (_analyserSettings->voltage[channel].used || _analyserSettings->spectrum[channel].used) &&
                this->analyzedData.size() >= 2 &&
                !this->analyzedData[0].samples.voltage.sample.empty() &&
                !this->analyzedData[1].samples.voltage.sample.empty()
            );

        if (!validData) {
            // Clear unused channels
            channelData->samples.voltage.sample.clear();
            this->analyzedData[_analyserSettings->physicalChannels].samples.voltage.interval = 0;
            continue;
        }

        // Set sampling interval
        const double interval = 1.0 / _incoming_samplerate;
        if(interval != channelData->samples.voltage.interval) {
            channelData->samples.voltage.interval = interval;
            if(_incoming_append) // Clear roll buffer if the samplerate changed
                channelData->samples.voltage.sample.clear();
        }


        unsigned size;
        if(channel < _analyserSettings->physicalChannels) {
            size = _incomingData.at(channel).size();
            if(_incoming_append)
                size += channelData->samples.voltage.sample.size();
            if(size > maxSamples)
                maxSamples = size;
        }
        else
            size = maxSamples;

        // Physical channels
        if(channel < _analyserSettings->physicalChannels) {
            // Copy the buffer of the oscilloscope into the sample buffer
            if(_incoming_append)
                channelData->samples.voltage.sample.insert(channelData->samples.voltage.sample.end(), _incomingData.at(channel).begin(), _incomingData.at(channel).end());
            else
                channelData->samples.voltage.sample = _incomingData.at(channel);
        }
        // Math channel
        else {
            // Resize the sample vector
            channelData->samples.voltage.sample.resize(size);
            // Set sampling interval
            this->analyzedData[_analyserSettings->physicalChannels].samples.voltage.interval = this->analyzedData[0].samples.voltage.interval;

            // Resize the sample vector
            this->analyzedData[_analyserSettings->physicalChannels].samples.voltage.sample.resize(std::min(this->analyzedData[0].samples.voltage.sample.size(), this->analyzedData[1].samples.voltage.sample.size()));

            // Calculate values and write them into the sample buffer
            std::vector<double>::const_iterator ch1Iterator = this->analyzedData[0].samples.voltage.sample.begin();
            std::vector<double>::const_iterator ch2Iterator = this->analyzedData[1].samples.voltage.sample.begin();
            std::vector<double> &resultData = this->analyzedData[_analyserSettings->physicalChannels].samples.voltage.sample;
            for(std::vector<double>::iterator resultIterator = resultData.begin(); resultIterator != resultData.end(); ++resultIterator) {
                switch(_analyserSettings->voltage[_analyserSettings->physicalChannels].misc) {
                    case MATHMODE_1ADD2:
                        *(resultIterator++) = *(ch1Iterator++) + *(ch2Iterator++);
                        break;
                    case MATHMODE_1SUB2:
                        *(resultIterator++) = *(ch1Iterator++) - *(ch2Iterator++);
                        break;
                    case MATHMODE_2SUB1:
                        *(resultIterator++) = *(ch2Iterator++) - *(ch1Iterator++);
                        break;
                }
            }
        }
    }
    _maxSamples = maxSamples;
}

void DataAnalyzer::computeFreqSpectrumPeak(unsigned& lastRecordLength, WindowFunction& lastWindow, double *window) {
    for(unsigned channel = 0; channel < this->analyzedData.size(); ++channel) {
        AnalyzedData *const channelData = &this->analyzedData[channel];

        if(!channelData->samples.voltage.sample.empty()) {
            // Calculate new window
            unsigned sampleCount = channelData->samples.voltage.sample.size();
            if(lastWindow != _analyserSettings->spectrumWindow || lastRecordLength != sampleCount) {
                if(lastRecordLength != sampleCount) {
                    lastRecordLength = sampleCount;

                    if(window)
                        fftw_free(window);
                    window = (double *) fftw_malloc(sizeof(double) * lastRecordLength);
                }

                unsigned windowEnd = lastRecordLength - 1;
                lastWindow = _analyserSettings->spectrumWindow;

                switch(_analyserSettings->spectrumWindow) {
                    case WINDOW_HAMMING:
                        for(unsigned windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                            *(window + windowPosition) = 0.54 - 0.46 * cos(2.0 * M_PI * windowPosition / windowEnd);
                        break;
                    case WINDOW_HANN:
                        for(unsigned windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                            *(window + windowPosition) = 0.5 * (1.0 - cos(2.0 * M_PI * windowPosition / windowEnd));
                        break;
                    case WINDOW_COSINE:
                        for(unsigned windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                            *(window + windowPosition) = sin(M_PI * windowPosition / windowEnd);
                        break;
                    case WINDOW_LANCZOS:
                        for(unsigned windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition) {
                            double sincParameter = (2.0 * windowPosition / windowEnd - 1.0) * M_PI;
                            if(sincParameter == 0)
                                *(window + windowPosition) = 1;
                            else
                                *(window + windowPosition) = sin(sincParameter) / sincParameter;
                        }
                        break;
                    case WINDOW_BARTLETT:
                        for(unsigned windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                            *(window + windowPosition) = 2.0 / windowEnd * (windowEnd / 2 - abs(windowPosition - windowEnd / 2));
                        break;
                    case WINDOW_TRIANGULAR:
                        for(unsigned windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                            *(window + windowPosition) = 2.0 / lastRecordLength * (lastRecordLength / 2 - abs(windowPosition - windowEnd / 2));
                        break;
                    case WINDOW_GAUSS:
                        {
                            double sigma = 0.4;
                            for(unsigned windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                                *(window + windowPosition) = exp(-0.5 * pow(((windowPosition - windowEnd / 2) / (sigma * windowEnd / 2)), 2));
                        }
                        break;
                    case WINDOW_BARTLETTHANN:
                        for(unsigned windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                            *(window + windowPosition) = 0.62 - 0.48 * abs(windowPosition / windowEnd - 0.5) - 0.38 * cos(2.0 * M_PI * windowPosition / windowEnd);
                        break;
                    case WINDOW_BLACKMAN:
                        {
                            double alpha = 0.16;
                            for(unsigned windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                                *(window + windowPosition) = (1 - alpha) / 2 - 0.5 * cos(2.0 * M_PI * windowPosition / windowEnd) + alpha / 2 * cos(4.0 * M_PI * windowPosition / windowEnd);
                        }
                        break;
                    //case WINDOW_KAISER:
                        // TODO
                        //double alpha = 3.0;
                        //for(unsigned windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                            //*(window + windowPosition) = ;
                        //break;
                    case WINDOW_NUTTALL:
                        for(unsigned windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                            *(window + windowPosition) = 0.355768 - 0.487396 * cos(2 * M_PI * windowPosition / windowEnd) + 0.144232 * cos(4 * M_PI * windowPosition / windowEnd) - 0.012604 * cos(6 * M_PI * windowPosition / windowEnd);
                        break;
                    case WINDOW_BLACKMANHARRIS:
                        for(unsigned windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                            *(window + windowPosition) = 0.35875 - 0.48829 * cos(2 * M_PI * windowPosition / windowEnd) + 0.14128 * cos(4 * M_PI * windowPosition / windowEnd) - 0.01168 * cos(6 * M_PI * windowPosition / windowEnd);
                        break;
                    case WINDOW_BLACKMANNUTTALL:
                        for(unsigned windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                            *(window + windowPosition) = 0.3635819 - 0.4891775 * cos(2 * M_PI * windowPosition / windowEnd) + 0.1365995 * cos(4 * M_PI * windowPosition / windowEnd) - 0.0106411 * cos(6 * M_PI * windowPosition / windowEnd);
                        break;
                    case WINDOW_FLATTOP:
                        for(unsigned windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                            *(window + windowPosition) = 1.0 - 1.93 * cos(2 * M_PI * windowPosition / windowEnd) + 1.29 * cos(4 * M_PI * windowPosition / windowEnd) - 0.388 * cos(6 * M_PI * windowPosition / windowEnd) + 0.032 * cos(8 * M_PI * windowPosition / windowEnd);
                        break;
                    default: // WINDOW_RECTANGULAR
                        for(unsigned windowPosition = 0; windowPosition < lastRecordLength; ++windowPosition)
                            *(window + windowPosition) = 1.0;
                }
            }

            // Set sampling interval
            channelData->samples.spectrum.interval = 1.0 / channelData->samples.voltage.interval / sampleCount;

            // Number of real/complex samples
            unsigned dftLength = sampleCount / 2;

            // Reallocate memory for samples if the sample count has changed
            channelData->samples.spectrum.sample.resize(sampleCount);

            // Create sample buffer and apply window
            double *windowedValues = new double[sampleCount];
            for(unsigned position = 0; position < sampleCount; ++position)
                windowedValues[position] = window[position] * channelData->samples.voltage.sample[position];

            // Do discrete real to half-complex transformation
            /// \todo Check if record length is multiple of 2
            /// \todo Reuse plan and use FFTW_MEASURE to get fastest algorithm
            fftw_plan fftPlan = fftw_plan_r2r_1d(sampleCount, windowedValues, &channelData->samples.spectrum.sample.front(), FFTW_R2HC, FFTW_ESTIMATE);
            fftw_execute(fftPlan);
            fftw_destroy_plan(fftPlan);

            // Do an autocorrelation to get the frequency of the signal
            double *conjugateComplex = windowedValues; // Reuse the windowedValues buffer

            // Real values
            unsigned position;
            double correctionFactor = 1.0 / dftLength / dftLength;
            conjugateComplex[0] = (channelData->samples.spectrum.sample[0] * channelData->samples.spectrum.sample[0]) * correctionFactor;
            for(position = 1; position < dftLength; ++position)
                conjugateComplex[position] = (channelData->samples.spectrum.sample[position] * channelData->samples.spectrum.sample[position] + channelData->samples.spectrum.sample[sampleCount - position] * channelData->samples.spectrum.sample[sampleCount - position]) * correctionFactor;
            // Complex values, all zero for autocorrelation
            conjugateComplex[dftLength] = (channelData->samples.spectrum.sample[dftLength] * channelData->samples.spectrum.sample[dftLength]) * correctionFactor;
            for(++position; position < sampleCount; ++position)
                conjugateComplex[position] = 0;

            // Do half-complex to real inverse transformation
            double *correlation = new double[sampleCount];
            fftPlan = fftw_plan_r2r_1d(sampleCount, conjugateComplex, correlation, FFTW_HC2R, FFTW_ESTIMATE);
            fftw_execute(fftPlan);
            fftw_destroy_plan(fftPlan);
            delete[] conjugateComplex;

            // Calculate peak-to-peak voltage
            double minimalVoltage, maximalVoltage;
            minimalVoltage = maximalVoltage = channelData->samples.voltage.sample[0];

            for(unsigned position = 1; position < sampleCount; ++position) {
                if(channelData->samples.voltage.sample[position] < minimalVoltage)
                    minimalVoltage = channelData->samples.voltage.sample[position];
                else if(channelData->samples.voltage.sample[position] > maximalVoltage)
                    maximalVoltage = channelData->samples.voltage.sample[position];
            }

            channelData->amplitude = maximalVoltage - minimalVoltage;

            // Get the frequency from the correlation results
            double minimumCorrelation = correlation[0];
            double peakCorrelation = 0;
            unsigned peakPosition = 0;

            for(unsigned position = 1; position < sampleCount / 2; ++position) {
                if(correlation[position] > peakCorrelation && correlation[position] > minimumCorrelation * 2) {
                    peakCorrelation = correlation[position];
                    peakPosition = position;
                }
                else if(correlation[position] < minimumCorrelation)
                    minimumCorrelation = correlation[position];
            }
            delete[] correlation;

            // Calculate the frequency in Hz
            if(peakPosition)
                channelData->frequency = 1.0 / (channelData->samples.voltage.interval * peakPosition);
            else
                channelData->frequency = 0;

            // Finally calculate the real spectrum if we want it
            if(_analyserSettings->spectrum[channel].used) {
                // Convert values into dB (Relative to the reference level)
                double offset = 60 - _analyserSettings->spectrumReference - 20 * log10(dftLength);
                double offsetLimit = _analyserSettings->spectrumLimit - _analyserSettings->spectrumReference;
                for(std::vector<double>::iterator spectrumIterator = channelData->samples.spectrum.sample.begin(); spectrumIterator != channelData->samples.spectrum.sample.end(); ++spectrumIterator) {
                    double value = 20 * log10(fabs(channelData->samples.spectrum.sample[position])) + offset;

                    // Check if this value has to be limited
                    if(offsetLimit > value)
                        value = offsetLimit;

                    *spectrumIterator = value;
                }
            }
        }
        else if(!channelData->samples.spectrum.sample.empty()) {
            // Clear unused channels
            channelData->samples.spectrum.interval = 0;
            channelData->samples.spectrum.sample.clear();
        }
    }
}

void DataAnalyzer::analyseThread() {
    unsigned lastRecordLength = 0; ///< The record length of the previously analyzed data
    WindowFunction lastWindow     = WINDOW_UNDEFINED; ///< The previously used dft window function
    double *window                = nullptr; ///< The array for the dft window factors

    while(1) {
        _mutex.lock();
        _analyseIsRunning = true;
        analyseSamples();
        computeFreqSpectrumPeak(lastRecordLength, lastWindow, window);
        _analyzed(_maxSamples);
        _analyseIsRunning = false;

        static unsigned long id = 0;
        (void)id;
        timestampDebug("Analyzed packet " << id++);
    }
}

/// \brief Starts the analyzing of new input data.
/// \param data The data arrays with the input data.
/// \param size The sizes of the data arrays.
/// \param samplerate The samplerate for all input data.
/// \param append The data will be appended to the previously analyzed data (Roll mode).
/// \param mutex The mutex for all input data.
void DataAnalyzer::data_from_device(const std::vector<std::vector<double> > *data, double samplerate, bool append, std::mutex& mutex) {
    // Previous analysis still running, drop the new data
    if(_analyseIsRunning) {
        timestampDebug("Analyzer overload, dropping packets!");
        return;
    }

    // Lock the device thread, make a copy of the sample data, unlock the device thread.
    mutex.lock();
    _incomingData = std::vector<std::vector<double>>(*data);
    mutex.unlock();
    _incoming_samplerate = samplerate;
    _incoming_append = append;
    _mutex.unlock(); ///< New data arrived, unlock analyse thread
}

}