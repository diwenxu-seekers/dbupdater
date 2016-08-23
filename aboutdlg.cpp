#include "aboutdlg.h"
#include "ui_aboutdlg.h"
#include "common.h"
#include <QMouseEvent>
#include <QStringBuilder>


AboutDlg::AboutDlg(QWidget *parent) : QDialog(parent), ui(new Ui::AboutDlg)
{
    ui->setupUi(this);

    // Borderless dialog
    this->setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);

    // Application name
    ui->lbAppName->setText(QApplication::applicationName());

    // About
    ui->lbAbout->setText(QString(tr("About %1")).arg(QApplication::applicationName()));

    QString sDateTime = QString(BUILD_DATETIME);
    sDateTime.replace('_', ' ');

    QString sInfo = tr("Version ") + QCoreApplication::applicationVersion();
    sInfo += tr(", build ") + QString(GIT_VERSION) + tr(", ") + sDateTime;
    sInfo += tr("<br>") + tr("Copyright(c) ") + QString(COPYRIGHTS_YEAR) + tr(" ") + QString(ORGANIZATION_NAME);
    sInfo += tr("<br>") + QString(COPYRIGHTS_STATEMENT);
    ui->lbInfo->setText(sInfo);

    // Ok button
    connect(ui->btnOk, SIGNAL(clicked()), this, SLOT(close()));
}

AboutDlg::~AboutDlg()
{
    delete ui;
}

void AboutDlg::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        _dragPos = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void AboutDlg::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        move(event->globalPos() - _dragPos);
        event->accept();
    }
}
