////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \brief Declares the Hantek::HantekDevice class.
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

#include "usbCommunication.h"
#include "usbCommunicationQueues.h"
#include "deviceBase.h"
#include "errorcodes.h"
#include "protocol.h"

class libusb_device;

namespace Hantek {

//////////////////////////////////////////////////////////////////////////////
/// Implementation of a DSO DeviceBase for Hantek USB DSO DSO-60xx
class HantekDevice : public DSO::DeviceBase, public DSO::CommunicationThreadQueues  {
    public:
        HantekDevice(std::unique_ptr<DSO::USBCommunication> device);
        ~HantekDevice();

        /// Implemented methods from base classes

        virtual ErrorCode setChannelUsed(unsigned int channel, bool used);
        virtual ErrorCode setCoupling(unsigned int channel, DSO::Coupling coupling);
        virtual ErrorCode setGain(unsigned int channel, double gain);
        virtual ErrorCode setOffset(unsigned int channel, double offset);
        virtual ErrorCode setTriggerSource(bool special, unsigned int id);
        virtual ErrorCode setTriggerLevel(unsigned int channel, double level);
        virtual ErrorCode setTriggerSlope(DSO::Slope slope);
        virtual double updatePretriggerPosition(double position);
        virtual double computeBestSamplerate(double samplerate, bool fastRate, bool maximum, unsigned int *downsampler);
        virtual unsigned int updateRecordLength(unsigned int index);
        virtual unsigned int updateSamplerate(unsigned int downsampler, bool fastRate);
        virtual int forceTrigger();

        virtual unsigned getUniqueID();

        virtual bool needFirmware();
        virtual ErrorCode uploadFirmware();

        virtual bool isDeviceConnected();
        virtual void connectDevice();
        virtual void disconnectDevice();
    private:
        std::unique_ptr<DSO::USBCommunication> _device;
        std::unique_ptr<std::thread> _thread;

        /// The DSO samples passes multiple buffers before it appears
        /// on screen. _data is the first one after the usb communication.
        HT6022_DataSize _dataSize = HT6022_DataSize::DS_128KB;
        unsigned char _data[(int)HT6022_DataSize::DS_1MB*2];

        //////////////////////////////////////////////////////////////////////////////
        /// \enum ControlIndex
        /// \brief The array indices for possible control commands.
        enum ControlIndex {
                CONTROLINDEX_SETOFFSET,
                CONTROLINDEX_SETRELAYS,
                CONTROLINDEX_COUNT
        };

        /// USB device has been disconnected. This will be called if disconnectDevice() is issued before
        /// or if an usb error occured or the device has been plugged out.
        void deviceDisconnected();

        /**
         * @brief Sends a bulk command. _device->bulkWrite cannot be called
         * directly, because a usb control sequence has to be send before each bulk request.
         * This can only be done in the sample thread (in run()).
         * @param command
         * @return Return an usb error code.
         */
        int readSamples();

        /// \brief Handles all USB communication and sampling until the device gets disconnected.
        void run();

};

}
