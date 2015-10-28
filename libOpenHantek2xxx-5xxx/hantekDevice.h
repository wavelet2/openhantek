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

#include "protocol.h"
#include "usbCommunication.h"
#include "usbCommunicationQueues.h"
#include "deviceBase.h"
#include "errorcodes.h"

namespace Hantek {

//////////////////////////////////////////////////////////////////////////////
/// Implementation of a DSO DeviceBase for Hantek USB DSO DSO-20xx, DSO-21xx, DSO-22xx, DSO-52xx
class HantekDevice : public DSO::DeviceBase, public DSO::CommunicationThreadQueues {
    public:
        HantekDevice(std::unique_ptr<DSO::USBCommunication> device);
        ~HantekDevice();

        /// Implemented methods from base classes

        virtual ErrorCode setChannelUsed(unsigned int channel, bool used) override;
        virtual ErrorCode setCoupling(unsigned int channel, DSO::Coupling coupling) override;
        virtual ErrorCode setGain(unsigned int channel, double gain) override;
        virtual ErrorCode setOffset(unsigned int channel, double offset) override;
        virtual ErrorCode setTriggerSource(bool special, unsigned int id) override;
        virtual ErrorCode setTriggerLevel(unsigned int channel, double level) override;
        virtual ErrorCode setTriggerSlope(DSO::Slope slope) override;
        virtual double updatePretriggerPosition(double position) override;
        virtual double computeBestSamplerate(double samplerate, bool fastRate, bool maximum, unsigned int *downsampler) override;
        virtual unsigned int updateRecordLength(unsigned int index) override;
        virtual unsigned int updateSamplerate(unsigned int downsampler, bool fastRate) override;
        virtual int forceTrigger();

        virtual unsigned getUniqueID() const override;

        virtual bool needFirmware() const override;
        virtual ErrorCode uploadFirmware() override;

        virtual bool isDeviceConnected() const override;
        virtual void connectDevice() override;
        virtual void disconnectDevice() override;
    private:
        //////////////////////////////////////////////////////////////////////////////
        /// \enum RollState
        /// \brief The states of the roll cycle (Since capture state isn't valid).
        enum class RollState {
                ROLL_STARTSAMPLING = 0, ///< Start sampling
                ROLL_ENABLETRIGGER = 1, ///< Enable triggering
                ROLL_FORCETRIGGER = 2, ///< Force triggering
                ROLL_GETDATA = 3, ///< Request sample data
                ROLL_COUNT
        };

        //////////////////////////////////////////////////////////////////////////////
        /// \enum ControlIndex
        /// \brief The array indices for possible control commands.
        enum ControlIndex {
                CONTROLINDEX_SETOFFSET,
                CONTROLINDEX_SETRELAYS,
                CONTROLINDEX_COUNT
        };

        /// \brief Handles all USB communication and sampling until the device gets disconnected.
        void run();
        bool runRollmode(RollState& rollState, int& samplingStarted);
        bool runStandardMode(CaptureState& captureState, int& cycleCounter, int& startCycle, int timerIntervall, int& samplingStarted);

        /**
         * @brief Sends a bulk command. _device->bulkWrite cannot be called
         * directly, because a usb control sequence has to be send before each bulk request.
         * This can only be done in the sample thread (in run()).
         * @param command
         * @return Return an usb error code.
         */
        int bulkCommand(TransferBuffer* command);
        ControlBeginCommand *beginCommandControl = new ControlBeginCommand();

        /// \brief Gets the current state.
        /// This is done in the sample thread (in run())
        /// \return The current CaptureState of the oscilloscope, libusb error code on error.
        int readCaptureState();

        /// \brief Gets sample data from the oscilloscope and converts it.
        /// This is done in the sample thread (in run())
        /// \return sample count on success, libusb error code on error.
        int readSamples(bool process);

        /// The USB device for the oscilloscope
        std::unique_ptr<DSO::USBCommunication> _device;
        std::unique_ptr<std::thread> _thread;

        /// USB device has been disconnected. This will be called if disconnectDevice() is issued before
        /// or if an usb error occured or the device has been plugged out.
        void deviceDisconnected();
};

}
