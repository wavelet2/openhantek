#include "currentdevice.h"
#include "deviceList.h"

CurrentDevice::CurrentDevice(DSO::DeviceList* deviceList)
    : m_deviceList(deviceList)
{

}

void CurrentDevice::setDevice(std::shared_ptr<DSO::DeviceBase> deviceBase)
{
    if (!deviceBase) return;
    m_valid = true;
    m_device = deviceBase;
    emit validChanged();
}

void CurrentDevice::setDevice(unsigned uid)
{
    setDevice(m_deviceList->getDeviceByUID(uid));
}

void CurrentDevice::resetDevice()
{
    m_valid = false;
    m_device.reset();
    emit validChanged();
}

bool CurrentDevice::valid() const
{
    return m_valid;
}
