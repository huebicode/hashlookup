#ifndef ITEMPROCESSOR_H
#define ITEMPROCESSOR_H

#include <QObject>
#include <QFutureWatcher>
#include <QMultiHash>
#include <QFile>

class ItemProcessor : public QObject {
    Q_OBJECT
public:
    explicit ItemProcessor(QObject *parent = nullptr);

    ~ItemProcessor();

    void startProcessing(const QHash<QString, QStringList> *file_list, const bool &md5, const bool &sha1, const bool &sha256);

signals:
    void processingFinished(const QString &results);
    void resultReady(const QString result);

private:
    static QString processItem(const QString &item);
    static QByteArray hashFile(QFile &file, const QString &algorithm);

    void onStarted();
    void onResultReady(int index);
    void onFinished();

    QStringList m_item_list;
    QFutureWatcher<QString> *m_futureWatcher;
    QElapsedTimer m_timer;
};

#endif // ITEMPROCESSOR_H
