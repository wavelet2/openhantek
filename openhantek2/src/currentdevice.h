#pragma once

#include <QObject>
#include <memory>

namespace DSO {
    class DeviceBase;
    class DeviceList;
}

/**
 * @brief A wrapper for DSO::DeviceBase with QObject signals and slots.
 */
class CurrentDevice : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool valid READ valid NOTIFY validChanged)
public:
    CurrentDevice(DSO::DeviceList* deviceList);

    bool valid() const;

signals:
    void validChanged();

public slots:
    void setDevice(std::shared_ptr<DSO::DeviceBase> deviceBase);
    void setDevice(unsigned uid);
    void resetDevice();

private:
    std::shared_ptr<DSO::DeviceBase> m_device = 0;
    DSO::DeviceList* m_deviceList;
    bool m_valid = false;
};
