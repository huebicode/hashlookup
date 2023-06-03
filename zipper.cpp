#include "zipper.h"
#include <quazip/quazipfile.h>


Zipper::Zipper(const QString &archive_path)
{
    zip.setZipName(archive_path);

    if(!zip.open(QuaZip::mdCreate))
    {
        qWarning("Zipper(): zip.open(): %d", zip.getZipError());
    }
}


void Zipper::zipFileList(const QStringList &file_path_list)
{

    if(!zip.isOpen())
        return;

    int counter = 0;

    for(const auto &file_path : file_path_list)
    {
        QFile in_file(file_path);
        if(!in_file.open(QIODevice::ReadOnly))
        {
            qWarning("Failed to open file for reading: %s", qPrintable(file_path));
            return;
        }

        QuaZipFile out_file(&zip);
        QuaZipNewInfo new_info(QFileInfo(file_path).fileName(), file_path);
        if(!out_file.open(QIODevice::WriteOnly, new_info))
        {
            qWarning("Failed to open zip file for writing: %s", qPrintable(file_path));
            return;
        }

        out_file.write(in_file.readAll());
        if(out_file.getZipError() != UNZ_OK)
            qWarning("addFile(): out_file.close(): %d", out_file.getZipError());

        out_file.close();
        if(out_file.getZipError()!=UNZ_OK) {
            qWarning("addFile(): outFile.close(): %d", out_file.getZipError());
        }

        in_file.close();
        if(in_file.error() != QFile::NoError)
            qWarning("addFile(): in_file.close(): %d", in_file.error());

        emit fileFinished(++counter);
    }

    close();
}


void Zipper::close()
{
    zip.close();
    if(zip.getZipError() != UNZ_OK)
    {
        qWarning("Zipper(): zip.close(): %d", zip.getZipError());
    }

    emit zippingFinished();
}






