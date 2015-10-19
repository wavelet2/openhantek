////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//
//  Copyright (C) 2008, 2009  Oleg Khudyakov
//  prcoder@potrebitel.ru
//  Copyright (C) 2010 - 2012  Oliver Haag
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


#include <vector>
#include <iostream>
#include <algorithm>
#include <climits>

#include "deviceBaseSamples.h"
#include "utils/timestampDebug.h"

namespace DSO {

std::vector<int>& operator<<(std::vector<int>& v, int x);
std::vector<unsigned short int>& operator<<(std::vector<unsigned short int>& v, unsigned short int x);

void DeviceBaseSamples::startSampling() {
    _sampling = true;
    _samplingStarted();
}

void DeviceBaseSamples::stopSampling() {
    _sampling = false;
    _samplingStopped();
}

bool DeviceBaseSamples::toogleSampling() {
    _sampling = !_sampling;
    if (_sampling)
        _samplingStarted();
    else
        _samplingStopped();

    return _sampling;
}

std::vector<unsigned> *DeviceBaseSamples::getAvailableRecordLengths() {
    return &_settings.samplerate.limits->recordLengths;
}


double DeviceBaseSamples::getMinSamplerate() {
    return (double) _specification.samplerate.single.base / _specification.samplerate.single.maxDownsampler;
}

double DeviceBaseSamples::getMaxSamplerate() {
    ControlSamplerateLimits *limits = (_settings.usedChannels <= 1) ? &_specification.samplerate.multi : &_specification.samplerate.single;
    return limits->max;
}

void DeviceBaseSamples::updateSamplerateLimits() {
    // Works only if the minimum samplerate for normal mode is lower than for fast rate mode, which is the case for all models
    ControlSamplerateLimits *limits = (_settings.usedChannels <= 1) ? &_specification.samplerate.multi : &_specification.samplerate.single;
    _samplerateLimitsChanged((double) _specification.samplerate.single.base / _specification.samplerate.single.maxDownsampler / _specification.bufferDividers[_settings.recordLengthId], limits->max / _specification.bufferDividers[_settings.recordLengthId]);
}

double DeviceBaseSamples::setSamplerate(double samplerate) {
    if(samplerate == 0.0) {
        samplerate = _settings.samplerate.target.samplerate;
    }
    else {
        _settings.samplerate.target.samplerate = samplerate;
        _settings.samplerate.target.samplerateSet = true;
    }

    // When possible, enable fast rate if it is required to reach the requested samplerate
    bool fastRate = (_settings.usedChannels <= 1) && (samplerate > _specification.samplerate.single.max / _specification.bufferDividers[_settings.recordLengthId]);

    // What is the nearest, at least as high samplerate the scope can provide?
    unsigned int downsampler = 0;
    double bestSamplerate = computeBestSamplerate(samplerate, fastRate, false, &(downsampler));

    // Set the calculated samplerate
    if(updateSamplerate(downsampler, fastRate) == UINT_MAX)
        return 0.0;
    else {
        return bestSamplerate;
    }
}


double DeviceBaseSamples::setRecordTime(double duration) {
    if(duration == 0.0) {
        duration = _settings.samplerate.target.duration;
    }
    else {
        _settings.samplerate.target.duration = duration;
        _settings.samplerate.target.samplerateSet = false;
    }

    // Calculate the maximum samplerate that would still provide the requested duration
    double maxSamplerate = (double) _specification.samplerate.single.recordLengths[_settings.recordLengthId] / duration;

    // When possible, enable fast rate if the record time can't be set that low to improve resolution
    bool fastRate = (_settings.usedChannels <= 1) && (maxSamplerate >= _specification.samplerate.multi.base / _specification.bufferDividers[_settings.recordLengthId]);

    // What is the nearest, at most as high samplerate the scope can provide?
    unsigned int downsampler = 0;
    double bestSamplerate = computeBestSamplerate(maxSamplerate, fastRate, true, &(downsampler));

    // Set the calculated samplerate
    if(updateSamplerate(downsampler, fastRate) == UINT_MAX)
        return 0.0;
    else {
        return (double) _settings.samplerate.limits->recordLengths[_settings.recordLengthId] / bestSamplerate;
    }
}


unsigned int DeviceBaseSamples::getSampleCount(unsigned packetSize) {
    unsigned int totalSampleCount = _settings.samplerate.limits->recordLengths[_settings.recordLengthId];
    bool fastRateEnabled = _settings.samplerate.limits == &_specification.samplerate.multi;

    if(totalSampleCount == UINT_MAX) {
        // Roll mode
        if(packetSize > 0)
            totalSampleCount = packetSize;
    }
    else {
        if(!fastRateEnabled)
            totalSampleCount *= _specification.channels;
    }

    return totalSampleCount;
}

void DeviceBaseSamples::restoreTargets() {
    if(_settings.samplerate.target.samplerateSet)
        setSamplerate();
    else
        setRecordTime();
}

void DeviceBaseSamples::setRecordLength(unsigned int index) {
    if(!updateRecordLength(index))
        return;

    restoreTargets();
    updatePretriggerPosition(_settings.trigger.position);

    _recordLengthChanged(_settings.samplerate.limits->recordLengths, _settings.recordLengthId);
}

void DeviceBaseSamples::processSamples(unsigned char* data, int dataLength, unsigned totalSampleCount) {
    unsigned int sampleCount = totalSampleCount;

    // How much data did we really receive?
    if(_specification.sampleSize > 8)
        totalSampleCount = dataLength / 2; // For 9bit-16bit Analog digital converters
    else
        totalSampleCount = dataLength;

    // Convert channel data
    if(isFastRate()) {
        // Fast rate mode, one channel is using all buffers
        sampleCount = totalSampleCount;
        unsigned channel = 0;
        for(; channel < _specification.channels; ++channel) {
            if(_settings.voltage[channel].used)
                break;
        }

        // Clear unused channels
        for(unsigned channelCounter = 0; channelCounter < _specification.channels; ++channelCounter)
            if(channelCounter != channel) {
                _samples[channelCounter].clear();
            }

        if(channel < _specification.channels) {
            // Resize sample vector
            _samples[channel].resize(sampleCount);

            // Convert data from the oscilloscope and write it into the sample buffer
            unsigned int bufferPosition = _settings.trigger.point * 2;
            if(_specification.sampleSize > 8) {
                // Additional most significant bits after the normal data
                unsigned int extraBitsPosition; // Track the position of the extra bits in the additional byte
                unsigned int extraBitsSize = _specification.sampleSize - 8; // Number of extra bits
                unsigned short int extraBitsMask = (0x00ff << extraBitsSize) & 0xff00; // Mask for extra bits extraction

                for(unsigned int realPosition = 0; realPosition < sampleCount; ++realPosition, ++bufferPosition) {
                    if(bufferPosition >= sampleCount)
                            bufferPosition %= sampleCount;

                    extraBitsPosition = bufferPosition % _specification.channels;

                    _samples[channel][realPosition] = ((double) ((unsigned short int) data[bufferPosition] + (((unsigned short int) data[sampleCount + bufferPosition - extraBitsPosition] << (8 - (_specification.channels - 1 - extraBitsPosition) * extraBitsSize)) & extraBitsMask)) / _specification.limits[channel].voltage[_settings.voltage[channel].gain] - _settings.voltage[channel].offsetReal) * _specification.gainSteps[_settings.voltage[channel].gain];
                }
            }
            else {
                for(unsigned int realPosition = 0; realPosition < sampleCount; ++realPosition, ++bufferPosition) {
                    if(bufferPosition >= sampleCount)
                            bufferPosition %= sampleCount;

                    _samples[channel][realPosition] = ((double) data[bufferPosition] / _specification.limits[channel].voltage[_settings.voltage[channel].gain] - _settings.voltage[channel].offsetReal) * _specification.gainSteps[_settings.voltage[channel].gain];
                }
            }
        }
    } else {
        // Normal mode, channels are using their separate buffers
        sampleCount = totalSampleCount / _specification.channels;
        for(unsigned channel = 0; channel < _specification.channels; ++channel) {
            if(!_settings.voltage[channel].used) {
                // Clear unused channels
                _samples[channel].clear();
                continue;
            }

            // Resize sample vector
            _samples[channel].resize(sampleCount);

            // Convert data from the oscilloscope and write it into the sample buffer
            unsigned bufferPosition = _settings.trigger.point * 2;
            if(_specification.sampleSize > 8) {
                // Additional most significant bits after the normal data
                unsigned extraBitsSize = _specification.sampleSize - 8; // Number of extra bits
                unsigned short int extraBitsMask = (0x00ff << extraBitsSize) & 0xff00; // Mask for extra bits extraction
                unsigned extraBitsIndex = 8 - channel * 2; // Bit position offset for extra bits extraction

                for(unsigned realPosition = 0; realPosition < sampleCount; ++realPosition, bufferPosition += _specification.channels) {
                    if(bufferPosition >= totalSampleCount)
                        bufferPosition %= totalSampleCount;

                    _samples[channel][realPosition] = ((double) ((unsigned short int) data[bufferPosition + _specification.channels - 1 - channel] + (((unsigned short int) data[totalSampleCount + bufferPosition] << extraBitsIndex) & extraBitsMask)) / _specification.limits[channel].voltage[_settings.voltage[channel].gain] - _settings.voltage[channel].offsetReal) * _specification.gainSteps[_settings.voltage[channel].gain];
                }
            }
            else {
                bufferPosition += _specification.channels - 1 - channel;
                for(unsigned realPosition = 0; realPosition < sampleCount; ++realPosition, bufferPosition += _specification.channels) {
                    if(bufferPosition >= totalSampleCount)
                            bufferPosition %= totalSampleCount;

                    _samples[channel][realPosition] = ((double) data[bufferPosition] / _specification.limits[channel].voltage[_settings.voltage[channel].gain] - _settings.voltage[channel].offsetReal) * _specification.gainSteps[_settings.voltage[channel].gain];
                }
            }
        }
    }

    static unsigned id = 0;
    (void)id;
    timestampDebug("Received packet " << id++);
}

}
