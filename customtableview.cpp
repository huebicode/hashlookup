#include "customtableview.h"

#include <QKeyEvent>
#include <QMouseEvent>

CustomTableView::CustomTableView(QWidget *parent)
    : QTableView(parent)
{
}

void CustomTableView::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape)
    {
        clearSelection();
        return;
    }
    else if(event->key() == Qt::Key_Delete)
    {
        QModelIndexList selected_rows = selectionModel()->selectedRows();
        if(selected_rows.count() > 0)
        {
            std::sort(selected_rows.begin(), selected_rows.end(), [](const QModelIndex &a, const QModelIndex &b)
            {
                return a.row() > b.row();
            });

            setSortingEnabled(false);
            for(const QModelIndex &index : selected_rows)
            {
                model()->removeRow(index.row());
            }

            setSortingEnabled(true);
        }
        return;
    }
    else if(event->matches(QKeySequence::Copy))
    {
        emit copyRequested(this, false, false);
        return;
    }
    else if(event->key() == Qt::Key_Alt)
    {
        alt_key_pressed = true;
    }
    else
        QTableView::keyPressEvent(event);
}

void CustomTableView::keyReleaseEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Alt)
    {
        alt_key_pressed = false;
    }
}


void CustomTableView::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::MiddleButton)
    {
        middle_mouse_button_pressed = true;
        setSelectionBehavior(QAbstractItemView::SelectColumns);
    }
    else if(event->button() == Qt::LeftButton)
    {
        setSelectionBehavior(alt_key_pressed ? QAbstractItemView::SelectItems : QAbstractItemView::SelectRows);
    }

    QTableView::mousePressEvent(event);
}


void CustomTableView::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::MiddleButton)
    {
        middle_mouse_button_pressed = false;
        setSelectionBehavior(QAbstractItemView::SelectRows);
    }

    QTableView::mouseReleaseEvent(event);
}


void CustomTableView::mouseMoveEvent(QMouseEvent *event)
{
    if(middle_mouse_button_pressed)
    {
        QPoint pos = event->pos();
        QModelIndex index = indexAt(pos);
        if(index.isValid())
            selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Columns);
    }

    QTableView::mouseMoveEvent(event);
}

