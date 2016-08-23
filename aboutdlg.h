#ifndef ABOUTDLG_H
#define ABOUTDLG_H

#include <QDialog>


namespace Ui
{
    class AboutDlg;
}

class AboutDlg : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDlg(QWidget *parent = 0);
    ~AboutDlg();

protected:
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

private:
    Ui::AboutDlg *ui;

    QPoint _dragPos;
};

#endif // ABOUTDLG_H
