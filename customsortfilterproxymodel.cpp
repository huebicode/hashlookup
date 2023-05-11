#include "customsortfilterproxymodel.h"
#include "Column.h"

CustomSortFilterProxyModel::CustomSortFilterProxyModel(QTableView *tableView, QObject *parent)
    : QSortFilterProxyModel(parent),
      m_tableView(tableView)
{

}

void CustomSortFilterProxyModel::setNumericSortingColumns(const QSet<int> &columns)
{
    numericSortingColumns = columns;
}

void CustomSortFilterProxyModel::setFilterDuplicates(bool filter)
{
    m_filter_duplicates = filter;
    invalidateFilter();
}

bool CustomSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index;
    bool regexFilterPassed = false;

    for(int col = 0; col < sourceModel()->columnCount(); ++col)
    {
        if(!m_tableView->isColumnHidden(col))
        {
            index = sourceModel()->index(sourceRow, col, sourceParent);

            if(index.isValid() && sourceModel()->data(index).toString().contains(filterRegularExpression()))
            {
                regexFilterPassed = true;
                break;
            }
        }
    }

    if(!regexFilterPassed)
        return false;

    if(m_filter_duplicates)
    {
        int visible_column = 0;

        QList<int> columns_to_check = {
            Column::SHA256,
            Column::SHA1,
            Column::MD5
        };

        for(int col : columns_to_check)
        {
            if(!m_tableView->isColumnHidden(col))
            {
                QModelIndex hash_index = sourceModel()->index(sourceRow, col, sourceParent);
                QString hash_values = sourceModel()->data(hash_index).toString();
                if(!hash_values.contains("-"))
                {
                    visible_column = col;
                    break;
                }
            }
        }

        if(visible_column)
        {
            QModelIndex hash_index = sourceModel()->index(sourceRow, visible_column, sourceParent);
            QString hash_values = sourceModel()->data(hash_index).toString();


            for (int i = 0; i < sourceRow; ++i)
            {
                QModelIndex other_hash_index = sourceModel()->index(i, visible_column);

                QString other_hash_values = sourceModel()->data(other_hash_index).toString();

                if (hash_values == other_hash_values && !(hash_values.isEmpty() || hash_values == "-"))
                {
                    return false;
                }
            }
        }
    }

    return true;
}


bool CustomSortFilterProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    if(numericSortingColumns.contains(source_left.column()))
    {
        bool ok1, ok2;
        double left = sourceModel()->data(source_left, Qt::UserRole).toDouble(&ok1);
        double right = sourceModel()->data(source_right, Qt::UserRole).toDouble(&ok2);

        if(ok1 && ok2)
            return left < right;
    }
    return QSortFilterProxyModel::lessThan(source_left, source_right);
}

