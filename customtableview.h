#ifndef CUSTOMTABLEVIEW_H
#define CUSTOMTABLEVIEW_H

#include <QObject>
#include <QTableView>

class CustomTableView : public QTableView
{
    Q_OBJECT

public:
    explicit CustomTableView(QWidget *parent = nullptr);

signals:
    void copyRequested(QTableView *tableView, bool copy_headers, bool copy_to_file);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    bool middle_mouse_button_pressed = false;
    bool alt_key_pressed = false;
};

#endif // CUSTOMTABLEVIEW_H
