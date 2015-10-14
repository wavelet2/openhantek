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

#include <vector>

#include "dsoSettings.h"
#include "dsoSpecification.h"
#include "deviceDescriptionEntry.h"

namespace DSO {

//////////////////////////////////////////////////////////////////////////////
/// \brief Part of the base class for an DSO device implementation. All settings,
///        specifications and information about the target device are here.
class DeviceBaseSpecifications {
public:
    DeviceBaseSpecifications(const DSODeviceDescription& model);
    const DSODeviceDescription& getModel() const { return _model; }

protected:
    /// The specifications of the device
    dsoSpecification _specification;
    /// The current settings of the device
    dsoSettings _settings;

    /// The description of the device (name, deviceID, productID)
    const DSODeviceDescription _model;
};

}
