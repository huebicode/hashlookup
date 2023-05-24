#ifndef FILEPROCESSOR_H
#define FILEPROCESSOR_H

#include <QDir>
#include <QHash>
#include <QObject>
#include <QStandardItemModel>
#include <QUrl>

#include "yaraprocessor.h"

class FileProcessor : public QObject
{
    Q_OBJECT
public:
    explicit FileProcessor(QObject *parent = nullptr);
    ~FileProcessor();

public slots:
    void processFiles(const QList<QUrl> &urls, bool yara);

    void initializeYara();
    void loadAndCompileYaraRules(const QString &yara_dir_path);


signals:
    void startProcessing(const QList<QUrl> &urls, bool yara);
    void fileCountSum(int count);
    void fileCount(int count);
    void updateModel(const QStringList &data);
    void finishedProcessing(const QHash<QString, QStringList> *file_list);

    void startInitializingYara();
    void startLoadingCompilingYaraRules(const QString &yara_dir_path);
    void finishedLoadingYaraRules();

    void yaraSuccess(QString success);
    void yaraError(QString error);
    void yaraWarning(QString warning);


private:
    void insertFileListData(QHash<QString, QStringList> &file_list, const QString &file_path);

    QString getFileType(const QString file_path, const bool &mime_type);

    QHash<QString, QStringList> *file_list;

    YaraProcessor *scanner;
    QDir yara_dir;
    bool yara_active;

    int file_count;
    int file_counter;
};

#endif // FILEPROCESSOR_H
