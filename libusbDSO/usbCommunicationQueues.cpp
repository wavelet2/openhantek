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


#include <libusb-1.0/libusb.h>
#include "usbCommunication.h"

#include "utils/stdStringSplit.h"
#include "utils/timestampDebug.h"

#include "usbCommunicationQueues.h"

namespace DSO {
    ErrorCode CommunicationThreadQueues::stringCommand(const std::string& command) {
        std::vector<std::string> commandParts = split(command, " ");

        if(commandParts.size() < 1)
            return ErrorCode::ERROR_PARAMETER;

        if(commandParts[0] != "send")
            return ErrorCode::ERROR_UNSUPPORTED;

        if(commandParts.size() < 2)
            return ErrorCode::ERROR_PARAMETER;

        if(commandParts[1] == "bulk") {
            std::string data = section(command, 2);
            unsigned char commandCode = 0;

            // Read command code (First byte)
            hexParse(commandParts[2], &commandCode, 1);
            if(commandCode > _bulkCommands.size())
                return ErrorCode::ERROR_UNSUPPORTED;

            // Update bulk command and mark as pending
            hexParse(data, _bulkCommands[commandCode].cmd->data(), _bulkCommands[commandCode].cmd->size());
            _bulkCommands[commandCode].pending = true;
            return ErrorCode::ERROR_NONE;
        }
        else if(commandParts[1] == "control") {
            unsigned char controlCode = 0;

            // Read command code (First byte)
            hexParse(commandParts[2], &controlCode, 1);
            unsigned control;
            for(control = 0; control < _controlCommands.size(); ++control) {
                    if(_controlCommands[control].controlCode == controlCode)
                            break;
            }
            if(control >= _controlCommands.size())
                    return ErrorCode::ERROR_UNSUPPORTED;

            std::string data = section(command, 3);

            // Update control command and mark as pending
            hexParse(data, _controlCommands[control].control->data(), _controlCommands[control].control->size());
            _controlCommands[control].pending = true;
            return ErrorCode::ERROR_NONE;
        }

        return ErrorCode::ERROR_UNSUPPORTED;
    }

    bool CommunicationThreadQueues::sendPendingCommands(USBCommunication* device) {
        int errorCode = 0;

        for(BulkCmdStr& cmd: _bulkCommands) {
            if(!cmd.pending)
                continue;

            timestampDebug("Sending bulk command: " << hexDump(cmd.cmd->data(), cmd.cmd->size()));

            errorCode = device->bulkWrite(cmd.cmd.get()->data(), cmd.cmd.get()->size());
            if(errorCode < 0) {
                std::cerr << "Sending bulk command %02x failed: " << " " <<
                libusb_error_name((libusb_error)errorCode) << " " <<
                    libusb_strerror((libusb_error)errorCode) << std::endl;

                if(errorCode == LIBUSB_ERROR_NO_DEVICE)
                    return false;
            }
            else
                cmd.pending = false;
        }

        errorCode = 0;

        for(Control& control: _controlCommands) {
            if(!control.pending)
                continue;

            timestampDebug("Sending control command " << control.controlCode << " " << hexDump(control.control->data(), control.control->size()));

            errorCode = device->controlWrite(control.controlCode, control.control->data(), control.control->size());
            if(errorCode < 0) {
                std::cerr << "Sending control command failed: " << control.controlCode << " " <<
                    libusb_error_name((libusb_error)errorCode) << " " <<
                    libusb_strerror((libusb_error)errorCode) << std::endl;

                if(errorCode == LIBUSB_ERROR_NO_DEVICE)
                    return false;
            }
            else
                control.pending = false;
        }
        return true;
    }

}
