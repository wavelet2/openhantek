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

#include "deviceBase.h"

namespace DSO {
    unsigned int DeviceBase::calculateTriggerPoint(unsigned int value) const {
        unsigned int result = value;

        // Each set bit inverts all bits with a lower value
        for(unsigned int bitValue = 1; bitValue; bitValue <<= 1)
            if(result & bitValue)
                result ^= bitValue - 1;

        return result;
    }

    /// \brief Get a list of the names of the special trigger sources.
    const std::vector<std::string>& DeviceBase::getSpecialTriggerSources() const {
        return _specification.specialTriggerSources;
    }


    ErrorCode DeviceBase::setTriggerMode(TriggerMode mode) {
        if(!isDeviceConnected())
            return ErrorCode::ERROR_CONNECTION;

        if(mode < TriggerMode::TRIGGERMODE_AUTO || mode > TriggerMode::TRIGGERMODE_SINGLE)
            return ErrorCode::ERROR_PARAMETER;

        _settings.trigger.mode = mode;
        return ErrorCode::ERROR_NONE;
    }
}
