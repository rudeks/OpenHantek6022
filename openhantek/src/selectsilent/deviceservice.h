#ifndef DEVICESERVICE_H
#define DEVICESERVICE_H

#include "usb/usbdevice.h"

#include <QDebug>
#include <QThread>

class FindDevices;

class DeviceMeta {
    friend class DeviceService;
    friend QDebug operator<<(QDebug dbg, const DeviceMeta &c);

  private:
    UniqueUSBid m_nID;
    QString m_name;
    QString m_errorMessage;
    bool m_bNeedsFirmware;
    bool m_bCanConnect;

  public:
    const QString &name() const { return m_name; }
    const QString &errorMessage() const { return m_errorMessage; }
    UniqueUSBid id() const { return m_nID; }
    bool isReady() const { return m_bCanConnect && !m_bNeedsFirmware; }
};

QDebug operator<<(QDebug dbg, const DeviceMeta &c);

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
 * Connect to QThread::started() and QThread::finished() if you want to know when the thread starts and finishes.
 *
 */
class DeviceService : public QThread {
    Q_OBJECT
    Q_DISABLE_COPY(DeviceService)

  private:
    static const int METATYPE_ID_UNIQUEUSBID;
    static const int METATYPE_ID_DEVICEMETA;
    static const int METATYPE_ID_DEVICEMETALIST;

  public:
    friend QDebug operator<<(QDebug dbg, const DeviceMeta &c);

  public:
    explicit DeviceService(libusb_context *pContext);
    virtual ~DeviceService() override;

    std::unique_ptr<USBDevice> waitForAnyDevice(int nTimeout = -1);

    std::unique_ptr<USBDevice> acceptDevice(UniqueUSBid id);

    void setScanTimeout(int nTimeout);
    int scanTimeout() const;

  private:
    const QList<DeviceMeta> &processDevices();

  protected:
    void run() override;

  signals:
    /**
     * @brief devices is emitted as soon as device list changes
     * @param devices
     */
    void devices(const QList<DeviceMeta> &devices);

    /**
     * @brief uploadFirmware is emitted when the firmware upload was triggered
     * @param name
     */
    void uploadFirmware(const QString &name);

    /**
     * @brief uploadFirmwareError is emitted when uploading firmware fails
     * @param name
     * @param error
     */
    void uploadFirmwareError(const QString &name, const QString &error);

    /**
     * @brief deviceReady is emitted if a device becomes ready for use
     * @param name
     * @param id
     */
    void deviceReady(const QString &name, UniqueUSBid id);

    /**
     * @brief timeout if a timeout is set up and it is hit, this event is emitted before the thread exits.
     */
    void timeout();

  public slots:

  private slots:
    void reScanDevices();

  private:
    libusb_context *m_pUSBContext;
    FindDevices *m_pFindDevices;

    QList<DeviceMeta> m_devices;

    int m_nTimeout;
};

#endif // DEVICESERVICE_H
