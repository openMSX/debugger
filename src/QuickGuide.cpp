#include "QuickGuide.h"
#include "ui_QuickGuide.h"


QuickGuide::QuickGuide(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QuickGuide)
{
    ui->setupUi(this);
}

QuickGuide::~QuickGuide()
{
    delete ui;
}
