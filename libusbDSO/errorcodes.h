#pragma once

//////////////////////////////////////////////////////////////////////////////
/// \enum ErrorCode
/// \brief The return codes for device control methods.
enum class ErrorCode {
        ERROR_NONE = 0, ///< Successful operation
        ERROR_CONNECTION = -1, ///< Device not connected or communication error
        ERROR_UNSUPPORTED = -2, ///< Not supported by this device
        ERROR_PARAMETER = -3, ///< Parameter out of range
        ERROR_ACCESS = -4 ///< Access rights not sufficient
};