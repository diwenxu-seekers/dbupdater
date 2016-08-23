#include "infobox.h"
#include "ui_infobox.h"

InfoBox::InfoBox(QWidget *parent) : QDialog(parent), ui(new Ui::InfoBox)
{
    ui->setupUi(this);

    // Borderless dialog
    this->setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
}

InfoBox::~InfoBox()
{
    delete ui;
}

void InfoBox::setText(const QString &msg)
{
    ui->lbMsg->setText(msg);
}
