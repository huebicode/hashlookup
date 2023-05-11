#include "customdelegate.h"
#include "Column.h"

#include <QPainter>
#include <QAbstractItemView>

CustomDelegate::CustomDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
    tableView = qobject_cast<QAbstractItemView *>(this->parent());
    if(tableView)
    {
        connect(tableView->model(), &QAbstractItemModel::rowsRemoved, this, &CustomDelegate::updateAnyDuplicates);
    }
}

void CustomDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(tableView)
    {
        QModelIndex hover = tableView->indexAt(tableView->viewport()->mapFromGlobal(QCursor::pos()));
        if(hover.row() == index.row())
        {
            QColor custom_color(150,170,200,50);
            painter->fillRect(option.rect, custom_color);
        }
        else
        {
            QList<int> columns_to_check = {
                Column::SHA256,
                Column::SHA1,
                Column::MD5
            };

            int used_column = 0;
            QModelIndex column_index;
            QVariant current_value;

            for(int col : columns_to_check)
            {
                if(tableView->model()->headerData(col, Qt::Horizontal).toBool())
                {
                    column_index = index.sibling(index.row(), col);
                    current_value = column_index.data();
                    if(!current_value.toString().contains("-"))
                    {
                        used_column = col;
                        break;
                    }
                }
            }

            static QMap<QString, QColor> value_to_colormap;
            static QVector<QColor> colors = { QColor(100, 255, 100, 100), // green
                                              QColor(100, 100, 255, 100), // blue
                                              QColor(100, 255, 200, 100), // teal
                                              QColor(100, 255, 255, 100), // turquoise
                                              QColor(255, 100, 100, 100), // red
                                              QColor(200, 100, 255, 100), // purple
                                              QColor(255, 100, 200, 100), // pink
                                              QColor(255, 200, 100, 100), // orange
                                              QColor(255, 255, 100, 100), // yellow
                                              QColor(200, 150, 100, 100)  // brown
                                            };

            if(!current_value.isNull() && !current_value.toString().isEmpty() && !(current_value.toString() == "-"))
            {
                bool has_duplicate = false;

                for(int i = 0; i < tableView->model()->rowCount(); ++i)
                {
                    if(i != index.row())
                    {
                        QModelIndex other_row_index = tableView->model()->index(i, used_column);
                        QVariant other_value = other_row_index.data();

                        if(current_value == other_value)
                        {
                            has_duplicate = true;
                            emit duplicatesFound();
                            break;
                        }
                    }
                }
                if(has_duplicate)
                {
                    QString value_str = current_value.toString();

                    if(!value_to_colormap.contains(value_str))
                    {
                        value_to_colormap[value_str] = colors[value_to_colormap.size() % colors.size()];
                    }
                    painter->fillRect(option.rect, value_to_colormap[value_str]);
                }
            }
        }
    }
    QStyledItemDelegate::paint(painter, option, index);
}


void CustomDelegate::updateAnyDuplicates()
{
    if(!tableView)
        return;

    bool any_duplicates = false;
    int used_column = getUsedColumn(tableView->model());
    int row_count = tableView->model()->rowCount();

    for(int row1 = 0; row1 < row_count; ++row1)
    {
        for(int row2 = row1 + 1; row2 < row_count; ++row2)
        {
            QModelIndex index1 = tableView->model()->index(row1, used_column);
            QModelIndex index2 = tableView->model()->index(row2, used_column);
            QVariant value1 = index1.data();
            QVariant value2 = index2.data();

            if(value1 == value2)
            {
                any_duplicates = true;
            }
        }
    }
    if(!any_duplicates)
        emit noDuplicatesFound();
}

int CustomDelegate::getUsedColumn(const QAbstractItemModel *tableModel)
{
    QList<int> columns_to_check = {
        Column::SHA256,
        Column::SHA1,
        Column::MD5
    };

    int used_column = 0;
    QModelIndex column_index;
    QVariant current_value;

    for(int col : columns_to_check)
    {
        if(tableModel->headerData(col, Qt::Horizontal).toBool())
        {
            column_index = tableModel->index(0, col);
            current_value = column_index.data();
            if(!current_value.toString().contains("-"))
            {
                used_column = col;
                break;
            }
        }
    }
    return used_column;
}
