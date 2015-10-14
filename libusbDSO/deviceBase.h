////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \brief Declares the DSO::DeviceBase class.
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

#pragma once

#include <thread>
#include <functional>
#include <chrono>
#include <future>

#include "deviceBaseSamples.h"
#include "errorcodes.h"

namespace DSO {

//////////////////////////////////////////////////////////////////////////////
/// \brief The base class for an DSO device implementation. It inherits from all
///        deviceBase* classes (Commands, Trigger, Samples, Specifications).
class DeviceBase : public DeviceBaseSamples {
    public:
        DeviceBase(const DSODeviceDescription& model) : DeviceBaseSamples(model) {}

        /// \brief Enables/disables filtering of the given channel.
        /// \param channel The channel that should be set.
        /// \param used true if the channel should be sampled.
        /// \return See ::ErrorCode::ErrorCode.
        virtual ErrorCode setChannelUsed(unsigned int channel, bool used) = 0;

        /// \brief Set the coupling for the given channel.
        /// \param channel The channel that should be set.
        /// \param coupling The new coupling for the channel.
        /// \return See ::ErrorCode::ErrorCode.
        virtual ErrorCode setCoupling(unsigned int channel, Coupling coupling) = 0;

        /// \brief Sets the gain for the given channel.
        /// \param channel The channel that should be set.
        /// \param gain The gain that should be met (V/div).
        /// \return The gain that has been set, ::ErrorCode::ErrorCode on error.
        virtual double setGain(unsigned int channel, double gain) = 0;

        /// \brief Set the offset for the given channel.
        /// \param channel The channel that should be set.
        /// \param offset The new offset value (0.0 - 1.0).
        /// \return The offset that has been set, ::ErrorCode::ErrorCode on error.
        virtual double setOffset(unsigned int channel, double offset) = 0;

        /// \brief Set the trigger source.
        /// \param special true for a special channel (EXT, ...) as trigger source.
        /// \param id The number of the channel, that should be used as trigger.
        /// \return See ::ErrorCode::ErrorCode.
        virtual ErrorCode setTriggerSource(bool special, unsigned int id) = 0;

        /// \brief Set the trigger level.
        /// \param channel The channel that should be set.
        /// \param level The new trigger level (V).
        /// \return The trigger level that has been set, ::ErrorCode::ErrorCode on error.
        virtual double setTriggerLevel(unsigned int channel, double level) = 0;

        /// \brief Set the trigger slope.
        /// \param slope The Slope that should cause a trigger.
        /// \return See ::ErrorCode::ErrorCode.
        virtual ErrorCode setTriggerSlope(Slope slope) = 0;

        /// \brief Set the trigger position.
        /// \param position The new trigger position (in s).
        /// \return The trigger position that has been set.
        virtual double setPretriggerPosition(double position) = 0;

        /// \brief Forces a trigger
        /// \return The trigger position that has been set.
        virtual int forceTrigger() = 0;

        /// \brief Set the trigger mode.
        /// \return See ::ErrorCode::ErrorCode.
        ErrorCode setTriggerMode(TriggerMode mode);

        /// \brief Calculates the trigger point from the CommandGetCaptureState data.
        /// \param value The data value that contains the trigger point.
        /// \return The calculated trigger point for the given data.
        unsigned int calculateTriggerPoint(unsigned int value) const;

        const std::vector<std::string>& getSpecialTriggerSources() const;

        /// \brief A unique id that is important for the DeviceList to identify already
        /// connected devices. For usb devices this is the bus/slot number.
        virtual unsigned getUniqueID() = 0;

        /// \return Return true if a connection to the device (e.g. usb device) is established.
        virtual bool isDeviceConnected() = 0;

        /// \brief Try to connect to the oscilloscope. For usb devices this will open the usb interface.
        virtual void connectDevice() = 0;

        /// \brief Disconnect the oscilloscope.
        virtual void disconnectDevice() = 0;

    public:
    /**
     * This section contains callback methods. Register your function or class method to get notified
     * of events.
     */

        /// The oscilloscope device has been connected
        std::function<void(void)> _deviceConnected = [](){};
        /// The oscilloscope device has been disconnected
        std::function<void(void)> _deviceDisconnected = [](){};
        /// Status message about the oscilloscope (int messageID, int timeout)
        std::function<void(int,int)> _statusMessage = [](int,int){};

protected:
    TriggerMode lastTriggerMode = TriggerMode::TRIGGERMODE_UNDEFINED;

};

}