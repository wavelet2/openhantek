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
#include "utils/stdStringSplit.h"

#include "deviceBaseCommands.h"

namespace DSO {
    ErrorCode DeviceBaseCommands::stringCommand(const std::string& command) {
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
            if(commandCode > _command.size())
                return ErrorCode::ERROR_UNSUPPORTED;

            // Update bulk command and mark as pending
            hexParse(data, _command[commandCode]->data(), _command[commandCode]->size());
            _commandPending[commandCode] = true;
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
            _controlCommands[control].controlPending = true;
            return ErrorCode::ERROR_NONE;
        }

        return ErrorCode::ERROR_UNSUPPORTED;
    }
}
