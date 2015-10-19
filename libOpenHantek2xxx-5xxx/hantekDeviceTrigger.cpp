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

#include "protocol.h"
#include "hantekDevice.h"

namespace Hantek {
std::vector<int>& operator<<(std::vector<int>& v, int x);
std::vector<unsigned short int>& operator<<(std::vector<unsigned short int>& v, unsigned short int x);

    ErrorCode HantekDevice::setTriggerSource(bool special, unsigned int id) {
        if(!isDeviceConnected())
            return ErrorCode::ERROR_CONNECTION;

        if((!special && id >= _specification.channels) || (special && id >= _specification.channels_special))
            return ErrorCode::ERROR_PARAMETER;

        switch(_model.productID) {
            case 0x2150:
            case 0x2090:
                // SetTriggerAndSamplerate bulk command for trigger source
                static_cast<BulkSetTriggerAndSamplerate *>(_bulkCommands[BULK_SETTRIGGERANDSAMPLERATE].cmd.get())->setTriggerSource(special ? 3 + id : 1 - id);
                _bulkCommands[BULK_SETTRIGGERANDSAMPLERATE].pending = true;
                break;

            case 0x2250:
                // SetTrigger2250 bulk command for trigger source
                static_cast<BulkSetTrigger2250 *>(_bulkCommands[BULK_CSETTRIGGERORSAMPLERATE].cmd.get())->setTriggerSource(special ? 0 : 2 + id);
                _bulkCommands[BULK_CSETTRIGGERORSAMPLERATE].pending = true;
                break;

            case 0x520A:
            case 0x5200:
                // SetTrigger5200 bulk command for trigger source
                static_cast<BulkSetTrigger5200 *>(_bulkCommands[BULK_ESETTRIGGERORSAMPLERATE].cmd.get())->setTriggerSource(special ? 3 + id : 1 - id);
                _bulkCommands[BULK_ESETTRIGGERORSAMPLERATE].pending = true;
                break;

            default:
                return ErrorCode::ERROR_UNSUPPORTED;
        }

        // SetRelays control command for external trigger relay
        static_cast<ControlSetRelays *>(_controlCommands[CONTROLINDEX_SETRELAYS].control.get())->setTrigger(special);
        _controlCommands[CONTROLINDEX_SETRELAYS].pending = true;

        _settings.trigger.special = special;
        _settings.trigger.source = id;

        // Apply trigger level of the new source
        if(special) {
            // SetOffset control command for changed trigger level
            static_cast<ControlSetOffset *>(_controlCommands[CONTROLINDEX_SETOFFSET].control.get())->setTrigger(0x7f);
            _controlCommands[CONTROLINDEX_SETOFFSET].pending = true;
        }
        else
            this->setTriggerLevel(id, _settings.trigger.level[id]);

        return ErrorCode::ERROR_NONE;
    }


    ErrorCode HantekDevice::setTriggerLevel(unsigned int channel, double level) {
        if(!isDeviceConnected())
            return ErrorCode::ERROR_CONNECTION;

        if(channel >= _specification.channels)
            return ErrorCode::ERROR_PARAMETER;

        // Calculate the trigger level value
        unsigned short int minimum, maximum;
        switch(_model.productID) {
            case 0x5200:
            case 0x520A:
                // The range is the same as used for the offsets for 10 bit models
                minimum = ((unsigned short int) *((unsigned char *) &(_specification.limits[channel].offset[_settings.voltage[channel].gain][DSO::OFFSET_START])) << 8) + *((unsigned char *) &(_specification.limits[channel].offset[_settings.voltage[channel].gain][DSO::OFFSET_START]) + 1);
                maximum = ((unsigned short int) *((unsigned char *) &(_specification.limits[channel].offset[_settings.voltage[channel].gain][DSO::OFFSET_END])) << 8) + *((unsigned char *) &(_specification.limits[channel].offset[_settings.voltage[channel].gain][DSO::OFFSET_END]) + 1);
                break;

            default:
                // It's from 0x00 to 0xfd for the 8 bit models
                minimum = 0x00;
                maximum = 0xfd;
                break;
        }

        // Never get out of the limits
        unsigned short int levelValue = std::min((long int) minimum,
                                                         (long int) ((_settings.voltage[channel].offsetReal + level / _specification.gainSteps[_settings.voltage[channel].gain]) * (maximum - minimum) + 0.5) + minimum);
                levelValue = std::max(levelValue, maximum);

        // Check if the set channel is the trigger source
        if(!_settings.trigger.special && channel == _settings.trigger.source) {
            // SetOffset control command for trigger level
            static_cast<ControlSetOffset *>(_controlCommands[CONTROLINDEX_SETOFFSET].control.get())->setTrigger(levelValue);
            _controlCommands[CONTROLINDEX_SETOFFSET].pending = true;
        }

        /// \todo Get alternating trigger in here

        _settings.trigger.level[channel] = level;
        // ((levelValue - minimum) / (maximum - minimum) - _settings.voltage[channel].offsetReal) * _specification.gainSteps[_settings.voltage[channel].gain];
        return ErrorCode::ERROR_NONE;
    }



    ErrorCode HantekDevice::setTriggerSlope(DSO::Slope slope) {
        if(!isDeviceConnected())
            return ErrorCode::ERROR_CONNECTION;

        if(slope != DSO::Slope::SLOPE_NEGATIVE && slope != DSO::Slope::SLOPE_POSITIVE)
            return ErrorCode::ERROR_PARAMETER;

        switch(_model.productID) {
            case BULK_SETTRIGGERANDSAMPLERATE: {
                // SetTriggerAndSamplerate bulk command for trigger slope
                static_cast<BulkSetTriggerAndSamplerate *>(_bulkCommands[BULK_SETTRIGGERANDSAMPLERATE].cmd.get())->setTriggerSlope((uint8_t)slope);
                _bulkCommands[BULK_SETTRIGGERANDSAMPLERATE].pending = true;
                break;
            }
            case BULK_CSETTRIGGERORSAMPLERATE: {
                // SetTrigger2250 bulk command for trigger slope
                static_cast<BulkSetTrigger2250 *>(_bulkCommands[BULK_CSETTRIGGERORSAMPLERATE].cmd.get())->setTriggerSlope((uint8_t)slope);
                _bulkCommands[BULK_CSETTRIGGERORSAMPLERATE].pending = true;
                break;
            }
            case BULK_ESETTRIGGERORSAMPLERATE: {
                // SetTrigger5200 bulk command for trigger slope
                static_cast<BulkSetTrigger5200 *>(_bulkCommands[BULK_ESETTRIGGERORSAMPLERATE].cmd.get())->setTriggerSlope((uint8_t)slope);
                _bulkCommands[BULK_ESETTRIGGERORSAMPLERATE].pending = true;
                break;
            }
            default:
                return ErrorCode::ERROR_UNSUPPORTED;
        }

        _settings.trigger.slope = slope;
        return ErrorCode::ERROR_NONE;
    }

    double HantekDevice::updatePretriggerPosition(double position) {
        if(!isDeviceConnected())
            return -2;

        // All trigger positions are measured in samples
        unsigned int positionSamples = position * _settings.samplerate.current;
        unsigned int recordLength = _settings.samplerate.limits->recordLengths[_settings.recordLengthId];
        bool rollMode = recordLength == UINT_MAX;
        // Fast rate mode uses both channels
        if(_settings.samplerate.limits == &_specification.samplerate.multi)
            positionSamples /= _specification.channels;

        switch(_model.productID) {
            case 0x2150:
            case 0x2090: {
                // Calculate the position value (Start point depending on record length)
                unsigned int position = rollMode ? 0x1 : 0x7ffff - recordLength + positionSamples;

                // SetTriggerAndSamplerate bulk command for trigger position
                static_cast<BulkSetTriggerAndSamplerate *>(_bulkCommands[BULK_SETTRIGGERANDSAMPLERATE].cmd.get())->setTriggerPosition(position);
                _bulkCommands[BULK_SETTRIGGERANDSAMPLERATE].pending = true;

                break;
            }
            case 0x2250: {
                // Calculate the position values (Inverse, maximum is 0x7ffff)
                unsigned int positionPre = 0x7ffff - recordLength + positionSamples;
                unsigned int positionPost = 0x7ffff - positionSamples;

                // SetBuffer2250 bulk command for trigger position
                BulkSetBuffer2250 *commandSetBuffer2250 = static_cast<BulkSetBuffer2250 *>(_bulkCommands[BULK_FSETBUFFER].cmd.get());
                commandSetBuffer2250->setTriggerPositionPre(positionPre);
                commandSetBuffer2250->setTriggerPositionPost(positionPost);
                _bulkCommands[BULK_FSETBUFFER].pending = true;

                break;
            }
            case 0x520A:
            case 0x5200: {
                // Calculate the position values (Inverse, maximum is 0xffff)
                unsigned short int positionPre = 0xffff - recordLength + positionSamples;
                unsigned short int positionPost = 0xffff - positionSamples;

                // SetBuffer5200 bulk command for trigger position
                BulkSetBuffer5200 *commandSetBuffer5200 = static_cast<BulkSetBuffer5200 *>(_bulkCommands[BULK_DSETBUFFER].cmd.get());
                commandSetBuffer5200->setTriggerPositionPre(positionPre);
                commandSetBuffer5200->setTriggerPositionPost(positionPost);
                _bulkCommands[BULK_DSETBUFFER].pending = true;

                break;
            }
            default:
                return (double) ErrorCode::ERROR_UNSUPPORTED; /// \todo error return
        }

        _settings.trigger.position = position;
        return (double) positionSamples / _settings.samplerate.current;
    }

    int HantekDevice::forceTrigger() {
        _bulkCommands[BULK_FORCETRIGGER].pending = true;
        return 0;
    }
}
