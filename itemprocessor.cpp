#include "itemprocessor.h"
#include "openssl/evp.h"

#include <QtConcurrent>
#include <QTime>


ItemProcessor::ItemProcessor(QObject *parent)
    : QObject(parent)
    , m_futureWatcher(new QFutureWatcher<QString>(this))
{
    connect(m_futureWatcher, &QFutureWatcher<void>::started, this, &ItemProcessor::onStarted);
    connect(m_futureWatcher, &QFutureWatcher<int>::resultReadyAt, this, &ItemProcessor::onResultReady);
    connect(m_futureWatcher, &QFutureWatcher<void>::finished, this, &ItemProcessor::onFinished);
}

ItemProcessor::~ItemProcessor()
{
    m_futureWatcher->cancel();
    m_futureWatcher->waitForFinished();
}


void ItemProcessor::startProcessing(const QHash<QString, QStringList> *file_list, const bool &md5, const bool &sha1, const bool &sha256)
{
    m_item_list.clear();

    for(auto it = file_list->constBegin(); it != file_list->constEnd(); ++it)
    {
        if(md5)
            m_item_list.append(it.key() + "\t" + "MD5");
        if(sha1)
            m_item_list.append(it.key() + "\t" + "SHA1");
        if(sha256)
            m_item_list.append(it.key() + "\t" + "SHA256");
    }

    QFuture<QString> future = QtConcurrent::mapped(m_item_list, ItemProcessor::processItem);
    m_futureWatcher->setFuture(future);
}


void ItemProcessor::onStarted()
{
    m_timer.start();
}


QString ItemProcessor::processItem(const QString &arguments)
{
    QStringList argument = arguments.split("\t");
    QString path = argument.at(0);
    QString algorithm = argument.at(1);

    QFile file(path);
    if(file.open(QIODevice::ReadOnly))
    {
        QByteArray result = hashFile(file, algorithm);
        QString hash_str = QString(result.toHex());
        result.clear();

        return algorithm + "\t" + hash_str + "\t" + path;
    }

    return QString("Error: Could't open file: %1").arg(path);
}


QByteArray ItemProcessor::hashFile(QFile &file, const QString &algorithm)
{
    const int CHUNK_SIZE = 8192;
    unsigned char buffer[CHUNK_SIZE];
    QByteArray result;

    const EVP_MD *md;
    if(algorithm == "MD5")
        md = EVP_md5();
    else if(algorithm == "SHA1")
        md = EVP_sha1();
    else if(algorithm == "SHA256")
        md = EVP_sha256();
    else
        md = nullptr;

    if (md == nullptr)
        return QByteArray();

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int digest_lenth;

    if (mdctx == nullptr)
        qWarning() << "mdctx == nullptr";


    if (EVP_DigestInit_ex(mdctx, md, NULL) != 1)
        qWarning() << "EVP_DigestInit_ex != 1";


    while(!file.atEnd())
    {
        qint64 bytes_read = file.read((char*)buffer, CHUNK_SIZE);
        if(EVP_DigestUpdate(mdctx, buffer, bytes_read) != 1)
            qWarning() << "EVP_DigestUpdate != 1";
    }

    if(EVP_DigestFinal_ex(mdctx, hash, &digest_lenth) != 1)
        qWarning() << "VP_DigestFinal_ex != 1";

    result = QByteArray(reinterpret_cast<char *>(hash), digest_lenth);

    EVP_MD_CTX_free(mdctx);

    return result;
}


void ItemProcessor::onResultReady(int index) {
    QString result = m_futureWatcher->resultAt(index);
    emit resultReady(result);
}


void ItemProcessor::onFinished()
{
    QString total_time = QString::number(m_timer.elapsed() / 1000.0);
    emit processingFinished(total_time);
}
