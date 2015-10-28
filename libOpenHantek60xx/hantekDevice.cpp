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

#include "utils/containerStream.h"

#include <libusb-1.0/libusb.h>

#include "HT6022BEfw.h"
#include "HT6022BLfw.h"

#include "hantekDevice.h"
#include "utils/timestampDebug.h"
#include "utils/stdStringSplit.h"

static std::vector<double>& operator<<(std::vector<double>& v, double x) {
    v.push_back(x);
    return v;
}
static std::vector<unsigned>& operator<<(std::vector<unsigned>& v, unsigned x) {
    v.push_back(x);
    return v;
}
static std::vector<unsigned char>& operator<<(std::vector<unsigned char>& v, unsigned char x) {
    v.push_back(x);
    return v;
}
static std::vector<unsigned short>& operator<<(std::vector<unsigned short>& v, unsigned short x) {
    v.push_back(x);
    return v;
}

namespace Hantek60xx {

HantekDevice::HantekDevice(std::unique_ptr<DSO::USBCommunication> device)
    : DeviceBase(device->model()), _device(std::move(device)) {
    _device->setDisconnected_signal(std::bind( &HantekDevice::deviceDisconnected, this));
}

HantekDevice::~HantekDevice() {
    _device->disconnect();
}

unsigned HantekDevice::getUniqueID() const {
    return _device->getUniqueID();
}

bool HantekDevice::needFirmware() const {
    return _model.need_firmware;
}

ErrorCode HantekDevice::uploadFirmware() {
    int error_code = _device->connect();
    if (error_code != LIBUSB_SUCCESS) {
        libusb_error_name(error_code);
        std::cerr << "Firmware upload: " << " " <<
        libusb_error_name((libusb_error)error_code) << " " <<
            libusb_strerror((libusb_error)error_code) << std::endl;
        return (ErrorCode)error_code;
    }

    int fwsize = 0;
    unsigned char* firmware = nullptr;
    switch (_model.productID) {
        case 0X6022:
            fwsize = HT6022_FIRMWARE_SIZE;
            firmware = HT6022_Firmware;
            break;
        case 0X602A:
            fwsize = HT6022BL_FIRMWARE_SIZE;
            firmware = HT6022BL_Firmware;
            break;
        default:
            return ErrorCode::ERROR_PARAMETER;
    }

    unsigned int Size, Value;
    while (fwsize) {
        Size  = *firmware + ((*(firmware + 1))<<0x08);
        firmware = firmware + 2;
        Value = *firmware + ((*(firmware + 1))<<0x08);
        firmware = firmware + 2;
        int error_code = _device->controlWrite(HT6022_FIRMWARE_REQUEST,firmware,Size,Value,HT6022_FIRMWARE_INDEX);
        if (error_code < 0) {
            _device->disconnect();
            return ErrorCode::ERROR_CONNECTION;
        }
        firmware = firmware + Size;
        fwsize--;
    }

    _uploadProgress(100);
    _device->disconnect();

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

bool HantekDevice::isDeviceConnected() const {
    return _device->isConnected();
}

void HantekDevice::connectDevice(){
    if (_model.need_firmware) return;

    if (_model.need_firmware) return;

    _statusMessage(_device->connect());
    if(!_device->isConnected())
            return;

    // No bulk commands so far. The GETSAMPLE command is issued
    // in the run() method and sample thread therefore and does
    // not nned the _bulkCommands synchronisation.
    _bulkCommands.clear();

    _controlCommands.clear();
    _controlCommands.resize(CONTROLINDEX_COUNT);
//    _controlCommands.push_back({std::unique_ptr<TransferBuffer>(new ControlSetOffset()), CONTROL_SETOFFSET, false});
//    _controlCommands.push_back({std::unique_ptr<TransferBuffer>(new ControlSetRelays()), CONTROL_SETRELAYS, false});

    for(Control& control: _controlCommands)
        control.pending = true;

    // Maximum possible samplerate for a single channel and dividers for record lengths
    _specification.channels         = 2;
    _specification.channels_special = 0;
    _settings.samplerate.limits = &(_specification.samplerate.single);

    _specification.limits.clear();
    _specification.limits.resize(_specification.channels);

    _specification.gainSteps.clear();

    _settings.voltage.clear();
    _settings.voltage.resize(_specification.channels);

    _settings.trigger.level.clear();
    _settings.trigger.level.resize(_specification.channels);

    // clear
    _specification.bufferDividers.clear();
    _specification.samplerate.single.recordLengths.clear();
    _specification.samplerate.multi.recordLengths.clear();

    _specification.specialTriggerSources.clear();

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

    _previousSampleCount = 0;

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

ErrorCode HantekDevice::setChannelUsed(unsigned int channel, bool used) {return ErrorCode::ERROR_NONE;}
ErrorCode HantekDevice::setCoupling(unsigned int channel, DSO::Coupling coupling) {return ErrorCode::ERROR_NONE;}
ErrorCode HantekDevice::setGain(unsigned int channel, double gain) {return ErrorCode::ERROR_NONE;}
ErrorCode HantekDevice::setOffset(unsigned int channel, double offset) {return ErrorCode::ERROR_NONE;}
ErrorCode HantekDevice::setTriggerSource(bool special, unsigned int id) {return ErrorCode::ERROR_NONE;}
ErrorCode HantekDevice::setTriggerLevel(unsigned int channel, double level) {return ErrorCode::ERROR_NONE;}
ErrorCode HantekDevice::setTriggerSlope(DSO::Slope slope) {return ErrorCode::ERROR_NONE;}
double HantekDevice::updatePretriggerPosition(double position) {return 0.0;}
double HantekDevice::computeBestSamplerate(double samplerate, bool fastRate, bool maximum, unsigned int *downsampler) {return 0.0;}
unsigned int HantekDevice::updateRecordLength(unsigned int index) {return 0;}
unsigned int HantekDevice::updateSamplerate(unsigned int downsampler, bool fastRate) {return 0;}
int HantekDevice::forceTrigger() {return 0;}

int HantekDevice::readSamples() {
    *_data = HT6022_READ_CONTROL_DATA;
    int errorCode = _device->controlWrite(HT6022_READ_CONTROL_REQUEST,
                                          _data,
                                          HT6022_READ_CONTROL_SIZE,
                                          HT6022_READ_CONTROL_VALUE,
                                          HT6022_READ_CONTROL_INDEX);
    if(errorCode < 0)
        return errorCode;

    return _device->bulkReadMulti(_data, (int)_dataSize*2);
}

void HantekDevice::run() {
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

        if (readSamples() < 0)
            break;

        //processSamples(_data, _dataSize);

        std::this_thread::sleep_for(std::chrono::milliseconds(cycleTime));
    }

    _device->disconnect();
    _statusMessage(LIBUSB_ERROR_NO_DEVICE);
}

}
