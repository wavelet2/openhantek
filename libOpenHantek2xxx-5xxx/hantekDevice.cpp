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

#include "protocol.h"
#include "utils/containerStream.h"
#include "hantekDevice.h"
#include "protocol.h"
#include "utils/timestampDebug.h"
#include "utils/stdStringSplit.h"

namespace Hantek {
std::vector<unsigned short int>& operator<<(std::vector<unsigned short int>& v, unsigned short int x);
std::vector<unsigned char>& operator<<(std::vector<unsigned char>& v, unsigned char x);
std::vector<unsigned>& operator<<(std::vector<unsigned>& v, unsigned x);
std::vector<double>& operator<<(std::vector<double>& v, double x);

HantekDevice::HantekDevice(std::unique_ptr<DSO::USBCommunication> device)
    : DeviceBase(device->model()), _device(std::move(device)) {
    _device->setDisconnected_signal(std::bind( &HantekDevice::deviceDisconnected, this));
}

HantekDevice::~HantekDevice() {
    _device->disconnect();
}

unsigned HantekDevice::getUniqueID() {
    return _device->getUniqueID();
}

bool HantekDevice::needFirmware() {
    return false;
}

ErrorCode HantekDevice::uploadFirmware() {
    //TODO: Store firmware files together with the software
    return ErrorCode::ERROR_NONE;
}

void HantekDevice::deviceDisconnected() {
    if (!_thread.get()) return;
    if (_thread->joinable()) _thread->join();
    _thread.reset();
}

void HantekDevice::disconnectDevice() {
    _device->disconnect();
}

bool HantekDevice::isDeviceConnected() {
    return _device->isConnected();
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
            _previousSampleCount = getSampleCount(_device->getPacketSize());

            errorCode = bulkCommand(_bulkCommands[BULK_STARTSAMPLING].cmd.get());
            if(errorCode < 0) {
                if(errorCode == LIBUSB_ERROR_NO_DEVICE)
                    return false;
                break;
            }

            timestampDebug("Starting to capture");
            samplingStarted = true;
            break;
        case RollState::ROLL_ENABLETRIGGER:
            errorCode = bulkCommand(_bulkCommands[BULK_ENABLETRIGGER].cmd.get());
            if(errorCode < 0) {
                if(errorCode == LIBUSB_ERROR_NO_DEVICE)
                    return false;
                break;
            }
            timestampDebug("Enabling trigger");
            break;
        case RollState::ROLL_FORCETRIGGER:
            errorCode = bulkCommand(_bulkCommands[BULK_FORCETRIGGER].cmd.get());
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
            _previousSampleCount = getSampleCount(_device->getPacketSize());

            if(samplingStarted && lastTriggerMode == _settings.trigger.mode) {
                ++cycleCounter;

                if(cycleCounter == startCycle && !isRollingMode()) {
                    // Buffer refilled completely since start of sampling, enable the trigger now
                    errorCode = bulkCommand(_bulkCommands[BULK_ENABLETRIGGER].cmd.get());
                    if(errorCode < 0) {
                        if(errorCode == LIBUSB_ERROR_NO_DEVICE)
                            return false;
                        break;
                    }

                    timestampDebug("Enabling trigger");

                }
                else if(cycleCounter >= 8 + startCycle && _settings.trigger.mode == DSO::TriggerMode::TRIGGERMODE_AUTO) {
                    // Force triggering
                    errorCode = bulkCommand(_bulkCommands[BULK_FORCETRIGGER].cmd.get());
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
            errorCode = bulkCommand(_bulkCommands[BULK_STARTSAMPLING].cmd.get());
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
        if (!sendPendingCommands(_device.get())) break;

        // Compute sleep time
        int cycleTime;

        // Check the current oscilloscope state everytime 25% of the time the buffer should be refilled
        if(isRollingMode())
            cycleTime = (int) ((double) _device->getPacketSize() / ((_settings.samplerate.limits == &_specification.samplerate.multi) ? 1 : _specification.channels) / _settings.samplerate.current * 250);
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

    _device->disconnect();
    _statusMessage(LIBUSB_ERROR_NO_DEVICE);
}

int HantekDevice::readCaptureState() {
    int errorCode;

    errorCode = bulkCommand(_bulkCommands[BULK_GETCAPTURESTATE].cmd.get());
    if(errorCode < 0)
        return errorCode;

    BulkResponseGetCaptureState response;
    errorCode = _device->bulkRead(response.data(), response.size());
    if(errorCode < 0)
        return errorCode;

    _settings.trigger.point = calculateTriggerPoint(response.getTriggerPoint());

    return (int) response.getCaptureState();
}

int HantekDevice::readSamples(bool process) {
    int errorCode;

    // Request data
    errorCode = bulkCommand(_bulkCommands[BULK_GETDATA].cmd.get());
    if(errorCode < 0)
        return errorCode;

    // Save raw data to temporary buffer
    unsigned int totalSampleCount = getSampleCount(_device->getPacketSize());
    if(totalSampleCount == UINT_MAX)
        return LIBUSB_ERROR_INVALID_PARAM;

    // To make sure no samples will remain in the scope buffer,
    // also check the sample count before the last sampling started
    if(totalSampleCount < _previousSampleCount)
        std::swap(_previousSampleCount,totalSampleCount);
    else
        _previousSampleCount = totalSampleCount;

    unsigned int dataLength = totalSampleCount;
    if(_specification.sampleSize > 8) // for ADCs with resolution of 9bit..16bit
        dataLength *= 2;              // we need two bytes for the transfer

    /// \todo We grow the stack by multiple kilobytes to megabytes each sample iteration.
    /// Should we allocate the buffer beforehand?
    unsigned char data[dataLength];

    errorCode = _device->bulkReadMulti(data, dataLength);
    if(errorCode < 0)
            return errorCode;
    dataLength = errorCode; // actual data read

    // Process the data only if we want it
    if(process) {
        _samplesMutex.lock();
        processSamples(data, dataLength, totalSampleCount);
        _samplesMutex.unlock();
        _samplesAvailable(&_samples,
                            _settings.samplerate.current,
                            isRollingMode(),
                            _samplesMutex);
    }

    return errorCode;
}

void HantekDevice::connectDevice() {
    if (_model.need_firmware) return;

    _statusMessage(_device->connect());
    if(!_device->isConnected())
            return;

    _bulkCommands.clear();
    _bulkCommands.resize(BULK_COUNT);
    _bulkCommands[BULK_FORCETRIGGER].cmd.reset(new BulkForceTrigger());
    _bulkCommands[BULK_STARTSAMPLING].cmd.reset(new BulkCaptureStart());
    _bulkCommands[BULK_ENABLETRIGGER].cmd.reset(new BulkTriggerEnabled());
    _bulkCommands[BULK_GETDATA].cmd.reset(new BulkGetData());
    _bulkCommands[BULK_GETCAPTURESTATE].cmd.reset(new BulkGetCaptureState());
    _bulkCommands[BULK_SETGAIN].cmd.reset(new BulkSetGain());

    _controlCommands.clear();
    _controlCommands.resize(CONTROLINDEX_COUNT);
    _controlCommands.push_back({std::unique_ptr<TransferBuffer>(new ControlSetOffset()), CONTROL_SETOFFSET, false});
    _controlCommands.push_back({std::unique_ptr<TransferBuffer>(new ControlSetRelays()), CONTROL_SETRELAYS, false});

    // Determine the _command version we need for this model
    bool unsupported = false;
    switch(_model.productID) {
        case 0x2150:
            unsupported = true;

        case 0x2090:
            // Instantiate additional _commands for the DSO-2090. Initialize those as pending
            _bulkCommands[BULK_SETTRIGGERANDSAMPLERATE].cmd.reset(new BulkSetTriggerAndSamplerate());
            _bulkCommands[BULK_SETTRIGGERANDSAMPLERATE].pending = true;
            break;

        case 0x2250:
            // Instantiate additional _commands for the DSO-2250. Initialize those as pending
            _bulkCommands[BULK_BSETCHANNELS].cmd.reset(new BulkSetChannels2250());
            _bulkCommands[BULK_BSETCHANNELS].pending = true;
            _bulkCommands[BULK_CSETTRIGGERORSAMPLERATE].cmd.reset(new BulkSetTrigger2250());
            _bulkCommands[BULK_CSETTRIGGERORSAMPLERATE].pending = true;
            _bulkCommands[BULK_DSETBUFFER].cmd.reset(new BulkSetRecordLength2250());
            _bulkCommands[BULK_DSETBUFFER].pending = true;
            _bulkCommands[BULK_ESETTRIGGERORSAMPLERATE].cmd.reset(new BulkSetSamplerate2250());
            _bulkCommands[BULK_ESETTRIGGERORSAMPLERATE].pending = true;
            _bulkCommands[BULK_FSETBUFFER].cmd.reset(new BulkSetBuffer2250());
            _bulkCommands[BULK_FSETBUFFER].pending = true;
            break;

        case 0x520A:
            unsupported = true;

        case 0x5200:
            // Instantiate additional _commands for the DSO-5200. Initialize those as pending
            _bulkCommands[BULK_CSETTRIGGERORSAMPLERATE].cmd.reset(new BulkSetSamplerate5200());
            _bulkCommands[BULK_CSETTRIGGERORSAMPLERATE].pending = true;
            _bulkCommands[BULK_DSETBUFFER].cmd.reset(new BulkSetBuffer5200());
            _bulkCommands[BULK_DSETBUFFER].pending = true;
            _bulkCommands[BULK_ESETTRIGGERORSAMPLERATE].cmd.reset(new BulkSetTrigger5200());
            _bulkCommands[BULK_ESETTRIGGERORSAMPLERATE].pending = true;

            break;

        default:
            _device->disconnect();
            _statusMessage(10000);
            return;
    }

    if(unsupported)
        std::cerr <<"Warning: This Hantek DSO model isn't supported officially, so it may not be working as expected. Reports about your experiences are very welcome though (Please open a feature request in the tracker at http://www.github.com/openhantek/openhantek). If it's working perfectly this warning can be removed, if not it should be possible to get it working with your help soon." << std::endl;

    for(Control& control: _controlCommands)
        control.pending = true;

    // Maximum possible samplerate for a single channel and dividers for record lengths
    _specification.channels         = 2;
    _specification.channels_special = 2;
    _settings.samplerate.limits = &(_specification.samplerate.single);

    _specification.limits.resize(_specification.channels);
    _specification.gainSteps.clear();
    _settings.voltage.resize(_specification.channels);
    _settings.trigger.level.resize(_specification.channels);

    // clear
    for(DSO::dsoSpecification::channelLimits& l: _specification.limits) l.voltage.clear();
    _specification.bufferDividers.clear();
    _specification.samplerate.single.recordLengths.clear();
    _specification.samplerate.multi.recordLengths.clear();

    _specification.specialTriggerSources = {"EXT", "EXT/10"};

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
    int errorCode = _device->controlRead(CONTROL_VALUE, (unsigned char *) &(offsetLimit),
                                       sizeof(offsetLimit), (int) VALUE_OFFSETLIMITS);
    if(errorCode < 0) {
        _device->disconnect();
        _statusMessage(errorCode);
        return;
    }

    for (unsigned c=0; c < _specification.channels; ++c)
        memcpy((char*)&(_specification.limits[c].offset[0]), offsetLimit[c], 9*DSO::OFFSET_COUNT);

    // _signals for initial _settings
    updateSamplerateLimits();
    _recordLengthChanged(_settings.samplerate.limits->recordLengths, _settings.recordLengthId);
    if(!isRollingMode())
        _recordTimeChanged((double) _settings.samplerate.limits->recordLengths[_settings.recordLengthId] / _settings.samplerate.current);
    _samplerateChanged(_settings.samplerate.current);

    _sampling = false;
    // The control loop is running until the device is disconnected
    _thread = std::unique_ptr<std::thread>(new std::thread(&HantekDevice::run,std::ref(*this)));
}

ErrorCode HantekDevice::setChannelUsed(unsigned int channel, bool used) {
    if(!_device->isConnected())
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
            if(_model.productID == 0x2250)
                usedChannels = BUSED_CH2;
            else
                usedChannels = USED_CH2;
        }
    }

    switch(_model.productID) {
        case 0x2150:
        case 0x2090: {
            // SetTriggerAndSamplerate bulk command for trigger source
            static_cast<BulkSetTriggerAndSamplerate *>(_bulkCommands[BULK_SETTRIGGERANDSAMPLERATE].cmd.get())->setUsedChannels(usedChannels);
            _bulkCommands[BULK_SETTRIGGERANDSAMPLERATE].pending = true;
            break;
        }
        case 0x2250: {
            // SetChannels2250 bulk command for active channels
            static_cast<BulkSetChannels2250 *>(_bulkCommands[BULK_BSETCHANNELS].cmd.get())->setUsedChannels(usedChannels);
            _bulkCommands[BULK_BSETCHANNELS].pending = true;

            break;
        }
        case 0x520A:
        case 0x5200: {
            // SetTrigger5200s bulk command for trigger source
            static_cast<BulkSetTrigger5200 *>(_bulkCommands[BULK_ESETTRIGGERORSAMPLERATE].cmd.get())->setUsedChannels(usedChannels);
            _bulkCommands[BULK_ESETTRIGGERORSAMPLERATE].pending = true;
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
    if(!_device->isConnected())
        return ErrorCode::ERROR_CONNECTION;

    if(channel >= _specification.channels)
        return ErrorCode::ERROR_PARAMETER;

    // SetRelays control command for coupling relays
    ControlSetRelays* c = static_cast<ControlSetRelays*>(_controlCommands[CONTROLINDEX_SETRELAYS].control.get());
    c->setCoupling(channel, coupling != DSO::Coupling::COUPLING_AC);
    _controlCommands[CONTROLINDEX_SETRELAYS].pending = true;

    return ErrorCode::ERROR_NONE;
}

ErrorCode HantekDevice::setGain(unsigned int channel, double gain) {
    if(!_device->isConnected())
        return ErrorCode::ERROR_CONNECTION;

    if(channel >= _specification.channels)
        return ErrorCode::ERROR_PARAMETER;

    // Find lowest gain voltage thats at least as high as the requested
    unsigned gainId;
    for(gainId = 0; gainId < _specification.gainSteps.size() - 1; ++gainId)
            if(_specification.gainSteps[gainId] >= gain)
                    break;

    // SetGain bulk command for gain
    static_cast<BulkSetGain *>(_bulkCommands[BULK_SETGAIN].cmd.get())->setGain(channel, _specification.gainIndex[gainId]);
    _bulkCommands[BULK_SETGAIN].pending = true;

    // SetRelays control command for gain relays
    ControlSetRelays *controlSetRelays = static_cast<ControlSetRelays *>(_controlCommands[CONTROLINDEX_SETRELAYS].control.get());
    controlSetRelays->setBelow1V(channel, gainId < 3);
    controlSetRelays->setBelow100mV(channel, gainId < 6);
    _controlCommands[CONTROLINDEX_SETRELAYS].pending = true;

    _settings.voltage[channel].gain = gainId;

    setOffset(channel, _settings.voltage[channel].offset);

    // _specification.gainSteps[gainId]
    return ErrorCode::ERROR_NONE;
}

ErrorCode HantekDevice::setOffset(unsigned int channel, double offset) {
    if(!_device->isConnected())
        return ErrorCode::ERROR_CONNECTION; /// \todo Return error

    if(channel >= _specification.channels)
        return ErrorCode::ERROR_PARAMETER; /// \todo Return error

    // Calculate the offset value
    // The range is given by the calibration data (convert from big endian)
    unsigned short int minimum = ((unsigned short int) *((unsigned char *) &(_specification.limits[channel].offset[_settings.voltage[channel].gain][DSO::OFFSET_START])) << 8) + *((unsigned char *) &(_specification.limits[channel].offset[_settings.voltage[channel].gain][DSO::OFFSET_START]) + 1);
    unsigned short int maximum = ((unsigned short int) *((unsigned char *) &(_specification.limits[channel].offset[_settings.voltage[channel].gain][DSO::OFFSET_END])) << 8) + *((unsigned char *) &(_specification.limits[channel].offset[_settings.voltage[channel].gain][DSO::OFFSET_END]) + 1);
    unsigned short int offsetValue = offset * (maximum - minimum) + minimum + 0.5;
    double offsetReal = (double) (offsetValue - minimum) / (maximum - minimum);

    // SetOffset control command for channel offset
    static_cast<ControlSetOffset *>(_controlCommands[CONTROLINDEX_SETOFFSET].control.get())->setChannel(channel, offsetValue);
    _controlCommands[CONTROLINDEX_SETOFFSET].pending = true;

    _settings.voltage[channel].offset = offset;
    _settings.voltage[channel].offsetReal = offsetReal;

    setTriggerLevel(channel, _settings.trigger.level[channel]);

    // offsetReal;
    return ErrorCode::ERROR_NONE;
}

int HantekDevice::bulkCommand(TransferBuffer* command) {
    // Send BeginCommand control command
    int errorCode = _device->controlWrite(CONTROL_BEGINCOMMAND, beginCommandControl->data(), beginCommandControl->size());
    if(errorCode < 0)
        return errorCode;

    return _device->bulkWrite(command->data(), command->size());
}

}
