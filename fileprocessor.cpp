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
}



void FileProcessor::processFiles(const QList<QUrl> &urls, const int model_row_count)
{
    file_list->clear();
    row_count = model_row_count;

    // get file count
    int file_count = 0;
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
    createModel(*file_list);
}


void FileProcessor::insertFileListData(QHash<QString, QStringList> &file_list, const QString &file_path)
{
    for(int i = 0; i < Column::NUM_COLUMNS; ++i)
        file_list[file_path].append("");

    QFileInfo file_info(file_path);

    QString file_name = file_info.fileName();
    file_list[file_path].replace(Column::FILENAME, file_name);

    QLocale locale;

    file_list[file_path].replace(Column::FILESIZE, locale.toString(file_info.size()));
    file_list[file_path].replace(Column::FILE_EXTENSION, file_info.suffix());
    file_list[file_path].replace(Column::MIMETYPE, getFileType(file_path, true));
    file_list[file_path].replace(Column::FILETYPE, getFileType(file_path, false));
    file_list[file_path].replace(Column::FULLPATH, file_path);
    file_list[file_path].replace(Column::DIRPATH, file_info.dir().dirName() + "/" + file_name);
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


void FileProcessor::createModel(QHash<QString, QStringList> &file_list)
{
    bool too_much_files = (file_list.size() > 1000 ? true : false);

    int file_count = 0;
    int insert_to_row = row_count;

    for(auto it = file_list.constBegin(); it != file_list.constEnd(); ++it)
    {
        emit updateModel(insert_to_row, Column::FILENAME, file_list[it.key()].at(Column::FILENAME));
        emit updateModel(insert_to_row, Column::FILESIZE, file_list[it.key()].at(Column::FILESIZE));
        emit updateModel(insert_to_row, Column::FILE_EXTENSION, file_list[it.key()].at(Column::FILE_EXTENSION));
        emit updateModel(insert_to_row, Column::FILETYPE, file_list[it.key()].at(Column::FILETYPE));
        emit updateModel(insert_to_row, Column::MIMETYPE, file_list[it.key()].at(Column::MIMETYPE));
        emit updateModel(insert_to_row, Column::DIRPATH, file_list[it.key()].at(Column::DIRPATH));
        emit updateModel(insert_to_row, Column::FULLPATH, file_list[it.key()].at(Column::FULLPATH));

        if(too_much_files)
            QThread::msleep(25);

        ++insert_to_row;
        ++file_count;
        emit fileCount(file_count);
    }

    emit finishedProcessing(&file_list);
}


