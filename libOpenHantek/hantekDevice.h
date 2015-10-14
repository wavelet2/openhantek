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

#include "utils/containerStream.h"

#include "dsoSettings.h"
#include "dsoSpecification.h"
#include "usbCommunication.h"
#include "protocol.h"
#include "deviceBase.h"
#include "deviceDescriptionEntry.h"
#include "errorcodes.h"

class libusb_device;

namespace Hantek {

//////////////////////////////////////////////////////////////////////////////
/// Implementation of a DSO DeviceBase for Hantek USB DSO DSO-20xx, DSO-21xx, DSO-22xx, DSO-52xx
class HantekDevice : public DSO::DeviceBase {
    public:
        HantekDevice(libusb_device *device, const DSO::DSODeviceDescription& model);
        ~HantekDevice();

        /// Implemented methods from base classes

        virtual ErrorCode setChannelUsed(unsigned int channel, bool used);
        virtual ErrorCode setCoupling(unsigned int channel, DSO::Coupling coupling);
        virtual double setGain(unsigned int channel, double gain);
        virtual double setOffset(unsigned int channel, double offset);
        virtual ErrorCode setTriggerSource(bool special, unsigned int id);
        virtual double setTriggerLevel(unsigned int channel, double level);
        virtual ErrorCode setTriggerSlope(DSO::Slope slope);
        virtual double setPretriggerPosition(double position);
        virtual double computeBestSamplerate(double samplerate, bool fastRate, bool maximum, unsigned int *downsampler);
        virtual unsigned int updateRecordLength(unsigned int index);
        virtual unsigned int updateSamplerate(unsigned int downsampler, bool fastRate);
        virtual int forceTrigger();

        virtual unsigned getUniqueID();

        virtual bool isDeviceConnected();
        virtual void connectDevice();
        virtual void disconnectDevice();
    protected:
        int getCommunicationPacketSize() { return _device.getPacketSize(); }
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
        /// \brief The array indices for the waiting control commands.
        enum ControlIndex {
                //CONTROLINDEX_VALUE,
                //CONTROLINDEX_GETSPEED,
                //CONTROLINDEX_BEGINCOMMAND,
                CONTROLINDEX_SETOFFSET,
                CONTROLINDEX_SETRELAYS,
                CONTROLINDEX_COUNT
        };

        /// \brief Handles all USB things until the device gets disconnected.
        void run();
        /// Send all pending bulk commands
        bool sendPendingBulkCommands();
        /// Send all pending control commands
        bool sendPendingControlCommands();
        bool runRollmode(RollState& rollState, int& samplingStarted);
        bool runStandardMode(CaptureState& captureState, int& cycleCounter, int& startCycle, int timerIntervall, int& samplingStarted);

        int bulkCommand(TransferBuffer* command);
        ControlBeginCommand *beginCommandControl = new ControlBeginCommand();

        /// \brief Gets the current state.
        /// \return The current CaptureState of the oscilloscope, libusb error code on error.
        int readCaptureState();

        /// \brief Gets sample data from the oscilloscope and converts it.
        /// \return sample count on success, libusb error code on error.
        int readSamples(bool process);

        /// The USB device for the oscilloscope
        DSO::USBCommunication _device;
        std::unique_ptr<std::thread> _thread;

        void deviceDisconnected();

        /// Command interface
        dsoSpecificationCommands specificationCommands;
};

}