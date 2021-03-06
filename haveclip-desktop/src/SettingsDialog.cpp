/*
  HaveClip

  Copyright (C) 2013-2016 Jakub Skokan <aither@havefun.cz>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QDebug>
#include <QFileDialog>
#include <QClipboard>
#include <QMessageBox>

#include "Settings.h"
#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"
#include "CertificateTrustDialog.h"
#include "NodeModel.h"
#include "NodeAddWizard.h"
#include "NodeDialog.h"
#include "Node.h"
#include "Network/ConnectionManager.h"
#include "CertificateInfo.h"

SettingsDialog::SettingsDialog(ConnectionManager *conman, QWidget *parent) :
        QDialog(parent),
	ui(new Ui::SettingsDialog),
	conman(conman)
{
	ui->setupUi(this);

	connect(ui->resetButton, SIGNAL(clicked()), this, SLOT(resetSettings()));

	connect(ui->nodeAddButton, SIGNAL(clicked()), this, SLOT(addNode()));
	connect(ui->nodeEditButton, SIGNAL(clicked()), this, SLOT(editNode()));
	connect(ui->nodeRemoveButton, SIGNAL(clicked()), this, SLOT(deleteNode()));

	nodeModel = new NodeModel(this);
	ui->nodeListView->setModel(nodeModel);

	connect(ui->nodeListView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(editNode(QModelIndex)));

	connect(ui->addSendMimeFilterButton, SIGNAL(clicked()),
		this, SLOT(addSendMimeFilter()));
	connect(ui->removeSendMimeFilterButton, SIGNAL(clicked()),
		this, SLOT(removeSendMimeFilter()));

	connect(ui->addRecvMimeFilterButton, SIGNAL(clicked()),
		this, SLOT(addRecvMimeFilter()));
	connect(ui->removeRecvMimeFilterButton, SIGNAL(clicked()),
		this, SLOT(removeRecvMimeFilter()));

	initForms();
}

SettingsDialog::~SettingsDialog()
{
	delete ui;
}

void SettingsDialog::apply()
{
	Settings *s = Settings::get();

	// Clipboard
	s->setTrackingEnabled( ui->clipboardTrackingCheckBox->isChecked() );

	// History
	s->setHistoryEnabled( ui->historyGroupBox->isChecked() );
	s->setHistorySize( ui->historySizeSpinBox->value() );
	s->setSaveHistory( ui->historySaveCheckBox->isChecked() );

	s->setSyncMode( (ClipboardManager::SynchronizeMode) ui->synchronizeComboBox->currentIndex() );

	// Pool
	s->setNodes( nodeModel->nodes() );

	// Network
	s->setHostAndPort(
		ui->hostLineEdit->text(),
		ui->portSpinBox->value()
	);

	// Auto discovery
	s->setAllowAutoDiscovery( ui->allowDiscoveryCheckBox->isChecked() );
	s->setNetworkName( ui->networkNameLineEdit->text() );

	// Limits
	s->setMaxSendSize( ui->maxSendSpinBox->value() * 1024 * 1024 );
	s->setMaxReceiveSize( ui->maxRecvSpinBox->value() * 1024 * 1024 );

	// Security
	s->setEncryption( (Communicator::Encryption) ui->encryptionComboBox->currentIndex() );
	s->setCertificatePath( ui->certificateLineEdit->text() );
	s->setPrivateKeyPath( ui->keyLineEdit->text() );

	// Advanced
	s->setSendFilterMode( (Settings::MimeFilterMode) ui->sendMimeFilterModeComboBox->currentIndex() );
	s->setSendFilters(sendMimeFilterModel->stringList());

	s->setReceiveFilterMode( (Settings::MimeFilterMode) ui->recvMimeFilterModeComboBox->currentIndex() );
	s->setReceiveFilters(recvMimeFilterModel->stringList());
}

void SettingsDialog::initForms()
{
	Settings *s = Settings::get();

	nodeModel->resetModel();

	// Clipboard
	ui->clipboardTrackingCheckBox->setChecked( s->isTrackingEnabled() );

	// History
	ui->historyGroupBox->setChecked( s->isHistoryEnabled());
	ui->historySizeSpinBox->setValue( s->historySize() );
	ui->historySaveCheckBox->setChecked( s->saveHistory() );

	if(qApp->clipboard()->supportsSelection())
	{
		ui->synchronizeComboBox->setCurrentIndex(s->syncMode());

	} else {
		ui->syncGroupBox->hide();
	}

	// Encryption
	ui->encryptionComboBox->setCurrentIndex(s->encryption());
	ui->certificateLineEdit->setText(s->certificatePath());
	ui->keyLineEdit->setText(s->privateKeyPath());

	connect(ui->certificateButton, SIGNAL(clicked()), this, SLOT(setCertificatePath()));
	connect(ui->keyButton, SIGNAL(clicked()), this, SLOT(setPrivateKeyPath()));
	connect(ui->certificateLineEdit, SIGNAL(textChanged(QString)), this, SLOT(showIdentity()));
	connect(ui->genCertButton, SIGNAL(clicked()), this, SLOT(generateCertificate()));

	showIdentity();

	// Network
	ui->hostLineEdit->setText( s->host() );
	ui->portSpinBox->setValue( s->port() );

	// Auto discovery
	ui->allowDiscoveryCheckBox->setChecked( s->allowAutoDiscovery() );
	ui->networkNameLineEdit->setText( s->networkName() );

	// Limits
	ui->maxSendSpinBox->setValue( s->maxSendSize() / 1024 / 1024 );
	ui->maxRecvSpinBox->setValue( s->maxReceiveSize() / 1024 / 1024 );

	// Advanced
	sendMimeFilterModel = new QStringListModel(Settings::get()->sendFilters(), this);
	recvMimeFilterModel = new QStringListModel(Settings::get()->receiveFilters(), this);

	ui->sendMimeListView->setModel(sendMimeFilterModel);
	ui->receiveMimeListView->setModel(recvMimeFilterModel);

	ui->sendMimeFilterModeComboBox->setCurrentIndex( s->sendFilterMode() );
	ui->recvMimeFilterModeComboBox->setCurrentIndex( s->receiveFilterMode() );
}

void SettingsDialog::addNode()
{
	NodeAddWizard *wizard = new NodeAddWizard(NodeAddWizard::SearchMode, conman, this);
	wizard->exec();
	wizard->deleteLater();
}

void SettingsDialog::editNode(const QModelIndex &index)
{
	Node n = nodeModel->nodeForIndex(index.isValid() ? index : ui->nodeListView->currentIndex());

	NodeDialog *dlg = new NodeDialog(n, conman, this);

	if(dlg->exec() == QDialog::Accepted)
	{
		nodeModel->updateNode(dlg->node());
	}

	dlg->deleteLater();
}

void SettingsDialog::deleteNode()
{
	nodeModel->removeNode(ui->nodeListView->currentIndex());
}

void SettingsDialog::setCertificatePath()
{
	QString path = QFileDialog::getOpenFileName(this, tr("Select certificate file"), "", tr("Certificates (*.crt *.pem)"));

	if(!path.isEmpty())
		ui->certificateLineEdit->setText(path);
}

void SettingsDialog::setPrivateKeyPath()
{
	QString path = QFileDialog::getOpenFileName(this, tr("Select private key file"), "", tr("Private keys (*.key *.pem)"));

	if(!path.isEmpty())
		ui->keyLineEdit->setText(path);
}

void SettingsDialog::showIdentity()
{
	QList<QSslCertificate> certs = QSslCertificate::fromPath(ui->certificateLineEdit->text());

	if(certs.empty())
	{
		ui->certInvalidLabel->show();
		ui->identityGroupBox->hide();

	} else {
		CertificateInfo info(certs.first());

		// issued to
		ui->toCommonNameLabel->setText( info.commonName() );
		ui->toOrgLabel->setText( info.organization() );
		ui->toOrgUnitLabel->setText( info.organizationUnit() );
		ui->serialLabel->setText( info.serialNumber() );

		// validity
		ui->issuedOnLabel->setText( info.issuedOn().toString("d/M/yyyy") );
		ui->expiresLabel->setText( info.expiryDate().toString("d/M/yyyy") );

		// fingerprints
		ui->sha1FingerLabel->setText( info.sha1Digest() );
		ui->md5FingerLabel->setText( info.md5Digest() );

		ui->certInvalidLabel->hide();
		ui->identityGroupBox->show();
	}
}

void SettingsDialog::generateCertificate()
{
	bool keyExists = QFile::exists(ui->keyLineEdit->text());
	bool certExists = QFile::exists(ui->certificateLineEdit->text());

	if(keyExists || certExists)
	{
		QString msg;

		if(keyExists && certExists)
			msg = tr("Private key and certificate");
		else if(keyExists)
			msg = tr("Private key");
		else
			msg = tr("Certificate");

		if(QMessageBox::question(this,
				      tr("File already exists"),
				      tr("%1 already exists.\n\nDo you want to overwrite it?").arg(msg),
				      QMessageBox::Ok | QMessageBox::No,
				      QMessageBox::No
		) != QMessageBox::Ok)
			return;
	}

	CertificateGeneratorDialog genDlg(this);

	if(genDlg.exec() == QDialog::Accepted)
	{
		genDlg.savePrivateKey(ui->keyLineEdit->text());
		genDlg.saveCertificate(ui->certificateLineEdit->text());

		Settings::get()->reloadIdentity();

		showIdentity();
	}
}

void SettingsDialog::addSendMimeFilter()
{
	const int cnt = sendMimeFilterModel->rowCount();

	if(sendMimeFilterModel->insertRow(cnt))
	{
		ui->sendMimeListView->edit(sendMimeFilterModel->index(cnt));
	}
}

void SettingsDialog::removeSendMimeFilter()
{
	QModelIndex i = ui->sendMimeListView->currentIndex();

	if(!i.isValid())
		return;

	sendMimeFilterModel->removeRow(i.row());
}

void SettingsDialog::addRecvMimeFilter()
{
	const int cnt = recvMimeFilterModel->rowCount();

	if(recvMimeFilterModel->insertRow(cnt))
	{
		ui->receiveMimeListView->edit(recvMimeFilterModel->index(cnt));
	}
}

void SettingsDialog::removeRecvMimeFilter()
{
	QModelIndex i = ui->receiveMimeListView->currentIndex();

	if(!i.isValid())
		return;

	recvMimeFilterModel->removeRow(i.row());
}

void SettingsDialog::resetSettings()
{
	if(QMessageBox::question(this, tr("Reset settings to defaults"),
			      tr("Do you really want to reset settings to defaults?"),
				QMessageBox::Ok | QMessageBox::No, QMessageBox::No)
		== QMessageBox::Ok)
	{
		Settings::get()->reset();
		initForms();
	}
}
