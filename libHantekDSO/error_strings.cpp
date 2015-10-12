#include "error_strings.h"
#include <QCoreApplication>

#include <libusb-1.0/libusb.h>

/// \brief Returns string representation for libusb errors.
/// \param error The error code.
/// \return String explaining the error.
QString libUsbErrorString(int error) {
    switch(error) {
        case LIBUSB_SUCCESS:
                return QCoreApplication::tr("Success (no error)");
        case LIBUSB_ERROR_IO:
                return QCoreApplication::tr("Input/output error");
        case LIBUSB_ERROR_INVALID_PARAM:
                return QCoreApplication::tr("Invalid parameter");
        case LIBUSB_ERROR_ACCESS:
                return QCoreApplication::tr("Access denied (insufficient permissions)");
        case LIBUSB_ERROR_NO_DEVICE:
                return QCoreApplication::tr("No such device (it may have been disconnected)");
        case LIBUSB_ERROR_NOT_FOUND:
                return QCoreApplication::tr("Entity not found");
        case LIBUSB_ERROR_BUSY:
                return QCoreApplication::tr("Resource busy");
        case LIBUSB_ERROR_TIMEOUT:
                return QCoreApplication::tr("Operation timed out");
        case LIBUSB_ERROR_OVERFLOW:
                return QCoreApplication::tr("Overflow");
        case LIBUSB_ERROR_PIPE:
                return QCoreApplication::tr("Pipe error");
        case LIBUSB_ERROR_INTERRUPTED:
                return QCoreApplication::tr("System call interrupted (perhaps due to signal)");
        case LIBUSB_ERROR_NO_MEM:
                return QCoreApplication::tr("Insufficient memory");
        case LIBUSB_ERROR_NOT_SUPPORTED:
                return QCoreApplication::tr("Operation not supported or unimplemented on this platform");
        default:
                return QCoreApplication::tr("Other error");
}
}