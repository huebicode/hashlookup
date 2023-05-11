#ifndef CUSTOMDELEGATE_H
#define CUSTOMDELEGATE_H

#include <QStyledItemDelegate>

class CustomDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    CustomDelegate(QObject *parent = nullptr);

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

signals:
    void duplicatesFound() const;
    void noDuplicatesFound();

private slots:
    void updateAnyDuplicates();

private:
    int getUsedColumn(const QAbstractItemModel *tableModel);

    QAbstractItemView *tableView;
};

#endif // CUSTOMDELEGATE_H
