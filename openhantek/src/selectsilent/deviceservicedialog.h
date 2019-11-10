#ifndef DEVICESERVICEDIALOG_H
#define DEVICESERVICEDIALOG_H

#include <QDialog>

#include "deviceservice.h"

namespace Ui {
class DeviceServiceDialog;
}

class DeviceServiceDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit DeviceServiceDialog(QWidget *parent = nullptr);
    ~DeviceServiceDialog();

    UniqueUSBid acceptedDeviceId() const;

  private:
    void setStatus(const QString &name);

  private:
    Ui::DeviceServiceDialog *ui;

  public slots:
    void showDevices(const QList<DeviceMeta> &devices);
    void onUploadFirmware(const QString &name);
    void onUploadFirmwareError(const QString &name, const QString &error);
    void onDeviceReady(const QString &name, UniqueUSBid id);

  private:
    UniqueUSBid m_id;

  private:
    static const int CONVENIENCE_DELAY;
    static const QString &WAITING_TEXT;

};

#endif // DEVICESERVICEDIALOG_H
