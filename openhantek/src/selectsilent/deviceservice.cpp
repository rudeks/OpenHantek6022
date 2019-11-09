#include "deviceservice.h"

#include "usb/finddevices.h"
#include "usb/uploadFirmware.h"

#include "dsomodel.h"

#include <memory>

#include <QTimer>

#include <QEventLoop>

#include <QDebug>

DeviceService::DeviceService(libusb_context *pContext) : QThread(nullptr), m_pUSBContext(pContext) {
    m_pFindDevices = new FindDevices(m_pUSBContext);
}

DeviceService::~DeviceService() { delete m_pFindDevices; }

std::unique_ptr<USBDevice> DeviceService::waitForAnyDevice(int nTimeout) {
    start();

    UniqueUSBid takeDeviceID = 0L;
    connect(this, &DeviceService::deviceReady, [&](const QString &, UniqueUSBid id) { takeDeviceID = id; });

    if (nTimeout > -1)
        QTimer::singleShot(nTimeout, this, &QThread::quit);

    wait();

    // takeDeviceID is still 0L, the thread was ended, probably with an error. Device can not be selected for
    // operation. Users may connect to the uploadFirmwareError() signal to get to know what's wrong.
    if (takeDeviceID == 0L) return std::unique_ptr<USBDevice>();

    // Taking device with the ID that was signalled by the deviceReady signal
    return m_pFindDevices->takeDevice(takeDeviceID);
}

void DeviceService::run() {
    qDebug() << "DeviceService thread started";

    QTimer updateTimer;
    updateTimer.setInterval(1000);
    updateTimer.start();
    // Direct connection to use the thread's event loop. Alternatively, this object could be moved to the new thread.
    connect(&updateTimer, &QTimer::timeout, this, &DeviceService::reScanDevices, Qt::DirectConnection);
    //connect(this, &DeviceService::devices, &updateTimer,
    //        &QTimer::stop); // stop update timer as soon as devices are found

    connect(this, &DeviceService::devices, this, &DeviceService::processDevices, Qt::DirectConnection);

    // Quit on error OR on device ready
    connect(this, &DeviceService::deviceReady, this, &QThread::quit, Qt::DirectConnection);
    connect(this, &DeviceService::uploadFirmwareError, this, &QThread::quit, Qt::DirectConnection);

    exec();

    qDebug() << "DeviceService thread ended";
}

QDebug operator<<(QDebug dbg, const DeviceService::DeviceMeta &c) {

    dbg.nospace() << c.name << c.name << " (" << c.id << "): canConnect=" << c.canConnect
                  << ", needsFirmware=" << c.needsFirmware;
    return dbg;
}

void DeviceService::reScanDevices() {
    processDevices();
}

/**
 * @brief Get a list of devices that can be displayed. Does nothing but convert the raw device list to a displayable,
 * simple structure.
 * @return
 */
const QList<DeviceService::DeviceMeta> &DeviceService::processDevices() {

    if (m_pFindDevices->updateDeviceList()) {
        qInfo() << "Device list changed";
        m_devices.clear();
        const FindDevices::DeviceList *pDeviceList = m_pFindDevices->getDevices();
        // create internal structure to manage devices
        for (auto &i : *pDeviceList) {
            DeviceMeta entry;
            entry.name = QString::fromStdString(i.second->getModel()->name);
            entry.id = i.first;
            if (i.second->needsFirmware()) {

                qInfo() << "Installing firmware";

                UploadFirmware uf;
                if (!uf.startUpload(i.second.get())) {
                    qCritical() << "Firmwar installation failed:" << uf.getErrorMessage();
                    entry.errorMessage = uf.getErrorMessage();
                }

                entry.needsFirmware = true;
            } else if (i.second->connectDevice(entry.errorMessage)) {
                if (entry.errorMessage.isEmpty())
                    qInfo() << "Connection attempt succeeded";
                else
                    qCritical() << "Connection attempt failed:" << entry.errorMessage;
                i.second->disconnectFromDevice();
                entry.canConnect = true;

                emit deviceReady(entry.name, entry.id);
            } else {
                entry.canConnect = false;
            }
            m_devices << entry;
        }

        emit devices(m_devices); // Emit all devices that were found
    }

    return m_devices;
}
