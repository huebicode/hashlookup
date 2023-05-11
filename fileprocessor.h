#ifndef FILEPROCESSOR_H
#define FILEPROCESSOR_H

#include <QHash>
#include <QObject>
#include <QStandardItemModel>
#include <QUrl>

class FileProcessor : public QObject
{
    Q_OBJECT
public:
    explicit FileProcessor(QObject *parent = nullptr);
    ~FileProcessor();

public slots:
    void processFiles(const QList<QUrl> &urls, int model_row_count);

signals:
    void startProcessing(const QList<QUrl> &urls, int model_row_count);
    void fileCountSum(int count);
    void fileCount(int count);
    void updateModel(int row, int col, const QString &data);
    void finishedProcessing(const QHash<QString, QStringList> *file_list);

private:
    void insertFileListData(QHash<QString, QStringList> &file_list, const QString &file_path);
    void createModel(QHash<QString, QStringList> &file_list);
    QString getFileType(const QString file_path, const bool &mime_type);

    QHash<QString, QStringList> *file_list;
    int row_count;
};

#endif // FILEPROCESSOR_H
