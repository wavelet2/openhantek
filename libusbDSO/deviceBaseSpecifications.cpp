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

#include "deviceBaseSpecifications.h"

namespace DSO {

DeviceBaseSpecifications::DeviceBaseSpecifications(const DSODeviceDescription& model) : _model(model) {
    // Use DSO-2090 specification as default
    _specification.samplerate.single.base = 50e6;
    _specification.samplerate.single.max = 50e6;
    _specification.samplerate.single.recordLengths.push_back(0);
    _specification.samplerate.multi.base = 100e6;
    _specification.samplerate.multi.max = 100e6;
    _specification.samplerate.multi.recordLengths.push_back(0);

    for(unsigned int channel = 0; channel < _specification.channels; ++channel) {
        for(unsigned int gainId = 0; gainId < 9; ++gainId) {
            _specification.limits[channel].offset[gainId][OFFSET_START] = 0x0000;
            _specification.limits[channel].offset[gainId][OFFSET_END] = 0xffff;
        }
    }

    // Set settings to default values
    _settings.samplerate.limits = &(_specification.samplerate.single);
    _settings.samplerate.downsampler = 1;
    _settings.samplerate.current = 1e8;
    _settings.trigger.position = 0;
    _settings.trigger.point = 0;
    _settings.trigger.mode = TriggerMode::TRIGGERMODE_NORMAL;
    _settings.trigger.slope = Slope::SLOPE_POSITIVE;
    _settings.trigger.special = false;
    _settings.trigger.source = 0;
    for(unsigned channel = 0; channel < _specification.channels; ++channel) {
        _settings.trigger.level[channel] = 0.0;
        _settings.voltage[channel].gain = 0;
        _settings.voltage[channel].offset = 0.0;
        _settings.voltage[channel].offsetReal = 0.0;
        _settings.voltage[channel].used = false;
    }
    _settings.recordLengthId = 1;
    _settings.usedChannels = 0;
}

unsigned DeviceBaseSpecifications::getChannelCount() {
    return _specification.channels;
}

}
