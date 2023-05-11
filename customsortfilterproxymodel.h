#ifndef CUSTOMSORTFILTERPROXYMODEL_H
#define CUSTOMSORTFILTERPROXYMODEL_H

#include <QTableView>
#include <QObject>
#include <QSortFilterProxyModel>

class CustomSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    CustomSortFilterProxyModel(QTableView *tableView, QObject *parent = nullptr);

    void setNumericSortingColumns(const QSet<int> &columns);
    void setFilterDuplicates(bool filter);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

private:

    QTableView *m_tableView;
    QSet<int> numericSortingColumns;
    bool m_filter_duplicates = false;
};

#endif // CUSTOMSORTFILTERPROXYMODEL_H
