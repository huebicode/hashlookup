#ifndef ZIPPER_H
#define ZIPPER_H

#include <quazip/quazip.h>
#include <QObject>


class Zipper : public QObject
{
    Q_OBJECT

public:
    explicit Zipper(const QString &archive_path);

signals:
    void startZipping(const QStringList &file_path_list);

    void fileFinished(int counter);

    void zippingFinished();

public slots:
    void zipFileList(const QStringList &file_path_list);

    void close();

private:
    QuaZip zip;
};

#endif // ZIPPER_H
