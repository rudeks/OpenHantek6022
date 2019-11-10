#include "deviceservicedialog.h"
#include "ui_deviceservicedialog.h"

#include <QTimer>

const int DeviceServiceDialog::CONVENIENCE_DELAY = 2000;
const QString &DeviceServiceDialog::WAITING_TEXT = QApplication::tr("Waiting for device");

DeviceServiceDialog::DeviceServiceDialog(QWidget *parent) : QDialog(parent), ui(new Ui::DeviceServiceDialog) {
    ui->setupUi(this);
    ui->labelError->hide();
    m_id = 0L;

    setStatus(WAITING_TEXT);
}

DeviceServiceDialog::~DeviceServiceDialog() { delete ui; }

void DeviceServiceDialog::setStatus(const QString &name) {
    ui->labelStatus->setText(name);
    setWindowTitle(name);
}

void DeviceServiceDialog::showDevices(const QList<DeviceMeta> &devices) {
    ui->listWidgetDevices->clear();

    if (devices.isEmpty()) {
        setStatus(WAITING_TEXT);
        ui->buttonBox->setEnabled(false);
    }

    for (const DeviceMeta &dm : devices) {
        ui->listWidgetDevices->addItem(QString("%1 [%3]%4")
                                           .arg(dm.name())
                                           .arg(dm.isReady() ? "ready" : "not ready")
                                           .arg(dm.errorMessage().isEmpty() ? "" : " Error: " + dm.errorMessage()));
    }
}
void DeviceServiceDialog::onUploadFirmware(const QString &name) {
    setWindowTitle(name);
    ui->buttonBox->setEnabled(false);

    ui->labelStatus->setText("Uploading firmware to " + name);
}
void DeviceServiceDialog::onUploadFirmwareError(const QString &name, const QString &error) {
    setWindowTitle(name);
    ui->buttonBox->setEnabled(false);

    ui->labelStatus->setText(name);
    ui->labelError->setText(error);
    ui->labelError->show();

    QTimer::singleShot(CONVENIENCE_DELAY, this, &QDialog::reject);
}
void DeviceServiceDialog::onDeviceReady(const QString &name, UniqueUSBid id) {
    setWindowTitle(name);
    ui->buttonBox->setEnabled(false);

    ui->labelError->hide();
    ui->labelStatus->setText(name + " is ready");

    m_id = id;

    QTimer::singleShot(CONVENIENCE_DELAY, this, &QDialog::accept);
}

UniqueUSBid DeviceServiceDialog::acceptedDeviceId() const { return m_id; }
