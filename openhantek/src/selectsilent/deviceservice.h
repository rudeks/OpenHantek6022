#ifndef DEVICESERVICE_H
#define DEVICESERVICE_H

#include "usb/usbdevice.h"

#include <QDebug>
#include <QThread>

class FindDevices;

/**
 * @brief The DeviceService class can be used to periodically scan for devices without the need of a user interface.
 *
 * After a device or multiple devices were found, the DeviceService attempts to upload the device firmware. If that
 * succeeds, it emits the deviceReady signal. If using waitForAnyDevice(), waitForAnyDevice() will return with a device
 * pointer of any device that became ready.
 *
 *
 * It is derived from QThread and has its own event loop. Therefore, the device scanner can
 * be used, before the main event loop is started.
 *
 *
 */
class DeviceService : public QThread {
    Q_OBJECT
    Q_DISABLE_COPY(DeviceService)

  private:
    class DeviceMeta {
        friend class DeviceService;
        friend QDebug operator<<(QDebug dbg, const DeviceService::DeviceMeta &c);

      private:
        UniqueUSBid id;
        QString name;
        QString errorMessage;
        bool needsFirmware;
        bool canConnect;
    };

    friend QDebug operator<<(QDebug dbg, const DeviceService::DeviceMeta &c);

  public:
    explicit DeviceService(libusb_context *pContext);
    virtual ~DeviceService() override;

    std::unique_ptr<USBDevice> waitForAnyDevice(int nTimeout = -1);

  private:
    const QList<DeviceMeta> &processDevices();

  protected:
    void run() override;

  signals:
    void devices(const QList<DeviceMeta> &devices);
    void uploadFirmwareError(const QString &name, const QString &error);
    void deviceReady(const QString &name, UniqueUSBid id);

  public slots:

  private slots:
    void reScanDevices();

  private:
    libusb_context *m_pUSBContext;
    FindDevices *m_pFindDevices;

    QList<DeviceMeta> m_devices;
};

QDebug operator<<(QDebug dbg, const DeviceService::DeviceMeta &c);

#endif // DEVICESERVICE_H
