// Copyright (c) 2015-2017, The Bytecoin developers
//
// This file is part of Bytecoin.
//
// NaniCoin is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// NaniCoin is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with NaniCoin.  If not, see <http://www.gnu.org/licenses/>.

#include <QFileDialog>
#include <QMessageBox>

#include "KeyDialog.h"
#include "IWalletAdapter.h"
#include "Settings/Settings.h"
#include "Style/Style.h"
#include "ui_KeyDialog.h"

namespace WalletGui {

namespace {

const char KEY_DIALOG_STYLE_SHEET_TEMPLATE[] =
  "* {"
    "font-family: %fontFamily%;"
  "}"

  "WalletGui--KeyDialog #m_keyEdit {"
    "font-size: %fontSizeNormal%;"
    "border-radius: 2px;"
    "border: 1px solid %borderColorDark%;"
  "}";

bool isTrackingKeys(const QByteArray& _array) {
  if (_array.size() < sizeof(AccountKeys)) {
    return false;
  }

  AccountKeys accountKeys;
  QDataStream trackingKeysDataStream(_array);
  trackingKeysDataStream.readRawData(reinterpret_cast<char*>(&accountKeys.spendKeys.publicKey), sizeof(Crypto::PublicKey));
  trackingKeysDataStream.readRawData(reinterpret_cast<char*>(&accountKeys.viewKeys.publicKey), sizeof(Crypto::PublicKey));
  trackingKeysDataStream.readRawData(reinterpret_cast<char*>(&accountKeys.spendKeys.secretKey), sizeof(Crypto::SecretKey));
  trackingKeysDataStream.readRawData(reinterpret_cast<char*>(&accountKeys.viewKeys.secretKey), sizeof(Crypto::SecretKey));
  return (std::memcmp(&accountKeys.spendKeys.secretKey, &CryptoNote::NULL_SECRET_KEY, sizeof(Crypto::SecretKey)) == 0);
}

}

KeyDialog::KeyDialog(const QByteArray& _key, bool _isTracking, QWidget *_parent)
  : QDialog(_parent, static_cast<Qt::WindowFlags>(Qt::WindowCloseButtonHint))
  , m_ui(new Ui::KeyDialog)
  , m_isTracking(_isTracking)
  , m_isExport(true)
  , m_key(_key) {
  m_ui->setupUi(this);
  m_ui->m_fileButton->setText(tr("Guardar a archivo"));
  m_ui->m_okButton->setText(tr("Cerrar"));
  m_ui->m_keyEdit->setReadOnly(true);
  m_ui->m_keyEdit->setPlainText(m_key.toHex().toUpper());
  if (m_isTracking) {
    m_ui->m_descriptionLabel->setText(tr("La llave de seguimiento permite a otras personas ver todas las transacciones entrantes de este monedero.\n"
      "No permite gastar su saldo."));
  }

  m_ui->m_cancelButton->hide();
  setFixedHeight(195);
  setStyleSheet(Settings::instance().getCurrentStyle().makeStyleSheet(KEY_DIALOG_STYLE_SHEET_TEMPLATE));
  setWindowTitle(m_isTracking ? tr("Exportar llave de seguimiento") : tr("Exportar llave"));
}

KeyDialog::KeyDialog(const QByteArray& _key, bool _isTracking, bool _isPrivateKeyExport, QWidget *_parent)
	: QDialog(_parent, static_cast<Qt::WindowFlags>(Qt::WindowCloseButtonHint))
	, m_ui(new Ui::KeyDialog)
	, m_isTracking(_isTracking)
	, m_isExport(true)
	, m_key(_key) {
	m_ui->setupUi(this);
	m_ui->m_fileButton->setText(tr("Guardar a archivo"));
	m_ui->m_okButton->setText(tr("Cerrar"));
	m_ui->m_keyEdit->setReadOnly(true);
	m_ui->m_keyEdit->setPlainText("Llave secreta de gasto:\n" + m_key.toHex().toUpper().mid(0,64) + "\n\nLlave secreta de vista:\n" + m_key.toHex().toUpper().mid(64));
	if (_isPrivateKeyExport) {
		m_ui->m_descriptionLabel->setText(tr("Estas llaves permiten restaurar su monedero en caso de pérdida de ésta."));
	}

	m_ui->m_cancelButton->hide();
	setFixedHeight(195);
	setStyleSheet(Settings::instance().getCurrentStyle().makeStyleSheet(KEY_DIALOG_STYLE_SHEET_TEMPLATE));
	setWindowTitle(tr("Exportar llaves secretas"));
}

KeyDialog::KeyDialog(QWidget* _parent)
  : QDialog(_parent, static_cast<Qt::WindowFlags>(Qt::WindowCloseButtonHint))
  , m_ui(new Ui::KeyDialog)
  , m_isTracking(false)
  , m_isExport(false) {
  m_ui->setupUi(this);
  setWindowTitle(m_isTracking ? tr("Importar llave de seguimiento") : tr("Importar llave"));
  m_ui->m_fileButton->setText(tr("Cargar desde archivo"));
  if (m_isTracking) {
    m_ui->m_descriptionLabel->setText(tr("Importar una llave de seguimiento de un monedero para ver todas sus transacciones entrantes.\n"
      "No permite gastar su saldo."));
  }

  setFixedHeight(195);
}

KeyDialog::~KeyDialog() {
}

QByteArray KeyDialog::getKey() const {
  return QByteArray::fromHex(m_ui->m_keyEdit->toPlainText().toLatin1());
}

void KeyDialog::saveKey() {
  QString filePath = QFileDialog::getSaveFileName(this, m_isTracking ? tr("Guardar llave de seguimiento en...") : tr("Guardar llave en..."),
#ifdef Q_OS_WIN
    QApplication::applicationDirPath(),
#else
    QDir::homePath(),
#endif
    m_isTracking ? tr("Llave de seguimiento (*.trackingkey)") : tr("Llave de monedero (*.walletkey)"));
  if (filePath.isEmpty()) {
    return;
  }

  if (m_isTracking && !filePath.endsWith(".trackingkey")) {
    filePath.append(".trackingkey");
  } else if (!m_isTracking && !filePath.endsWith(".walletkey")) {
    filePath.append(".walletkey");
  }

  QFile keyFile(filePath);
  if (!keyFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return;
  }

  keyFile.write(m_key);
  keyFile.close();
}

void KeyDialog::loadKey() {
  QString filePath = QFileDialog::getOpenFileName(this, m_isTracking ? tr("Cargar llave de seguimiento desde...") : tr("Cargar llave desde..."),
#ifdef Q_OS_WIN
    QApplication::applicationDirPath(),
#else
    QDir::homePath(),
#endif
    m_isTracking ? tr("Llave de seguimiento (*.trackingkey)") : tr("Llave de monedero (*.walletkey)"));
  if (filePath.isEmpty()) {
    return;
  }

  QFile keyFile(filePath);
  if (!keyFile.open(QIODevice::ReadOnly)) {
    return;
  }

  m_key = keyFile.readAll();
  keyFile.close();
  m_ui->m_keyEdit->setPlainText(m_key.toHex().toUpper());
}

void KeyDialog::fileClicked() {
  if (m_isExport) {
    saveKey();
    accept();
  } else {
    loadKey();
  }
}

void KeyDialog::keyChanged() {
  m_isTracking = isTrackingKeys(getKey());
  setWindowTitle(m_isTracking ? tr("Importar llave de seguimiento") : tr("Importar llave"));
  if (m_isTracking) {
    m_ui->m_descriptionLabel->setText(tr("Importar una llave de seguimiento de un monedero para ver todas sus transacciones entrantes.\n"
      "No permite gastar su saldo."));
  } else {
    m_ui->m_descriptionLabel->clear();
  }
}

}
