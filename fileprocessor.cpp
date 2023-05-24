#include "Column.h"
#include "fileprocessor.h"
#include "libmagic/magic.h"

#include <QCoreApplication>
#include <QDirIterator>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QThread>


FileProcessor::FileProcessor(QObject *parent) : QObject(parent)
{
    file_list = new QHash<QString, QStringList>();
}

FileProcessor::~FileProcessor()
{
    if(file_list)
        delete file_list;

    yr_finalize();
    delete scanner;
}



void FileProcessor::processFiles(const QList<QUrl> &urls, bool yara)
{
    file_list->clear();
    yara_active = yara;

    // get file count
    file_count = 0;
    for(const QUrl &url :urls)
    {
        if(url.isLocalFile())
        {
            QString file_path = url.toLocalFile();
            QFileInfo file_info(file_path);

            if(file_info.isDir())
            {
                QDirIterator dir_iter(file_path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
                while(dir_iter.hasNext())
                {
                    dir_iter.next();
                    ++file_count;
                }
            }
            else if(file_info.isFile())
            {
                ++file_count;
            }
        }
    }
    emit fileCountSum(file_count);


    // process files
    file_counter = 0;

    for(const QUrl &url : urls)
    {
        if(url.isLocalFile())
        {
            QString file_path = url.toLocalFile();
            QFileInfo file_info(file_path);

            if(file_info.isDir())
            {
                QDirIterator dir_iter(file_path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
                while(dir_iter.hasNext())
                {
                    QString iter_path = dir_iter.next();

                    insertFileListData(*file_list, iter_path);
                }
            }
            else if(file_info.isFile())
            {
                insertFileListData(*file_list, file_path);
            }
        }
    }

    emit finishedProcessing(file_list);
}


void FileProcessor::insertFileListData(QHash<QString, QStringList> &file_list, const QString &file_path)
{
    bool large_file_count = (file_count > 1000 ? true : false);

    for(int i = 0; i < Column::NUM_COLUMNS; ++i)
        file_list[file_path].append("");

    QFileInfo file_info(file_path);

    QString file_name = file_info.fileName();
    file_list[file_path].replace(Column::FILENAME, file_name);

    if(yara_active)
    {
        file_list[file_path].replace(Column::YARA, scanner->scanFile(file_path));
    }

    QLocale locale;
    file_list[file_path].replace(Column::FILESIZE, locale.toString(file_info.size()));

    file_list[file_path].replace(Column::FILE_EXTENSION, file_info.suffix());

    file_list[file_path].replace(Column::MIMETYPE, getFileType(file_path, true));

    file_list[file_path].replace(Column::FILETYPE, getFileType(file_path, false));

    file_list[file_path].replace(Column::FULLPATH, file_path);

    file_list[file_path].replace(Column::DIRPATH, file_info.dir().dirName() + "/" + file_name);

    emit updateModel(file_list[file_path]);

    ++file_counter;
    emit fileCount(file_counter);

    if(large_file_count)
        QThread::msleep(10);
}


QString FileProcessor::getFileType(const QString file_path, const bool &mime_type)
{
// hacky workaround with a 64k temp file in Windows, because of libmagics file limit to 2GB on Windows...
#ifdef Q_OS_WIN
    bool large_win_file = false;
    QString temp_file_path;
    QTemporaryFile temp_file;
    QFileInfo file_info(file_path);

    if(file_info.exists() && (file_info.size() > 2147483647))
    {
        large_win_file = true;
        if(temp_file.open())
        {
            QFile large_file(file_path);
            if(large_file.open(QIODevice::ReadOnly))
            {
                const qint64 read_size = 65536;
                QByteArray data = large_file.read(read_size);
                temp_file.write(data);
                temp_file.flush();
                large_file.close();
            }
            else
            {
                qWarning() << "Failed to open large file for reading";
                return "error";
            }
            temp_file_path = temp_file.fileName();
        }
        else
        {
            qWarning() << "Failed to create temporary file";
            return "error";
        }
    }
#endif

    magic_t magic_cookie;
    if(mime_type)
        magic_cookie = magic_open(MAGIC_MIME);
    else
        magic_cookie = magic_open(MAGIC_NONE);

    if(magic_cookie == nullptr)
    {
        qWarning() << "Failed to initialize magic cookie";
    }

    QString app_path = QCoreApplication::applicationDirPath();
    QString magic_mgc_path = app_path + "/db/magic.mgc";

    if(magic_load(magic_cookie, magic_mgc_path.toStdString().c_str()) != 0)
    {
        magic_close(magic_cookie);
        qWarning() << "Failed to load magic database";
    }

    const char *file_type;

#ifdef Q_OS_WIN
    if(large_win_file)
        file_type = magic_file(magic_cookie, temp_file_path.toStdString().c_str());
    else
        file_type = magic_file(magic_cookie, file_path.toStdString().c_str());
#else
    file_type = magic_file(magic_cookie, file_path.toStdString().c_str());
#endif

    if(file_type == nullptr)
    {
        QString error_msg = magic_error(magic_cookie);
        magic_close(magic_cookie);
        qWarning() << "Failed to get file type:" << error_msg;
    }

    QString result(file_type);
    magic_close(magic_cookie);
    return result;
}


void FileProcessor::initializeYara()
{
    if(yr_initialize() != ERROR_SUCCESS)
    {
        qDebug() << "Unable to initialize YARA";
    }

    scanner = new YaraProcessor();

    connect(scanner, &YaraProcessor::yaraError, this, &FileProcessor::yaraError);
    connect(scanner, &YaraProcessor::yaraWarning, this, &FileProcessor::yaraWarning);
    connect(scanner, &YaraProcessor::yaraSuccess, this, &FileProcessor::yaraSuccess);

    yara_dir = QCoreApplication::applicationDirPath() + "/YARA";

    if(!yara_dir.exists())
        yara_dir.mkpath(".");

    loadAndCompileYaraRules(yara_dir.absolutePath());
}

void FileProcessor::loadAndCompileYaraRules(const QString &yara_dir_path)
{
    scanner->clearRules();

    // load copiled rules
    QDir compiled_rules_dir(yara_dir_path + "/Compiled");

    if(!compiled_rules_dir.exists())
        compiled_rules_dir.mkpath(".");

    QDirIterator it_compiled(compiled_rules_dir.absolutePath(), QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);

    QStringList compiled_rule_list;

    while(it_compiled.hasNext())
    {
        QString file_path = it_compiled.next();
        compiled_rule_list.append(file_path);
    }

    scanner->loadCompiledYaraRules(compiled_rule_list);


    // compile and load text rules
    QDir text_rules_dir(yara_dir_path + "/Rules");

    if(!text_rules_dir.exists())
        text_rules_dir.mkpath(".");

    QDirIterator it_text(text_rules_dir.absolutePath(), QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);

    QStringList text_rule_list;

    while(it_text.hasNext())
    {
        QString file_path = it_text.next();
        text_rule_list.append(file_path);
    }

    scanner->compileYaraRules(text_rule_list);

    emit finishedLoadingYaraRules();
}


