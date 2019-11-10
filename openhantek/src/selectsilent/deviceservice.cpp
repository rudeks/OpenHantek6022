#include "deviceservice.h"

#include "usb/finddevices.h"
#include "usb/uploadFirmware.h"

#include "dsomodel.h"

#include <memory>

#include <QTimer>

#include <QEventLoop>

#include <QDebug>

const int DeviceService::METATYPE_ID_UNIQUEUSBID = qRegisterMetaType<UniqueUSBid>("UniqueUSBid");
const int DeviceService::METATYPE_ID_DEVICEMETA = qRegisterMetaType<DeviceMeta>("DeviceMeta");
const int DeviceService::METATYPE_ID_DEVICEMETALIST = qRegisterMetaType<QList<DeviceMeta>>("QList<DeviceMeta>");

DeviceService::DeviceService(libusb_context *pContext) : QThread(nullptr), m_pUSBContext(pContext), m_nTimeout(-1) {
    m_pFindDevices = new FindDevices(m_pUSBContext);
}

DeviceService::~DeviceService() { delete m_pFindDevices; }

std::unique_ptr<USBDevice> DeviceService::waitForAnyDevice(int nTimeout) {
    m_nTimeout = nTimeout;

    start();

    UniqueUSBid takeDeviceID = 0L;
    connect(this, &DeviceService::deviceReady, [&](const QString &, UniqueUSBid id) { takeDeviceID = id; });

    wait();

    // takeDeviceID is still 0L, the thread was ended, probably with an error. Device can not be selected for
    // operation. Users may connect to the uploadFirmwareError() signal to get to know what's wrong.
    if (takeDeviceID == 0L) return std::unique_ptr<USBDevice>();

    // Taking device with the ID that was signalled by the deviceReady signal
    return acceptDevice(takeDeviceID);
}

/**
 * @brief DeviceService::acceptDevice
 *
 * Unlike waitForAnyDevice(), acceptDevice() returns immediately without blocking the caller. However, acceptDevice()
 * should only be called after deviceReady() signal was received.
 *
 * @return Unique pointer to the device that was accepted to be used, \c nullptr if no device is available to be
 * accepted.
 */
std::unique_ptr<USBDevice> DeviceService::acceptDevice(UniqueUSBid id) { return m_pFindDevices->takeDevice(id); }

void DeviceService::run() {
    qDebug() << "DeviceService thread started";

    QTimer timeoutTimer;

    if (m_nTimeout > -1) {
        timeoutTimer.setSingleShot(true);
        timeoutTimer.setInterval(m_nTimeout);
        timeoutTimer.start();
        connect(&timeoutTimer, &QTimer::timeout, this, &DeviceService::timeout, Qt::DirectConnection);
        connect(&timeoutTimer, &QTimer::timeout, this, &QThread::quit, Qt::DirectConnection);
    }

    QTimer updateTimer;
    updateTimer.setInterval(1000);
    updateTimer.start();
    // Direct connection to use the thread's event loop. Alternatively, this object could be moved to the new thread.
    connect(&updateTimer, &QTimer::timeout, this, &DeviceService::reScanDevices, Qt::DirectConnection);
    // connect(this, &DeviceService::devices, &updateTimer,
    //        &QTimer::stop); // stop update timer as soon as devices are found

    connect(this, &DeviceService::devices, this, &DeviceService::processDevices, Qt::DirectConnection);

    // Quit on error OR on device ready
    connect(this, &DeviceService::deviceReady, this, &QThread::quit, Qt::DirectConnection);
    connect(this, &DeviceService::uploadFirmwareError, this, &QThread::quit, Qt::DirectConnection);

    exec();

    qDebug() << "DeviceService thread ended";
}

QDebug operator<<(QDebug dbg, const DeviceMeta &c) {

    dbg.nospace() << c.m_name << c.m_name << " (" << c.m_nID << "): canConnect=" << c.m_bCanConnect
                  << ", needsFirmware=" << c.m_bNeedsFirmware;
    return dbg;
}

void DeviceService::reScanDevices() { processDevices(); }

/**
 * @brief Get a list of devices that can be displayed. Does nothing but convert the raw device list to a displayable,
 * simple structure.
 * @return
 */
const QList<DeviceMeta> &DeviceService::processDevices() {

    if (m_pFindDevices->updateDeviceList()) {
        qInfo() << "Device list changed";
        m_devices.clear();
        const FindDevices::DeviceList *pDeviceList = m_pFindDevices->getDevices();
        // create internal structure to manage devices
        for (auto &i : *pDeviceList) {
            DeviceMeta entry;
            entry.m_name = QString::fromStdString(i.second->getModel()->name);
            entry.m_nID = i.first;
            if (i.second->needsFirmware()) {
                qInfo() << "Installing firmware";

                emit uploadFirmware(entry.m_name);

                UploadFirmware uf;
                if (!uf.startUpload(i.second.get())) {
                    qCritical() << "Firmwar installation failed:" << uf.getErrorMessage();
                    entry.m_errorMessage = uf.getErrorMessage();

                    emit uploadFirmwareError(entry.m_name, entry.m_errorMessage);
                }

                entry.m_bNeedsFirmware = true;
            } else if (i.second->connectDevice(entry.m_errorMessage)) {
                if (entry.m_errorMessage.isEmpty())
                    qInfo() << "Connection attempt succeeded";
                else
                    qCritical() << "Connection attempt failed:" << entry.m_errorMessage;
                i.second->disconnectFromDevice();
                entry.m_bCanConnect = true;

                emit deviceReady(entry.m_name, entry.m_nID);
            } else {
                entry.m_bCanConnect = false;
            }
            m_devices << entry;
        }

        emit devices(m_devices); // Emit all devices that were found
    }

    return m_devices;
}

void DeviceService::setScanTimeout(int nTimeout) { m_nTimeout = nTimeout; }

int DeviceService::scanTimeout() const { return m_nTimeout; }
