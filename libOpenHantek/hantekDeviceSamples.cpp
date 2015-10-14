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

#include "hantekDevice.h"

namespace Hantek {

std::vector<int>& operator<<(std::vector<int>& v, int x);
std::vector<unsigned short int>& operator<<(std::vector<unsigned short int>& v, unsigned short int x);

double HantekDevice::computeBestSamplerate(double samplerate, bool fastRate, bool maximum, unsigned int *downsampler) {
    // Abort if the input value is invalid
    if(samplerate <= 0.0)
        return 0.0;

    double bestSamplerate = 0.0;

    // Get samplerate specifications for this mode and model
    DSO::ControlSamplerateLimits *limits;
    if(fastRate)
        limits = &(_specification.samplerate.multi);
    else
        limits = &(_specification.samplerate.single);

    // Get downsampling factor that would provide the requested rate
    double bestDownsampler = (double) limits->base / _specification.bufferDividers[_settings.recordLengthId] / samplerate;
    // Base samplerate sufficient, or is the maximum better?
    if(bestDownsampler < 1.0 && (samplerate <= limits->max / _specification.bufferDividers[_settings.recordLengthId] || !maximum)) {
        bestDownsampler = 0.0;
        bestSamplerate = limits->max / _specification.bufferDividers[_settings.recordLengthId];
    }
    else {
        switch(specificationCommands.bulk.setSamplerate) {
            case BULK_SETTRIGGERANDSAMPLERATE:
                // DSO-2090 supports the downsampling factors 1, 2, 4 and 5 using valueFast or all even values above using valueSlow
                if((maximum && bestDownsampler <= 5.0) || (!maximum && bestDownsampler < 6.0)) {
                    // valueFast is used
                    if(maximum) {
                        // The samplerate shall not be higher, so we round up
                        bestDownsampler = ceil(bestDownsampler);
                        if(bestDownsampler > 2.0) // 3 and 4 not possible with the DSO-2090
                            bestDownsampler = 5.0;
                    }
                    else {
                        // The samplerate shall not be lower, so we round down
                        bestDownsampler = floor(bestDownsampler);
                        if(bestDownsampler > 2.0 && bestDownsampler < 5.0) // 3 and 4 not possible with the DSO-2090
                            bestDownsampler = 2.0;
                    }
                }
                else {
                    // valueSlow is used
                    if(maximum) {
                        bestDownsampler = ceil(bestDownsampler / 2.0) * 2.0; // Round up to next even value
                    }
                    else {
                        bestDownsampler = floor(bestDownsampler / 2.0) * 2.0; // Round down to next even value
                    }
                    if(bestDownsampler > 2.0 * 0x10001) // Check for overflow
                        bestDownsampler = 2.0 * 0x10001;
                }
                break;

            case BULK_CSETTRIGGERORSAMPLERATE:
                // DSO-5200 may not supports all downsampling factors, requires testing
                if(maximum) {
                    bestDownsampler = ceil(bestDownsampler); // Round up to next integer value
                }
                else {
                    bestDownsampler = floor(bestDownsampler); // Round down to next integer value
                }
                break;

            case BULK_ESETTRIGGERORSAMPLERATE:
                // DSO-2250 doesn't have a fast value, so it supports all downsampling factors
                if(maximum) {
                    bestDownsampler = ceil(bestDownsampler); // Round up to next integer value
                }
                else {
                    bestDownsampler = floor(bestDownsampler); // Round down to next integer value
                }
                break;

            default:
                return 0.0;
        }

        // Limit maximum downsampler value to avoid overflows in the sent commands
        if(bestDownsampler > limits->maxDownsampler)
            bestDownsampler = limits->maxDownsampler;

        bestSamplerate = limits->base / bestDownsampler / _specification.bufferDividers[_settings.recordLengthId];
    }

    if(downsampler)
        *downsampler = (unsigned int) bestDownsampler;
    return bestSamplerate;
}

unsigned int HantekDevice::updateRecordLength(unsigned int index) {
    if(index >= (unsigned int) _settings.samplerate.limits->recordLengths.size())
        return 0;

    switch(specificationCommands.bulk.setRecordLength) {
        case BULK_SETTRIGGERANDSAMPLERATE:
            // SetTriggerAndSamplerate bulk command for record length
            static_cast<BulkSetTriggerAndSamplerate *>(_command[BULK_SETTRIGGERANDSAMPLERATE].get())->setRecordLength(index);
            _commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;

            break;

        case BULK_DSETBUFFER:
            if(specificationCommands.bulk.setPretrigger == BULK_FSETBUFFER) {
                // Pointers to needed commands
                BulkSetRecordLength2250 *commandSetRecordLength2250 = static_cast<BulkSetRecordLength2250 *>(_command[BULK_DSETBUFFER].get());

                commandSetRecordLength2250->setRecordLength(index);
            }
            else {
                // SetBuffer5200 bulk command for record length
                BulkSetBuffer5200 *commandSetBuffer5200 = static_cast<BulkSetBuffer5200 *>(_command[BULK_DSETBUFFER].get());

                commandSetBuffer5200->setUsedPre(DTRIGGERPOSITION_ON);
                commandSetBuffer5200->setUsedPost(DTRIGGERPOSITION_ON);
                commandSetBuffer5200->setRecordLength(index);
            }

            _commandPending[BULK_DSETBUFFER] = true;

            break;

        default:
            return 0;
    }

    // Check if the divider has changed and adapt samplerate limits accordingly
    bool bDividerChanged = _specification.bufferDividers[index] != _specification.bufferDividers[_settings.recordLengthId];

    _settings.recordLengthId = index;

    if(bDividerChanged) {
        this->updateSamplerateLimits();

        // Samplerate dividers changed, recalculate it
        this->restoreTargets();
    }

    return _settings.samplerate.limits->recordLengths[index];
}

unsigned int HantekDevice::updateSamplerate(unsigned int downsampler, bool fastRate) {
    // Get samplerate limits
    DSO::ControlSamplerateLimits *limits = fastRate ? &_specification.samplerate.multi : &_specification.samplerate.single;

    // Set the calculated samplerate
    switch(specificationCommands.bulk.setSamplerate) {
        case BULK_SETTRIGGERANDSAMPLERATE: {
            short int downsamplerValue = 0;
            unsigned char samplerateId = 0;
            bool downsampling = false;

            if(downsampler <= 5) {
                // All dividers up to 5 are done using the special samplerate IDs
                if(downsampler == 0 && limits->base >= limits->max)
                    samplerateId = 1;
                else if(downsampler <= 2)
                    samplerateId = downsampler;
                else { // Downsampling factors 3 and 4 are not supported
                    samplerateId = 3;
                    downsampler = 5;
                    downsamplerValue = 0xffff;
                }
            }
            else {
                // For any dividers above the downsampling factor can be set directly
                downsampler &= ~0x0001; // Only even values possible
                downsamplerValue = (short int) (0x10001 - (downsampler >> 1));

                downsampling = true;
            }

            // Pointers to needed commands
            BulkSetTriggerAndSamplerate *commandSetTriggerAndSamplerate = static_cast<BulkSetTriggerAndSamplerate *>(_command[BULK_SETTRIGGERANDSAMPLERATE].get());

            // Store if samplerate ID or downsampling factor is used
            commandSetTriggerAndSamplerate->setDownsamplingMode(downsampling);
            // Store samplerate ID
            commandSetTriggerAndSamplerate->setSamplerateId(samplerateId);
            // Store downsampling factor
            commandSetTriggerAndSamplerate->setDownsampler(downsamplerValue);
            // Set fast rate when used
            commandSetTriggerAndSamplerate->setFastRate(false /*fastRate*/);

            _commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;

            break;
        }
        case BULK_CSETTRIGGERORSAMPLERATE: {
            // Split the resulting divider into the values understood by the device
            // The fast value is kept at 4 (or 3) for slow sample rates
            long int valueSlow = std::max(((long int) downsampler - 3) / 2, (long int) 0);
            unsigned char valueFast = downsampler - valueSlow * 2;

            // Pointers to needed commands
            BulkSetSamplerate5200 *commandSetSamplerate5200 = static_cast<BulkSetSamplerate5200 *>(_command[BULK_CSETTRIGGERORSAMPLERATE].get());
            BulkSetTrigger5200 *commandSetTrigger5200 = static_cast<BulkSetTrigger5200 *>(_command[BULK_ESETTRIGGERORSAMPLERATE].get());

            // Store samplerate fast value
            commandSetSamplerate5200->setSamplerateFast(4 - valueFast);
            // Store samplerate slow value (two's complement)
            commandSetSamplerate5200->setSamplerateSlow(valueSlow == 0 ? 0 : 0xffff - valueSlow);
            // Set fast rate when used
            commandSetTrigger5200->setFastRate(fastRate);

            _commandPending[BULK_CSETTRIGGERORSAMPLERATE] = true;
            _commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;

            break;
        }
        case BULK_ESETTRIGGERORSAMPLERATE: {
            // Pointers to needed commands
            BulkSetSamplerate2250 *commandSetSamplerate2250 = static_cast<BulkSetSamplerate2250 *>(_command[BULK_ESETTRIGGERORSAMPLERATE].get());

            bool downsampling = downsampler >= 1;
            // Store downsampler state value
            commandSetSamplerate2250->setDownsampling(downsampling);
            // Store samplerate value
            commandSetSamplerate2250->setSamplerate(downsampler > 1 ? 0x10001 - downsampler : 0);
            // Set fast rate when used
            commandSetSamplerate2250->setFastRate(fastRate);

            _commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;

            break;
        }
        default:
            return UINT_MAX;
    }

    // Update settings
    bool fastRateChanged = fastRate != (_settings.samplerate.limits == &_specification.samplerate.multi);
    if(fastRateChanged) {
        _settings.samplerate.limits = limits;
    }

    _settings.samplerate.downsampler = downsampler;
    if(downsampler)
        _settings.samplerate.current = _settings.samplerate.limits->base / _specification.bufferDividers[_settings.recordLengthId] / downsampler;
    else
        _settings.samplerate.current = _settings.samplerate.limits->max / _specification.bufferDividers[_settings.recordLengthId];

    // Update dependencies
    this->setPretriggerPosition(_settings.trigger.position);

    // Emit signals for changed settings
    if(fastRateChanged) {
        _availableRecordLengthsChanged(_settings.samplerate.limits->recordLengths);
        _recordLengthChanged(_settings.samplerate.limits->recordLengths[_settings.recordLengthId]);
    }

    // Check for Roll mode
    if(!isRollingMode())
                _recordTimeChanged((double) _settings.samplerate.limits->recordLengths[_settings.recordLengthId] / _settings.samplerate.current);

    _samplerateChanged(_settings.samplerate.current);

    return downsampler;
}

}
