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
#include <memory>

#include "utils/transferBuffer.h"
#include "errorcodes.h"

namespace DSO {
class USBCommunication;

//////////////////////////////////////////////////////////////////////////////
/// \brief Part of the base class for an DSO device implementation. To implement
/// thread safety, bulk and control "queues" are established. Those can be "filled"
/// by any thread, and are send in the communication thread via sendCommands(device).
/// Each "queue" entry has a pending flag. If that is set, the corresponding entry
/// will be send.
class CommunicationThreadQueues {
public:
    struct BulkCmdStr {
        /// Pointers to bulk commands, ready to be transmitted
        std::unique_ptr<TransferBuffer> cmd;
        /// true, when the command should be executed
        bool pending;
    };

    std::vector<BulkCmdStr> _bulkCommands;

    struct Control {
        /// Pointers to control commands, ready to be transmitted
        std::unique_ptr<TransferBuffer> control;
        unsigned char controlCode;
        /// true, when the command should be executed
        bool pending;
    };

    std::vector<Control> _controlCommands;

    /// \brief Sends bulk/control commands directly.
    /// Check if device is connected with isDeviceConnected before.
    /// <p>
    ///        <b>Syntax:</b><br />
    ///        <br />
    ///        Bulk command:
    ///        <pre>send bulk [<em>hex data</em>]</pre>
    ///        %Control command:
    ///        <pre>send control [<em>hex code</em>] [<em>hex data</em>]</pre>
    /// </p>
    /// \param command The command as string (Has to be parsed).
    /// \return See ::ErrorCode::ErrorCode.
    ErrorCode stringCommand(const std::string& command);

    /// Send all pending bulk/control commands
    bool sendPendingCommands(DSO::USBCommunication* device);
};

}
