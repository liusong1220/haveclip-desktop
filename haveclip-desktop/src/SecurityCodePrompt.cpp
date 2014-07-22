#include "SecurityCodePrompt.h"
#include "ui_SecurityCodePrompt.h"

#include <QMessageBox>

#include "Network/ConnectionManager.h"
#include "Node.h"

SecurityCodePrompt::SecurityCodePrompt(const Node &n, ConnectionManager *conman, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::SecurityCodePrompt),
	m_conman(conman)
{
	ui->setupUi(this);

	connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
	connect(ui->verifyButton, SIGNAL(clicked()), this, SLOT(verifyCode()));
	connect(m_conman, SIGNAL(verificationFinished(bool)), this, SLOT(verificationFinish(bool)));
	connect(m_conman, SIGNAL(verificationFailed(Communicator::CommunicationStatus)), this, SLOT(verificationFailed(Communicator::CommunicationStatus)));

	ui->infoLabel->setText( ui->infoLabel->text().arg(n.name()).arg(n.host()).arg(n.port()) );
	ui->errorLabel->hide();
}

SecurityCodePrompt::~SecurityCodePrompt()
{
	delete ui;
}

void SecurityCodePrompt::verifyCode()
{
	ui->verifyButton->setText(tr("Verifying..."));
	ui->verifyButton->setEnabled(false);

	m_conman->provideSecurityCode(ui->codeLineEdit->text());
}

void SecurityCodePrompt::verificationFinish(bool ok)
{
	if(ok)
	{
		accept();

	} else {
		ui->errorLabel->show();
		ui->verifyButton->setText(tr("Verify"));
		ui->verifyButton->setEnabled(true);
	}
}

void SecurityCodePrompt::verificationFailed(Communicator::CommunicationStatus status)
{
	QMessageBox::critical(
		this,
		tr("Connection failed"),
		tr("Unable to establish connection: %1").arg(Communicator::statusToString(status))
	);

	reject();
}
