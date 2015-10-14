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


#include <cmath>
#include <limits>
#include <vector>
#include <iostream>
#include <memory>
#include <climits>
#include <cstring>

#include <libusb-1.0/libusb.h>

#include "hantekDevice.h"
#include "protocol.h"
#include "utils/timestampDebug.h"
#include "utils/stdStringSplit.h"

namespace Hantek {
std::vector<unsigned short int>& operator<<(std::vector<unsigned short int>& v, unsigned short int x);
std::vector<unsigned char>& operator<<(std::vector<unsigned char>& v, unsigned char x);
std::vector<unsigned>& operator<<(std::vector<unsigned>& v, unsigned x);
std::vector<double>& operator<<(std::vector<double>& v, double x);


/// \brief Initializes the command buffers and lists.
/// \param parent The parent widget.
HantekDevice::HantekDevice(libusb_device *device, const DSO::DSODeviceDescription& model)
    : DeviceBase(model), _device(device, model, std::bind( &HantekDevice::deviceDisconnected, this)) {
}

/// \brief Disconnects the device.
HantekDevice::~HantekDevice() {
    _device.disconnect();
}

unsigned HantekDevice::getUniqueID() {
    return _device.getUniqueID();
}

void HantekDevice::deviceDisconnected() {
    if (_thread->joinable()) _thread->join();
    _thread.reset();
}

void HantekDevice::disconnectDevice() {
    _device.disconnect();
}

bool HantekDevice::isDeviceConnected() {
    return _device.isConnected();
}

bool HantekDevice::sendPendingBulkCommands() {
    int errorCode = 0;

    for(int index = 0; index < BULK_COUNT; ++index) {
        if(!_commandPending[index])
            continue;

        timestampDebug("Sending bulk command: " << hexDump(_command[index]->data(), _command[index]->size()));

        errorCode = bulkCommand(_command[index].get());
        if(errorCode < 0) {
            std::cerr << "Sending bulk command %02x failed: " << index << " " <<
            libusb_error_name((libusb_error)errorCode) << " " <<
                libusb_strerror((libusb_error)errorCode) << std::endl;

            if(errorCode == LIBUSB_ERROR_NO_DEVICE)
                return false;
        }
        else
            _commandPending[index] = false;
    }
    return true;
}

bool HantekDevice::sendPendingControlCommands() {
    int errorCode = 0;

    for(Control& control: _controlCommands) {
        if(!control.controlPending)
            continue;

        timestampDebug("Sending control command " << control.controlCode << " " << hexDump(control.control->data(), control.control->size()));

        errorCode = _device.controlWrite(control.controlCode, control.control->data(), control.control->size());
        if(errorCode < 0) {
            std::cerr << "Sending control command failed: " << control.controlCode << " " <<
                libusb_error_name((libusb_error)errorCode) << " " <<
                libusb_strerror((libusb_error)errorCode) << std::endl;

            if(errorCode == LIBUSB_ERROR_NO_DEVICE)
                return false;
        }
        else
            control.controlPending = false;
    }
    return true;
}

bool HantekDevice::runRollmode(RollState& rollState, int& samplingStarted) {
    bool toNextState = true;
    int errorCode = 0;

    switch(rollState) {
        case RollState::ROLL_STARTSAMPLING:
            // Don't iterate through roll mode steps when stopped
            if(!_sampling) {
                toNextState = false;
                break;
            }

            // Sampling hasn't started, update the expected sample count
            _previousSampleCount = getSampleCount();

            errorCode = bulkCommand(_command[BULK_STARTSAMPLING].get());
            if(errorCode < 0) {
                if(errorCode == LIBUSB_ERROR_NO_DEVICE)
                    return false;
                break;
            }

            timestampDebug("Starting to capture");
            samplingStarted = true;
            break;
        case RollState::ROLL_ENABLETRIGGER:
            errorCode = bulkCommand(_command[BULK_ENABLETRIGGER].get());
            if(errorCode < 0) {
                if(errorCode == LIBUSB_ERROR_NO_DEVICE)
                    return false;
                break;
            }
            timestampDebug("Enabling trigger");
            break;
        case RollState::ROLL_FORCETRIGGER:
            errorCode = bulkCommand(_command[BULK_FORCETRIGGER].get());
            if(errorCode < 0) {
                if(errorCode == LIBUSB_ERROR_NO_DEVICE)
                    return false;
                break;
            }
            timestampDebug("Forcing trigger");
            break;

        case RollState::ROLL_GETDATA:
            // Get data and process it, if we're still sampling
            errorCode = readSamples(samplingStarted);
            if(errorCode < 0)
                std::cerr << "Getting sample data failed: " <<
                libusb_error_name((libusb_error)errorCode) << " " <<
                libusb_strerror((libusb_error)errorCode) << std::endl;
            else
                timestampDebug("Received " << errorCode << " B of sampling data");

            // Check if we're in single trigger mode
            if(_settings.trigger.mode == DSO::TriggerMode::TRIGGERMODE_SINGLE && samplingStarted)
                stopSampling();

            // Sampling completed, restart it when necessary
            samplingStarted = false;
            break;
        default:
            timestampDebug("Roll mode state unknown");
            break;
    }

    // Go to next state, or restart if last state was reached
    if(toNextState)
        rollState = (RollState) (((int)rollState + 1) % (int)RollState::ROLL_COUNT);

    return true;
}

bool HantekDevice::runStandardMode(CaptureState& captureState, int& cycleCounter, int& startCycle, int timerIntervall, int& samplingStarted) {
    int errorCode = readCaptureState();

    if(errorCode < 0) {
        std::cerr <<"Getting capture state failed: " <<
                    libusb_error_name((libusb_error)errorCode) <<
                    libusb_strerror((libusb_error)errorCode) << std::endl;
        return false;
    }

    CaptureState lastCaptureState = captureState;
    captureState = (CaptureState)errorCode;

    if(captureState != lastCaptureState)
        timestampDebug("Capture state changed to " << captureState);

    switch(captureState) {
        case CAPTURE_READY:
        case CAPTURE_READY2250:
        case CAPTURE_READY5200:
            // Get data and process it, if we're still sampling
            errorCode = readSamples(samplingStarted);
            if(errorCode < 0)
                std::cerr << "Getting sample data failed: " <<
                libusb_error_name((libusb_error)errorCode) << " " <<
                libusb_strerror((libusb_error)errorCode) << std::endl;
            else
                timestampDebug("Received "<< errorCode << " B of sampling data");

            // Check if we're in single trigger mode
            if(_settings.trigger.mode == DSO::TriggerMode::TRIGGERMODE_SINGLE && samplingStarted)
                    stopSampling();

            // Sampling completed, restart it when necessary
            samplingStarted = false;

            // Start next capture if necessary by leaving out the break statement
            if(!_sampling)
                break;

        case CAPTURE_WAITING:
            // Sampling hasn't started, update the expected sample count
            _previousSampleCount = getSampleCount();

            if(samplingStarted && lastTriggerMode == _settings.trigger.mode) {
                ++cycleCounter;

                if(cycleCounter == startCycle && !isRollingMode()) {
                    // Buffer refilled completely since start of sampling, enable the trigger now
                    errorCode = bulkCommand(_command[BULK_ENABLETRIGGER].get());
                    if(errorCode < 0) {
                        if(errorCode == LIBUSB_ERROR_NO_DEVICE)
                            return false;
                        break;
                    }

                    timestampDebug("Enabling trigger");

                }
                else if(cycleCounter >= 8 + startCycle && _settings.trigger.mode == DSO::TriggerMode::TRIGGERMODE_AUTO) {
                    // Force triggering
                    errorCode = bulkCommand(_command[BULK_FORCETRIGGER].get());
                    if(errorCode < 0) {
                        if(errorCode == LIBUSB_ERROR_NO_DEVICE)
                            return false;
                        break;
                    }
                    timestampDebug("Forcing trigger");
                }

                if(cycleCounter < 20 || cycleCounter < 4000 / timerIntervall)
                    break;
            }

            // Start capturing
            errorCode = bulkCommand(_command[BULK_STARTSAMPLING].get());
            if(errorCode < 0) {
                if(errorCode == LIBUSB_ERROR_NO_DEVICE)
                    return false;
                break;
            }

            timestampDebug("Starting to capture");

            samplingStarted = true;
            cycleCounter = 0;
            startCycle = _settings.trigger.position * 1000 / timerIntervall + 1;
            lastTriggerMode = _settings.trigger.mode;
            break;

        case CAPTURE_SAMPLING:
            break;
        default:
            break;
    }
    return true;
}

void HantekDevice::run() {
    // Initialize usb communication thread state
    CaptureState captureState = CAPTURE_WAITING;
    RollState rollState       = RollState::ROLL_STARTSAMPLING;
    int samplingStarted       = false;
    lastTriggerMode           = DSO::TriggerMode::TRIGGERMODE_UNDEFINED;
    int cycleCounter          = 0;
    int startCycle            = 0;

    while (1) {
        if (!sendPendingBulkCommands()) break;
        if (!sendPendingControlCommands()) break;

        // Compute sleep time
        int cycleTime;

        // Check the current oscilloscope state everytime 25% of the time the buffer should be refilled
        if(isRollingMode())
            cycleTime = (int) ((double) _device.getPacketSize() / ((_settings.samplerate.limits == &_specification.samplerate.multi) ? 1 : _specification.channels) / _settings.samplerate.current * 250);
        else
            cycleTime = (int) ((double) _settings.samplerate.limits->recordLengths[_settings.recordLengthId] / _settings.samplerate.current * 250);

        // Not more often than every 10 ms though but at least once every second
        cycleTime = std::max(std::min(10, cycleTime), 1000);

        // State machine for the device communication
        if(isRollingMode()) {
            captureState = CAPTURE_WAITING;
            if (!runRollmode(rollState, samplingStarted)) break;
        } else {
            rollState = RollState::ROLL_STARTSAMPLING;
            if (!runStandardMode(captureState, cycleCounter, startCycle, cycleTime, samplingStarted)) break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(cycleTime));
    }

    _device.disconnect();
    _statusMessage(LIBUSB_ERROR_NO_DEVICE, 0);
}

int HantekDevice::readCaptureState() {
    int errorCode;

    errorCode = bulkCommand(_command[BULK_GETCAPTURESTATE].get());
    if(errorCode < 0)
        return errorCode;

    BulkResponseGetCaptureState response;
    errorCode = _device.bulkRead(response.data(), response.size());
    if(errorCode < 0)
        return errorCode;

    _settings.trigger.point = calculateTriggerPoint(response.getTriggerPoint());

    return (int) response.getCaptureState();
}

int HantekDevice::readSamples(bool process) {
    int errorCode;

    // Request data
    errorCode = bulkCommand(_command[BULK_GETDATA].get());
    if(errorCode < 0)
        return errorCode;

    // Save raw data to temporary buffer
    bool fastRate = false;
    unsigned int totalSampleCount = getSampleCount(&fastRate);
    if(totalSampleCount == UINT_MAX)
        return LIBUSB_ERROR_INVALID_PARAM;

    // To make sure no samples will remain in the scope buffer, also check the sample count before the last sampling started
    if(totalSampleCount < _previousSampleCount) {
        unsigned int currentSampleCount = totalSampleCount;
        totalSampleCount = _previousSampleCount;
        _previousSampleCount = currentSampleCount; // Using sampleCount as temporary buffer since it was set to totalSampleCount
    }
    else {
        _previousSampleCount = totalSampleCount;
    }

    unsigned int dataLength = totalSampleCount;
    if(_specification.sampleSize > 8)
        dataLength *= 2;

    unsigned char data[dataLength];

    errorCode = _device.bulkReadMulti(data, dataLength);
    if(errorCode < 0)
            return errorCode;
    dataLength = errorCode; // actual data read

    // Process the data only if we want it
    if(process) {
        _samplesMutex.lock();
        processSamples(data, dataLength, totalSampleCount, fastRate);
        _samplesMutex.unlock();
        _samplesAvailable(&_samples,
                            _settings.samplerate.current,
                            isRollingMode(),
                            _samplesMutex);
    }

    return errorCode;
}

void HantekDevice::connectDevice() {
    _statusMessage(_device.connect(), 0);
    if(!_device.isConnected())
            return;

    // Instantiate bulk command later, some are not the same for all models
    _command.resize(BULK_COUNT);
    _commandPending.resize(BULK_COUNT);
    for(int index = 0; index < BULK_COUNT; ++index) {
        _command[index].reset();
        _commandPending[index] = false;
    }

    _controlCommands.clear();
    _controlCommands.resize(CONTROLINDEX_COUNT);
    _controlCommands.push_back({std::unique_ptr<TransferBuffer>(new ControlSetOffset()), CONTROL_SETOFFSET, false});
    _controlCommands.push_back({std::unique_ptr<TransferBuffer>(new ControlSetRelays()), CONTROL_SETRELAYS, false});

    // Instantiate the _commands needed for all models
    _command[BULK_FORCETRIGGER].reset(new BulkForceTrigger());
    _command[BULK_STARTSAMPLING].reset(new BulkCaptureStart());
    _command[BULK_ENABLETRIGGER].reset(new BulkTriggerEnabled());
    _command[BULK_GETDATA].reset(new BulkGetData());
    _command[BULK_GETCAPTURESTATE].reset(new BulkGetCaptureState());
    _command[BULK_SETGAIN].reset(new BulkSetGain());
    // Initialize the _command versions to the ones used on the DSO-2090
    specificationCommands.bulk.setRecordLength = (BulkCode) -1;
    specificationCommands.bulk.setChannels = (BulkCode) -1;
    specificationCommands.bulk.setGain = BULK_SETGAIN;
    specificationCommands.bulk.setSamplerate = (BulkCode) -1;
    specificationCommands.bulk.setTrigger = (BulkCode) -1;
    specificationCommands.bulk.setPretrigger = (BulkCode) -1;
    specificationCommands.control.setOffset = CONTROL_SETOFFSET;
    specificationCommands.control.setRelays = CONTROL_SETRELAYS;
    specificationCommands.values.offsetLimits = VALUE_OFFSETLIMITS;
    specificationCommands.values.voltageLimits = (ControlValue) -1;

    // Determine the _command version we need for this model
    bool unsupported = false;
    switch(_model.productID) {
        case 0x2150:
            unsupported = true;

        case 0x2090:
            // Instantiate additional _commands for the DSO-2090
            _command[BULK_SETTRIGGERANDSAMPLERATE].reset(new BulkSetTriggerAndSamplerate());
            specificationCommands.bulk.setRecordLength = BULK_SETTRIGGERANDSAMPLERATE;
            specificationCommands.bulk.setChannels = BULK_SETTRIGGERANDSAMPLERATE;
            specificationCommands.bulk.setSamplerate = BULK_SETTRIGGERANDSAMPLERATE;
            specificationCommands.bulk.setTrigger = BULK_SETTRIGGERANDSAMPLERATE;
            specificationCommands.bulk.setPretrigger = BULK_SETTRIGGERANDSAMPLERATE;
            // Initialize those as pending
            _commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
            break;

        case 0x2250:
            // Instantiate additional _commands for the DSO-2250
            _command[BULK_BSETCHANNELS].reset(new BulkSetChannels2250());
            _command[BULK_CSETTRIGGERORSAMPLERATE].reset(new BulkSetTrigger2250());
            _command[BULK_DSETBUFFER].reset(new BulkSetRecordLength2250());
            _command[BULK_ESETTRIGGERORSAMPLERATE].reset(new BulkSetSamplerate2250());
            _command[BULK_FSETBUFFER].reset(new BulkSetBuffer2250());
            specificationCommands.bulk.setRecordLength = BULK_DSETBUFFER;
            specificationCommands.bulk.setChannels = BULK_BSETCHANNELS;
            specificationCommands.bulk.setSamplerate = BULK_ESETTRIGGERORSAMPLERATE;
            specificationCommands.bulk.setTrigger = BULK_CSETTRIGGERORSAMPLERATE;
            specificationCommands.bulk.setPretrigger = BULK_FSETBUFFER;

            _commandPending[BULK_BSETCHANNELS] = true;
            _commandPending[BULK_CSETTRIGGERORSAMPLERATE] = true;
            _commandPending[BULK_DSETBUFFER] = true;
            _commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;
            _commandPending[BULK_FSETBUFFER] = true;

            break;

        case 0x520A:
            unsupported = true;

        case 0x5200:
            // Instantiate additional _commands for the DSO-5200
            _command[BULK_CSETTRIGGERORSAMPLERATE].reset(new BulkSetSamplerate5200());
            _command[BULK_DSETBUFFER].reset(new BulkSetBuffer5200());
            _command[BULK_ESETTRIGGERORSAMPLERATE].reset(new BulkSetTrigger5200());
            specificationCommands.bulk.setRecordLength = BULK_DSETBUFFER;
            specificationCommands.bulk.setChannels = BULK_ESETTRIGGERORSAMPLERATE;
            specificationCommands.bulk.setSamplerate = BULK_CSETTRIGGERORSAMPLERATE;
            specificationCommands.bulk.setTrigger = BULK_ESETTRIGGERORSAMPLERATE;
            specificationCommands.bulk.setPretrigger = BULK_ESETTRIGGERORSAMPLERATE;
            //command.values.voltageLimits = VALUE_ETSCORRECTION;

            _commandPending[BULK_CSETTRIGGERORSAMPLERATE] = true;
            _commandPending[BULK_DSETBUFFER] = true;
            _commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;

            break;

        default:
            _device.disconnect();
            _statusMessage(10000, 0);
            return;
    }

    if(unsupported)
        std::cerr <<"Warning: This Hantek DSO model isn't supported officially, so it may not be working as expected. Reports about your experiences are very welcome though (Please open a feature request in the tracker at http://www.github.com/openhantek/openhantek). If it's working perfectly this warning can be removed, if not it should be possible to get it working with your help soon." << std::endl;

    for(Control& control: _controlCommands)
        control.controlPending = true;

    // Maximum possible samplerate for a single channel and dividers for record lengths
    _specification.bufferDividers.clear();
    _specification.samplerate.single.recordLengths.clear();
    _specification.samplerate.multi.recordLengths.clear();
    _specification.gainSteps.clear();
    for(unsigned channel = 0; channel < _specification.channels; ++channel)
        _specification.limits[channel].voltage.clear();

    switch(_model.productID) {
        case 0x5200:
        case 0x520A:
            _specification.samplerate.single.base = 100e6;
            _specification.samplerate.single.max = 125e6;
            _specification.samplerate.single.maxDownsampler = 131072;
            _specification.samplerate.single.recordLengths << UINT_MAX << 10240 << 14336;
            _specification.samplerate.multi.base = 200e6;
            _specification.samplerate.multi.max = 250e6;
            _specification.samplerate.multi.maxDownsampler = 131072;
            _specification.samplerate.multi.recordLengths << UINT_MAX << 20480 << 28672;
            _specification.bufferDividers << 1000 << 1 << 1;
            _specification.sampleSize = 10;
            _specification.gainSteps << 0.16 << 0.40 << 0.80 << 1.60 << 4.00 <<  8.0 << 16.0 << 40.0 << 80.0;
            _specification.gainIndex <<    1 <<    0 <<    0 <<    1 <<    0 <<    0 <<    1 <<    0 <<    0;

            /// \todo Use calibration data to get the DSO-5200(A) sample ranges
            for(unsigned channel = 0; channel < _specification.channels; ++channel)
                _specification.limits[channel].voltage
                                    <<  368 <<  454 <<  908 <<  368 <<  454 <<  908 <<  368 <<  454 <<  908;

            break;

        case 0x2250:
            _specification.samplerate.single.base = 100e6;
            _specification.samplerate.single.max = 100e6;
            _specification.samplerate.single.maxDownsampler = 65536;
            _specification.samplerate.single.recordLengths << UINT_MAX << 10240 << 524288;
            _specification.samplerate.multi.base = 200e6;
            _specification.samplerate.multi.max = 250e6;
            _specification.samplerate.multi.maxDownsampler = 65536;
            _specification.samplerate.multi.recordLengths << UINT_MAX << 20480 << 1048576;
            _specification.bufferDividers << 1000 << 1 << 1;
            _specification.sampleSize = 8;
            _specification.gainSteps << 0.08 << 0.16 << 0.40 << 0.80 << 1.60 << 4.00 <<  8.0 << 16.0 << 40.0;
            _specification.gainIndex <<    0 <<    2 <<    3 <<    0 <<    2 <<    3 <<    0 <<    2 <<    3;

            for(unsigned channel = 0; channel < _specification.channels; ++channel)
                _specification.limits[channel].voltage
                                    <<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255;
            break;

        case 0x2150:
            _specification.samplerate.single.base = 50e6;
            _specification.samplerate.single.max = 75e6;
            _specification.samplerate.single.maxDownsampler = 131072;
            _specification.samplerate.single.recordLengths << UINT_MAX << 10240 << 32768;
            _specification.samplerate.multi.base = 100e6;
            _specification.samplerate.multi.max = 150e6;
            _specification.samplerate.multi.maxDownsampler = 131072;
            _specification.samplerate.multi.recordLengths << UINT_MAX << 20480 << 65536;
            _specification.bufferDividers << 1000 << 1 << 1;
            _specification.sampleSize = 8;
            _specification.gainSteps << 0.08 << 0.16 << 0.40 << 0.80 << 1.60 << 4.00 <<  8.0 << 16.0 << 40.0;
            _specification.gainIndex <<    0 <<    1 <<    2 <<    0 <<    1 <<    2 <<    0 <<    1 <<    2;

            for(unsigned channel = 0; channel < _specification.channels; ++channel)
                _specification.limits[channel].voltage
                                    <<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255;
            break;

        default:
            _specification.samplerate.single.base = 50e6;
            _specification.samplerate.single.max = 50e6;
            _specification.samplerate.single.maxDownsampler = 131072;
            _specification.samplerate.single.recordLengths << UINT_MAX << 10240 << 32768;
            _specification.samplerate.multi.base = 100e6;
            _specification.samplerate.multi.max = 100e6;
            _specification.samplerate.multi.maxDownsampler = 131072;
            _specification.samplerate.multi.recordLengths << UINT_MAX << 20480 << 65536;
            _specification.bufferDividers << 1000 << 1 << 1;
            _specification.sampleSize = 8;
            _specification.gainSteps << 0.08 << 0.16 << 0.40 << 0.80 << 1.60 << 4.00 <<  8.0 << 16.0 << 40.0;
            _specification.gainIndex <<    0 <<    1 <<    2 <<    0 <<    1 <<    2 <<    0 <<    1 <<    2;

            for(unsigned channel = 0; channel < _specification.channels; ++channel)
                _specification.limits[channel].voltage
                                    <<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255;
            break;
    }
    _previousSampleCount = 0;

    // Get channel level data
    unsigned short int offsetLimit[_specification.channels][9][DSO::OFFSET_COUNT];
    int errorCode = _device.controlRead(CONTROL_VALUE, (unsigned char *) &(offsetLimit),
                                       sizeof(offsetLimit), (int) VALUE_OFFSETLIMITS);
    if(errorCode < 0) {
        _device.disconnect();
        _statusMessage(errorCode, 0);
        return;
    }

    for (unsigned c=0; c < _specification.channels; ++c)
        memcpy(_specification.limits[c].offset, offsetLimit[c], 9*DSO::OFFSET_COUNT);

    // _signals for initial _settings
    _availableRecordLengthsChanged(_settings.samplerate.limits->recordLengths);
    updateSamplerateLimits();
    _recordLengthChanged(_settings.samplerate.limits->recordLengths[_settings.recordLengthId]);
    if(!isRollingMode())
        _recordTimeChanged((double) _settings.samplerate.limits->recordLengths[_settings.recordLengthId] / _settings.samplerate.current);
    _samplerateChanged(_settings.samplerate.current);

    _sampling = false;
    // The control loop is running until the device is disconnected
    _thread = std::unique_ptr<std::thread>(new std::thread(&HantekDevice::run,std::ref(*this)));
}

ErrorCode HantekDevice::setChannelUsed(unsigned int channel, bool used) {
    if(!_device.isConnected())
        return ErrorCode::ERROR_CONNECTION;

    if(channel >= _specification.channels)
        return ErrorCode::ERROR_PARAMETER;

    // Update _settings
    _settings.voltage[channel].used = used;
    unsigned int channelCount = 0;
    for(unsigned channelCounter = 0; channelCounter < _specification.channels; ++channelCounter) {
        if(_settings.voltage[channelCounter].used)
            ++channelCount;
    }

    // Calculate the UsedChannels field for the command
    unsigned char usedChannels = USED_CH1;

    if(_settings.voltage[1].used) {
        if(_settings.voltage[0].used) {
            usedChannels = USED_CH1CH2;
        }
        else {
            // DSO-2250 uses a different value for channel 2
            if(specificationCommands.bulk.setChannels == BULK_BSETCHANNELS)
                usedChannels = BUSED_CH2;
            else
                usedChannels = USED_CH2;
        }
    }

    switch(specificationCommands.bulk.setChannels) {
        case BULK_SETTRIGGERANDSAMPLERATE: {
            // SetTriggerAndSamplerate bulk command for trigger source
            static_cast<BulkSetTriggerAndSamplerate *>(_command[BULK_SETTRIGGERANDSAMPLERATE].get())->setUsedChannels(usedChannels);
            _commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
            break;
        }
        case BULK_BSETCHANNELS: {
            // SetChannels2250 bulk command for active channels
            static_cast<BulkSetChannels2250 *>(_command[BULK_BSETCHANNELS].get())->setUsedChannels(usedChannels);
            _commandPending[BULK_BSETCHANNELS] = true;

            break;
        }
        case BULK_ESETTRIGGERORSAMPLERATE: {
            // SetTrigger5200s bulk command for trigger source
            static_cast<BulkSetTrigger5200 *>(_command[BULK_ESETTRIGGERORSAMPLERATE].get())->setUsedChannels(usedChannels);
            _commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;
            break;
        }
        default:
            break;
    }

    // Check if fast rate mode availability changed
    bool fastRateChanged = (_settings.usedChannels <= 1) != (channelCount <= 1);
    _settings.usedChannels = channelCount;

    if(fastRateChanged)
        updateSamplerateLimits();

    return ErrorCode::ERROR_NONE;
}

ErrorCode HantekDevice::setCoupling(unsigned int channel, DSO::Coupling coupling) {
    if(!_device.isConnected())
        return ErrorCode::ERROR_CONNECTION;

    if(channel >= _specification.channels)
        return ErrorCode::ERROR_PARAMETER;

    // SetRelays control command for coupling relays
    ControlSetRelays* c = static_cast<ControlSetRelays*>(_controlCommands[CONTROLINDEX_SETRELAYS].control.get());
    c->setCoupling(channel, coupling != DSO::Coupling::COUPLING_AC);
    _controlCommands[CONTROLINDEX_SETRELAYS].controlPending = true;

    return ErrorCode::ERROR_NONE;
}

double HantekDevice::setGain(unsigned int channel, double gain) {
    if(!_device.isConnected())
        return (double) ErrorCode::ERROR_CONNECTION; /// \todo Return error

    if(channel >= _specification.channels)
        return (double) ErrorCode::ERROR_PARAMETER; /// \todo Return error

    // Find lowest gain voltage thats at least as high as the requested
    unsigned gainId;
    for(gainId = 0; gainId < _specification.gainSteps.size() - 1; ++gainId)
            if(_specification.gainSteps[gainId] >= gain)
                    break;

    // SetGain bulk command for gain
    static_cast<BulkSetGain *>(_command[BULK_SETGAIN].get())->setGain(channel, _specification.gainIndex[gainId]);
    _commandPending[BULK_SETGAIN] = true;

    // SetRelays control command for gain relays
    ControlSetRelays *controlSetRelays = static_cast<ControlSetRelays *>(_controlCommands[CONTROLINDEX_SETRELAYS].control.get());
    controlSetRelays->setBelow1V(channel, gainId < 3);
    controlSetRelays->setBelow100mV(channel, gainId < 6);
    _controlCommands[CONTROLINDEX_SETRELAYS].controlPending = true;

    _settings.voltage[channel].gain = gainId;

    setOffset(channel, _settings.voltage[channel].offset);

    return _specification.gainSteps[gainId];
}

double HantekDevice::setOffset(unsigned int channel, double offset) {
    if(!_device.isConnected())
        return (double) ErrorCode::ERROR_CONNECTION; /// \todo Return error

    if(channel >= _specification.channels)
        return (double) ErrorCode::ERROR_PARAMETER; /// \todo Return error

    // Calculate the offset value
    // The range is given by the calibration data (convert from big endian)
    unsigned short int minimum = ((unsigned short int) *((unsigned char *) &(_specification.limits[channel].offset[_settings.voltage[channel].gain][DSO::OFFSET_START])) << 8) + *((unsigned char *) &(_specification.limits[channel].offset[_settings.voltage[channel].gain][DSO::OFFSET_START]) + 1);
    unsigned short int maximum = ((unsigned short int) *((unsigned char *) &(_specification.limits[channel].offset[_settings.voltage[channel].gain][DSO::OFFSET_END])) << 8) + *((unsigned char *) &(_specification.limits[channel].offset[_settings.voltage[channel].gain][DSO::OFFSET_END]) + 1);
    unsigned short int offsetValue = offset * (maximum - minimum) + minimum + 0.5;
    double offsetReal = (double) (offsetValue - minimum) / (maximum - minimum);

    // SetOffset control command for channel offset
    static_cast<ControlSetOffset *>(_controlCommands[CONTROLINDEX_SETOFFSET].control.get())->setChannel(channel, offsetValue);
    _controlCommands[CONTROLINDEX_SETOFFSET].controlPending = true;

    _settings.voltage[channel].offset = offset;
    _settings.voltage[channel].offsetReal = offsetReal;

    setTriggerLevel(channel, _settings.trigger.level[channel]);

    return offsetReal;
}

int HantekDevice::bulkCommand(TransferBuffer* command) {
    // Send BeginCommand control command
    int errorCode = _device.controlWrite(CONTROL_BEGINCOMMAND, beginCommandControl->data(), beginCommandControl->size());
    if(errorCode < 0)
            return errorCode;

    return _device.bulkCommand(_command[BULK_GETCAPTURESTATE]->data(), _command[BULK_GETCAPTURESTATE]->size());
}

}
