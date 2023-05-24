#include "Column.h"
#include "widget.h"
#include "ui_widget.h"
#include "customdelegate.h"
#include "customtableview.h"

#include <QClipboard>
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QLabel>
#include <QMimeData>
#include <QMovie>
#include <QRegularExpression>
#include <QFileDialog>
#include <QTimer>
#include <QSettings>

#include <QSvgWidget>
#include <QDesktopServices>


Widget::Widget(QWidget *parent)
    : QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    md5 = true;
    sha1 = true;
    sha256 = true;
    yara = false;
    show_filesize = true;
    show_extension = true;
    show_mimetype = true;
    show_filetype = true;
    show_dirpath = true;
    show_fullpath = true;

    doubles_found = false;

    setAcceptDrops(true);

    ui->progressBar->hide();
    ui->lbl_status->hide();
    ui->lbl_clock->hide();
    ui->stackedWidget->setCurrentWidget(ui->page_drop);

    layer = new QWidget(ui->tableView);

    model = new QStandardItemModel(ui->tableView);

    proxyModel = new CustomSortFilterProxyModel(ui->tableView, this);
    proxyModel->setSourceModel(model);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    QSet<int> numeric_columns = {Column::FILESIZE};
    proxyModel->setNumericSortingColumns(numeric_columns);

    proxyModel->setDynamicSortFilter(false);

    headerSortingAdapter = new HeaderSortingAdapter(ui->tableView);

    ui->tableView->setModel(proxyModel);
    ui->tableView->setSortingEnabled(true);
    ui->tableView->horizontalHeader()->setSectionsMovable(true);
    ui->tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tableView->setIconSize(QSize(13,13));

    CustomDelegate *custom_delegate = new CustomDelegate(ui->tableView);
    ui->tableView->setItemDelegate(custom_delegate);

    connect(model, &QStandardItemModel::itemChanged, this, &Widget::onCellItemChanged);
    connect(model, &QStandardItemModel::itemChanged, this, &Widget::showFileStatistics);
    connect(model, &QAbstractItemModel::rowsRemoved, this, &Widget::showFileStatistics);

    connect(ui->tableView, &CustomTableView::copyRequested, this, &Widget::copySelectedCells);
    connect(ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &Widget::showFileStatistics);
    connect(ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &Widget::showSelectedFiles);

    connect(custom_delegate, &CustomDelegate::duplicatesFound, this, &Widget::onDoublesFound);
    connect(custom_delegate, &CustomDelegate::noDuplicatesFound, this, &Widget::onMissingDoubles);

    connect(ui->stackedWidget, &QStackedWidget::currentChanged, this, &Widget::onPageChanged);

    fileProcessor = new FileProcessor();
    QThread *file_processor_thread = new QThread();
    fileProcessor->moveToThread(file_processor_thread);

    connect(fileProcessor, &FileProcessor::startProcessing, fileProcessor, &FileProcessor::processFiles);
    connect(fileProcessor, &FileProcessor::fileCountSum, this, &Widget::onFileProcessorFileCountSum);
    connect(fileProcessor, &FileProcessor::fileCount, this, &Widget::onFileProcessorFileCount);
    connect(fileProcessor, &FileProcessor::updateModel, this, &Widget::onFileProcessorUpdateModel);
    connect(fileProcessor, &FileProcessor::finishedProcessing, this, &Widget::onFileProcessingFinished);

    connect(fileProcessor, &FileProcessor::startInitializingYara, fileProcessor, &FileProcessor::initializeYara);
    connect(fileProcessor, &FileProcessor::startLoadingCompilingYaraRules, fileProcessor, &FileProcessor::loadAndCompileYaraRules);
    connect(fileProcessor, &FileProcessor::finishedLoadingYaraRules, this, &Widget::onFinishedLoadingYaraRules);
    connect(fileProcessor, &FileProcessor::yaraSuccess, this, &Widget::onYaraSuccess);
    connect(fileProcessor, &FileProcessor::yaraError, this, &Widget::onYaraError);
    connect(fileProcessor, &FileProcessor::yaraWarning, this, &Widget::onYaraWarning);

    file_processor_thread->start();

    processor = new ItemProcessor(this);
    connect(processor, &ItemProcessor::resultReady, this, &Widget::onResultReady);
    connect(processor, &ItemProcessor::processingFinished, this, &Widget::onProcessingFinished);

    movie = new QMovie(":/img/loading.gif", QByteArray(), ui->tableView);
    ui->lbl_loading->setMovie(movie);
    movie->start();

    file_icon = QIcon(":/img/file.svg");

    readSettings();

    ui->btn_md5->setToolTip("MD5");
    ui->btn_sha1->setToolTip("SHA1");
    ui->btn_sha256->setToolTip("SHA256");
    ui->btn_yara->setToolTip("YARA");

    ui->btn_hide_doubles->setToolTip("Filter Out Doubles");
    ui->btn_filesize->setToolTip("Show Filesize");
    ui->btn_extension->setToolTip("Show Extension");
    ui->btn_mimetype->setToolTip("Show MIME Type");
    ui->btn_filetype->setToolTip("Show Filetype");
    ui->btn_dirpath->setToolTip("Show Dirpath");
    ui->btn_fullpath->setToolTip("Show Fullpath");
    ui->btn_clipboard->setToolTip("Copy To Clipboard");
    ui->btn_save->setToolTip("Save As .tsv");
    ui->btn_clear->setToolTip("Clear Table");
    ui->btn_options->hide();
    ui->btn_about->setToolTip("Show Info");

    ui->btn_match_case->setToolTip("Case Sensitive");
    ui->btn_whole_word->setToolTip("Whole Words");
    ui->btn_regex->setToolTip("Regex");

    ui->btn_yara_status->setToolTip("Show YARA Output");

    hideButtons();

    drop_area_svg = new QSvgWidget(this);
    drop_area_svg->setFixedSize(QSize(450,300));
    ui->layout_drop->addWidget(drop_area_svg);
    drop_area_svg->load(QString(":/img/drop-area-normal.svg"));

    info_text_svg = new QSvgWidget(this);
    info_text_svg->setFixedSize(QSize(790,1099));
    ui->info_layout->addWidget(info_text_svg);
    info_text_svg->load(QString(":/img/info-text.svg"));

    yara_icon_red = QIcon(":/img/red.svg");
    yara_icon_orange = QIcon(":/img/orange.svg");
    yara_icon_green = QIcon(":/img/green.svg");
    yara_icon_grey = QIcon(":/img/grey.svg");

    ui->btn_open_yaradir->setIcon(QIcon(":/img/dir.svg"));
    ui->btn_reload_rules->setIcon(QIcon(":/img/reload.svg"));

    yara_init = false;
    ui->textEdit_error->hide();
    ui->textEdit_warning->hide();
    ui->textEdit_success->hide();
    ui->lbl_yara_error->hide();
    ui->lbl_no_rules_found->hide();

    ui->btn_yara_status->hide();
    ui->btn_yara_status_main->hide();
    ui->btn_reload_rules->hide();
    ui->btn_open_yaradir->hide();
}


Widget::~Widget()
{
    delete ui;
    writeSettings();
}


void Widget::dragEnterEvent(QDragEnterEvent *event)
{
    if(ui->stackedWidget->currentWidget() == ui->page_about
        || ui->stackedWidget->currentWidget() == ui->page_options
        || ui->stackedWidget->currentWidget() == ui->page_yara)
        event->ignore();
    else
    {
        if(ui->stackedWidget->currentWidget() == ui->page_main)
        {
            layer->resize(ui->tableView->size());
            layer->setStyleSheet("background-color: rgba(118,164,189,0.5);");
            layer->show();
        }
        else if(ui->stackedWidget->currentWidget() == ui->page_drop)
        {
            drop_area_svg->load(QString(":/img/drop-area-hover.svg"));
        }

        if(event->mimeData()->hasUrls())
            event->acceptProposedAction();
    }
}


void Widget::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
    layer->hide();
    drop_area_svg->load(QString(":/img/drop-area-normal.svg"));
}


void Widget::dropEvent(QDropEvent *event)
{
    row_count = model->rowCount();

    drop_area_svg->load(QString(":/img/drop-area-normal.svg"));
    urls = event->mimeData()->urls();

    if(urls.isEmpty())
        return;

    bool all_dirs_empty = true;
    for(const QUrl &url : urls)
    {
        QString local_path(url.toLocalFile());
        QFileInfo file_info(local_path);

        if(file_info.isFile() || (file_info.isDir() && dir_contains_file(QDir(local_path))))
        {
            all_dirs_empty = false;
            break;
        }
    }

    if(all_dirs_empty)
        return;

    setAcceptDrops(false);

    file_hash_list.clear();

    processed_items = 0;

    doubles_found = false;
    page_main_visible = false;

    ui->stackedWidget->setCurrentWidget(ui->page_loading);

    ui->tableView->setMouseTracking(true);
    ui->tableView->setSortingEnabled(false);
    ui->tableView->setFocus();
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->verticalHeader()->setVisible(false);

    ui->frame_search->hide();
    ui->lineEdit_search->clear();

    ui->lbl_status->hide();
    ui->lbl_status->clear();
    ui->lbl_clock->hide();

    ui->btn_hide_doubles->setChecked(false);
    ui->btn_hide_doubles->setEnabled(false);
    ui->btn_clear->hide();
    ui->btn_clipboard->hide();
    ui->btn_save->hide();

    layer->hide();

    model->setColumnCount(Column::NUM_COLUMNS);

    QStringList header_labels;
    header_labels << "Filename"
                  << (md5 ? "MD5" : "")
                  << (sha1 ? "SHA1" : "")
                  << (sha256 ? "SHA256" : "")
                  << (yara ? "YARA" : "")
                  << (show_filesize ? "Filesize" : "")
                  << (show_extension ? "Ext" : "")
                  << (show_mimetype ? "MIME type" : "")
                  << (show_filetype ? "Filetype" : "")
                  << (show_dirpath ? "Dirpath" : "")
                  << (show_fullpath ? "Fullpath" : "")
        ;
    model->setHorizontalHeaderLabels(header_labels);

    for(int i = 0; i < model->columnCount(); ++i)
    {
        if(model->horizontalHeaderItem(i)->text().isEmpty())
            ui->tableView->setColumnHidden(i, true);
        else
            ui->tableView->setColumnHidden(i, false);
    }
    setColumnHeaders();

    emit fileProcessor->startProcessing(urls, yara);
}


void Widget::addLoadingGifToEmptyCells(QTableView *tableView)
{
    for(int row = 0; row < model->rowCount(); ++row)
    {
        for(int col = 0; col < model->columnCount(); ++col)
        {
            QStandardItem *item = model->item(row, col);

            if(!item || item->text().isEmpty())
            {
                QLabel *gif_label = new QLabel();
                gif_label->setMovie(movie);

                gif_label->setAlignment(Qt::AlignCenter);
                gif_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

                gif_label->setAttribute(Qt::WA_TranslucentBackground, true);

                QModelIndex source_index = model->index(row, col);
                QModelIndex proxy_index = proxyModel->mapFromSource(source_index);
                tableView->setIndexWidget(proxy_index, gif_label);
            }
        }
    }
}


void Widget::addDashToEmptyCells(QTableView *tableView)
{
    for (int row = 0; row < proxyModel->rowCount(); ++row)
    {
        for (int col = 0; col < proxyModel->columnCount(); ++col)
        {
            QModelIndex proxyIndex = proxyModel->index(row, col);
            QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);
            QStandardItem *item = model->itemFromIndex(sourceIndex);
            QWidget *cell_widget = tableView->indexWidget(proxyIndex);

            if (cell_widget)
            {
                tableView->setIndexWidget(proxyIndex, nullptr);
                delete cell_widget;
                item->setText("-");
            } else if(item->text().isEmpty())
            {
                item->setText("-");
            }
        }
    }
}


void Widget::copySelectedCells(QTableView *tableView, bool copy_headers, bool copy_to_file)
{
    QItemSelectionModel *selectionModel = tableView->selectionModel();
    QModelIndexList selectedIndexes = selectionModel->selectedIndexes();

    if (selectedIndexes.isEmpty()) {
        tableView->selectAll();
        selectedIndexes = selectionModel->selectedIndexes();
    }

    QModelIndexList visibleSelectedIndexes;
    for(const QModelIndex &index : selectedIndexes)
    {
        if(!tableView->isColumnHidden(index.column()))
            visibleSelectedIndexes.append(index);
    }

    std::sort(visibleSelectedIndexes.begin(), visibleSelectedIndexes.end(), [](const QModelIndex &a, const QModelIndex &b) {
        return (a.row() < b.row()) || (a.row() == b.row() && a.column() < b.column());
    });

    QString copiedData;

    if(copy_headers)
    {
        QAbstractItemModel *headerModel = tableView->horizontalHeader()->model();
        int headerCount = headerModel->columnCount();
        for(int i = 0; i < headerCount; ++i)
        {
            if(!tableView->isColumnHidden(i))
            {
                QVariant headerData = headerModel->headerData(i, Qt::Horizontal);
                copiedData.append(headerData.toString().append("\t"));
            }
        }
        copiedData.chop(1);
        copiedData.append("\n");
    }

    int previousRow = visibleSelectedIndexes.first().row();
    for (const QModelIndex &index : visibleSelectedIndexes) {
        if (index.row() != previousRow) {
            copiedData.chop(1);
            copiedData.append("\n");
            previousRow = index.row();
        }

        QVariant cellData;
        if(index.data(Qt::UserRole).toBool())
            cellData = index.data(Qt::UserRole);
        else
            cellData = index.data();
        copiedData.append(cellData.toString()).append("\t");
    }
    copiedData.chop(1);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(copiedData);

    if(copy_to_file)
    {
        QFileDialog file_dialog(this, "Save to .tsv");
        file_dialog.setAcceptMode(QFileDialog::AcceptSave);
        file_dialog.setFileMode(QFileDialog::AnyFile);
        file_dialog.setNameFilter("Tab Separated Value Files (*.tsv);;All Files (*)");
        file_dialog.setDefaultSuffix("tsv");

        if(file_dialog.exec() == QDialog::Accepted)
        {
            QString file_path = file_dialog.selectedFiles().at(0);
            if(clipboard->mimeData()->hasText())
            {
                QString text_data = clipboard->text();

                QFile file(file_path);
                if(file.open(QIODevice::WriteOnly | QIODevice::Text))
                {
                    QTextStream out(&file);
                    out << text_data;
                    file.close();
                }
            }
        }
    }
}


bool Widget::allHashboxesUnchecked()
{
    if(!model->rowCount())
        return true;
    for(QObject *child : ui->frame_btn_hashes->children())
    {
        QPushButton *push_button = qobject_cast<QPushButton *>(child);
        if(push_button && push_button->isChecked())
            return false;
    }
    return true;
}


void Widget::showFileStatistics()
{
    QItemSelectionModel *selection_model = ui->tableView->selectionModel();
    QModelIndexList selected_rows = selection_model->selectedRows();
    QModelIndexList selected_columns = selection_model->selectedColumns();
    QModelIndexList selected_items = selection_model->selectedIndexes();

    int visible_rows_count = proxyModel->rowCount();
    int source_rows_count = proxyModel->sourceModel()->rowCount();
    int outfiltered_rows_count = source_rows_count - visible_rows_count;

    QString status_text;

    if(visible_rows_count > 0)
        status_text.append(QString::number(visible_rows_count));

    if(outfiltered_rows_count > 0)
        status_text.append(" ( " + QString::number(outfiltered_rows_count) + " )");

    if(selected_rows.count() > 0)
        status_text.append(" [ " + QString::number(selected_rows.count()) + " ]");
    else if(selected_columns.count() > 0)
        status_text.append(" [ " + QString::number(selected_columns.count()) + " ]");
    else if(selected_items.count() > 0)
        status_text.append(" [ " + QString::number(selected_items.count()) + " ]");


    ui->lbl_status_files->setText(status_text);
}


void Widget::setColumnHeaders()
{
    ui->tableView->setColumnWidth(Column::FILENAME, 300);
    ui->tableView->setColumnWidth(Column::MD5, 245);
    ui->tableView->setColumnWidth(Column::SHA1, 300);
    ui->tableView->setColumnWidth(Column::SHA256, 465);
    ui->tableView->setColumnWidth(Column::YARA, 300);
    ui->tableView->setColumnWidth(Column::FILESIZE, 100);
    ui->tableView->setColumnWidth(Column::FILE_EXTENSION, 50);
    ui->tableView->setColumnWidth(Column::MIMETYPE, 300);
    ui->tableView->setColumnWidth(Column::FILETYPE, 300);
    ui->tableView->setColumnWidth(Column::DIRPATH, 300);
    ui->tableView->setColumnWidth(Column::FULLPATH, 300);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
}


void Widget::showSelectedFiles()
{
    QItemSelectionModel *selection_model = ui->tableView->selectionModel();
    QModelIndexList selected_items = selection_model->selectedIndexes();
    QStringList files;

    for(int row = 0; row < proxyModel->rowCount(); ++row)
    {
        QModelIndex index = proxyModel->index(row, Column::FULLPATH);
        if(selected_items.contains(index))
        {
            QVariant item_data = proxyModel->data(index);
            files.append(item_data.toString());
        }
    }
}


void Widget::toggleFrameButtons(const QObjectList &frame_children)
{
    for(QObject *child : frame_children)
    {
        QPushButton *push_button = qobject_cast<QPushButton *>(child);
        if(push_button)
        {
            (push_button->isEnabled()) ? push_button->setDisabled(true) : push_button->setEnabled(true);
        }
    }
}


void Widget::hideButtons()
{
    ui->btn_hide_doubles->hide();
    ui->btn_filesize->hide();
    ui->btn_extension->hide();
    ui->btn_mimetype->hide();
    ui->btn_filetype->hide();    
    ui->btn_dirpath->hide();
    ui->btn_fullpath->hide();
    ui->btn_clipboard->hide();
    ui->btn_save->hide();
    ui->btn_clear->hide();
}


void Widget::showButtons()
{
    ui->btn_hide_doubles->show();
    ui->btn_filesize->show();
    ui->btn_extension->show();
    ui->btn_mimetype->show();
    ui->btn_filetype->show();
    ui->btn_dirpath->show();
    ui->btn_fullpath->show();
    ui->btn_clipboard->show();
    ui->btn_save->show();
    ui->btn_clear->show();
}


void Widget::onResultReady(const QString &result)
{
    ui->progressBar->setValue(++processed_items);
    ui->progressBar->setFormat(QString("hashing files: %1/%2").arg(ui->progressBar->value()).arg(ui->progressBar->maximum()));

    QStringList columns = result.split("\t");
    QString hash_type = columns[0];
    QString hash_value = columns[1];
    QString file_path = columns[2];

    if(!file_hash_list.contains(file_path))
    {
        file_hash_list[file_path] = QStringList() << "" //MD5
                                                  << "" //SHA1
                                                  << ""; //SHA256
    }

    if(hash_type == "MD5")
        file_hash_list[file_path][0] = hash_value;
    else if(hash_type == "SHA1")
        file_hash_list[file_path][1] = hash_value;
    else if(hash_type == "SHA256")
        file_hash_list[file_path][2] = hash_value;

    int rows = model->rowCount();
    for(int row = 0; row < rows; ++row)
    {
        QStandardItem *full_path_column = model->item(row, Column::FULLPATH);
        if(file_path == full_path_column->text())
        {
            QStandardItem *md5_item = new QStandardItem(file_hash_list[file_path].at(0));
            model->setItem(row, Column::MD5, md5_item);

            QStandardItem *sha1_item = new QStandardItem(file_hash_list[file_path].at(1));
            model->setItem(row, Column::SHA1, sha1_item);

            QStandardItem *sha256_item = new QStandardItem(file_hash_list[file_path].at(2));
            model->setItem(row, Column::SHA256, sha256_item);

            setColumnHeaders();
        }
    }
}


void Widget::onProcessingFinished(const QString &result)
{
    ui->progressBar->hide();

    if(!(model->rowCount() > 1000))
        ui->frame_search->show();

    ui->lbl_clock->show();
    ui->lbl_status->show();
    ui->lbl_status->setText((QString::number(file_count) + " %1 hashed in " + result + " seconds").arg((file_count == 1) ? "File" : "Files"));

    ui->tableView->setSortingEnabled(true);

    addDashToEmptyCells(ui->tableView);
    setColumnHeaders();

    showButtons();
    setAcceptDrops(true);
}


void Widget::onCellItemChanged(QStandardItem *item)
{
    if(!item->text().isEmpty())
    {
        QModelIndex source_index = item->index();
        QModelIndex proxy_index = proxyModel->mapFromSource(source_index);
        QWidget *cell_widget = ui->tableView->indexWidget(proxy_index);
        if(cell_widget)
        {
            ui->tableView->setIndexWidget(proxy_index, nullptr);
            delete cell_widget;
        }
    }
}


void Widget::onSortIndicatorChanged(int logical_index, Qt::SortOrder order)
{
    if(logical_index == -1)
    {
        ui->tableView->setModel(model);
    }
    else
    {
        ui->tableView->sortByColumn(logical_index, order);
    }
}


void Widget::onDoublesFound()
{
    doubles_found = true;

    if(allHashboxesUnchecked())
    {   ui->btn_hide_doubles->setChecked(false);
        ui->btn_hide_doubles->setDisabled(true);
    }
    else
    {
        ui->btn_hide_doubles->setEnabled(true);
    }
}


void Widget::onMissingDoubles()
{
    if(!ui->btn_hide_doubles->isChecked())
    {
        doubles_found = false;
        ui->btn_hide_doubles->setDisabled(true);
    }
}

void Widget::onPageChanged()
{
    if(ui->stackedWidget->currentWidget() == ui->page_drop)
    {
        hideButtons();
        ui->frame->show();
        ui->frame_btn_hashes->show();
        ui->btn_about->show();
        ui->frame_yara_status->show();
    }
    else if(ui->stackedWidget->currentWidget() == ui->page_main)
    {
        showButtons();
        ui->frame->show();
        ui->frame_btn_hashes->show();
        ui->btn_about->show();
        ui->frame_yara_status->hide();
    }
    else if(ui->stackedWidget->currentWidget() == ui->page_options
             || ui->stackedWidget->currentWidget() == ui->page_about)
    {
        hideButtons();
        ui->frame->show();
        ui->frame_btn_hashes->hide();
        ui->frame_yara_status->hide();
    }
    else if(ui->stackedWidget->currentWidget() == ui->page_loading)
    {
        hideButtons();
        ui->frame->hide();
        ui->frame_yara_status->hide();
    }
    else if(ui->stackedWidget->currentWidget() == ui->page_yara)
    {
        hideButtons();
        ui->frame->hide();
        ui->btn_about->hide();
        ui->frame_yara_status->show();
    }
}


void Widget::onFileProcessorFileCountSum(int count)
{
    model->setRowCount(model->rowCount() + count);
    file_count = count;

    if(!md5 && !sha1 && !sha256)
        item_count = file_count;
    else
    {
        int multiplicator = 0;
        if(md5)
            ++multiplicator;
        if(sha1)
            ++multiplicator;
        if(sha256)
            ++multiplicator;

        item_count = multiplicator * file_count;
    }

    ui->progressBar_modelfiles->show();
    ui->progressBar_modelfiles->setRange(0, file_count);

    ui->lbl_status_files->hide();
}

void Widget::onFileProcessorFileCount(int count)
{
    ui->progressBar_modelfiles->setValue(count);
    ui->progressBar_modelfiles->setFormat(QString("processing files: %1/%2").arg(ui->progressBar_modelfiles->value()).arg(ui->progressBar_modelfiles->maximum()));
}


void Widget::onFileProcessorUpdateModel(const QStringList &data)
{
    for(int col = 0; col < data.size(); ++col){
        QStandardItem *item = new QStandardItem(data.at(col));

        if(col == Column::FILESIZE)
        {
            QString number_str = data.at(col);
            unsigned long long file_size = number_str.remove('.').toLongLong();
            item->setData(file_size, Qt::UserRole);
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        }
        else if(col == Column::FILE_EXTENSION)
        {
            if(data.at(col).isEmpty())
            {
                item->setData("-", Qt::DisplayRole);
            }
        }
        else if(col == Column::FILENAME)
        {
            QColor foreground_color(235,240,250,255);
            item->setData(foreground_color, Qt::ForegroundRole);
            item->setIcon(file_icon);
        }
        else if(col == Column::YARA)
        {
            if(!data.at(col).isEmpty())
            {
                QColor foreground_color(255,200,121,255);
                item->setData(foreground_color, Qt::ForegroundRole);
            }
        }

        model->setItem(row_count, col, item);
    }

    ++row_count;

    if(!page_main_visible)
    {
        page_main_visible = true;
        ui->stackedWidget->setCurrentWidget(ui->page_main);
    }
}


void Widget::onFileProcessingFinished(const QHash<QString, QStringList> *file_list)
{
    ui->progressBar_modelfiles->hide();
    ui->lbl_status_files->show();

    if(file_list->size() < 500)
        addLoadingGifToEmptyCells(ui->tableView);

    if(md5 || sha1 || sha256)
    {
        processor->startProcessing(file_list, md5, sha1, sha256);

        ui->progressBar->show();
        ui->progressBar->setRange(0, item_count);
        ui->progressBar->setValue(processed_items);
        ui->progressBar->setFormat(QString("hashing: %1/%2").arg(ui->progressBar->value()).arg(ui->progressBar->maximum()));
        setColumnHeaders();
    }
    else
    {
        ui->progressBar->hide();
        ui->lbl_status->show();
        ui->tableView->setSortingEnabled(true);
        setAcceptDrops(true);
        addDashToEmptyCells(ui->tableView);

        if(!(model->rowCount() > 1000))
            ui->frame_search->show();

        showButtons();

        setColumnHeaders();
    }
}


void Widget::on_btn_clear_clicked()
{
    model->clear();
    hideButtons();
    ui->stackedWidget->setCurrentWidget(ui->page_drop);
    doubles_found = false;
    ui->btn_hide_doubles->setDisabled(true);
}


void Widget::on_btn_clipboard_clicked()
{
    copySelectedCells(ui->tableView, false, false);
}


void Widget::on_btn_save_clicked()
{
    copySelectedCells(ui->tableView, true, true);
}


void Widget::on_lineEdit_search_textChanged(const QString &arg1)
{
    if (ui->btn_regex->isChecked())
    {
        QRegularExpression regex(arg1, proxyModel->filterCaseSensitivity() == Qt::CaseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
        if(regex.isValid())
        {
            proxyModel->setFilterRegularExpression(regex);
            ui->lineEdit_search->setStyleSheet("");
        }
        else
            ui->lineEdit_search->setStyleSheet("background-color: rgba(255, 75, 92, .5); border-color: rgba(255, 75, 92, .5);");
    }
    else if(ui->btn_whole_word->isChecked())
    {
        QString whole_word_pattern = QString("\\b%1\\b").arg(arg1);
        QRegularExpression regex(whole_word_pattern, proxyModel->filterCaseSensitivity() == Qt::CaseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
        if(regex.isValid())
        {
            proxyModel->setFilterRegularExpression(regex);
            ui->lineEdit_search->setStyleSheet("");
        }
        else
            ui->lineEdit_search->setStyleSheet("background-color: rgba(255, 75, 92, .5); border-color: rgba(255, 75, 92, .5);");
    }
    else
    {
        proxyModel->setFilterWildcard(arg1);
        ui->lineEdit_search->setStyleSheet("");
    }
}


void Widget::on_btn_match_case_toggled(bool checked)
{
    if(checked)
    {
        case_option_set = true;
        proxyModel->setFilterCaseSensitivity(Qt::CaseSensitive);
    }
    else
    {
        case_option_set = false;
        proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    }

    ui->lineEdit_search->setFocus();
}


void Widget::on_btn_whole_word_toggled(bool checked)
{
    if (checked)
    {
        wholeword_option_set = true;
        QString whole_word_pattern = QString("\\b%1\\b").arg(ui->lineEdit_search->text());
        QRegularExpression regex(whole_word_pattern, proxyModel->filterCaseSensitivity() == Qt::CaseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
        if(regex.isValid())
        {
            proxyModel->setFilterRegularExpression(regex);
            ui->lineEdit_search->setStyleSheet("");
        }
        else
            ui->lineEdit_search->setStyleSheet("background-color: rgba(255, 75, 92, .5); border-color: rgba(255, 75, 92, .5);");
    }
    else
    {
        wholeword_option_set = false;
        proxyModel->setFilterWildcard(ui->lineEdit_search->text());
        ui->lineEdit_search->setStyleSheet("");
    }

    ui->lineEdit_search->setFocus();
}



void Widget::on_btn_regex_toggled(bool checked)
{
    if (checked)
    {
        regex_option_set = true;
        QRegularExpression regex(ui->lineEdit_search->text(), proxyModel->filterCaseSensitivity() == Qt::CaseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
        if(regex.isValid())
        {
            proxyModel->setFilterRegularExpression(regex);
            ui->lineEdit_search->setStyleSheet("");
        }
        else
            ui->lineEdit_search->setStyleSheet("background-color: rgba(255, 75, 92, .5); border-color: rgba(255, 75, 92, .5);");
    }
    else
    {
        regex_option_set = false;
        proxyModel->setFilterWildcard(ui->lineEdit_search->text());
        ui->lineEdit_search->setStyleSheet("");
    }

    ui->lineEdit_search->setFocus();
}


void Widget::on_btn_hide_doubles_toggled(bool checked)
{
    proxyModel->setFilterDuplicates(checked);
    showFileStatistics();
}


void Widget::on_btn_fullpath_toggled(bool checked)
{
    show_fullpath = checked ? true : false;

    if(model->rowCount())
    {
        if(model->horizontalHeaderItem(Column::FULLPATH)->text().isEmpty())
            model->setHorizontalHeaderItem(Column::FULLPATH, new QStandardItem("Fullpath"));
    }
    ui->tableView->setColumnHidden(Column::FULLPATH, !show_fullpath);
}


void Widget::on_btn_dirpath_toggled(bool checked)
{
    show_dirpath = checked ? true : false;

    if(model->rowCount())
    {
        if(model->horizontalHeaderItem(Column::DIRPATH)->text().isEmpty())
            model->setHorizontalHeaderItem(Column::DIRPATH, new QStandardItem("Dirpath"));
    }
    ui->tableView->setColumnHidden(Column::DIRPATH, !show_dirpath);
}


void Widget::on_btn_extension_toggled(bool checked)
{
    show_extension = checked ? true : false;

    if(model->rowCount())
    {
        if(model->horizontalHeaderItem(Column::FILE_EXTENSION)->text().isEmpty())
            model->setHorizontalHeaderItem(Column::FILE_EXTENSION, new QStandardItem("Ext"));
    }
    ui->tableView->setColumnHidden(Column::FILE_EXTENSION, !show_extension);
}



void Widget::on_btn_filesize_toggled(bool checked)
{
    show_filesize = checked ? true : false;

    if(model->rowCount())
    {
        if(model->horizontalHeaderItem(Column::FILESIZE)->text().isEmpty())
            model->setHorizontalHeaderItem(Column::FILESIZE, new QStandardItem("Filesize"));
    }
    ui->tableView->setColumnHidden(Column::FILESIZE, !show_filesize);
}


void Widget::on_btn_mimetype_toggled(bool checked)
{
    show_mimetype = checked ? true : false;

    if(model->rowCount())
    {
        if(model->horizontalHeaderItem(Column::MIMETYPE)->text().isEmpty())
            model->setHorizontalHeaderItem(Column::MIMETYPE, new QStandardItem("MIME type"));
    }
    ui->tableView->setColumnHidden(Column::MIMETYPE, !show_mimetype);
}


void Widget::on_btn_filetype_toggled(bool checked)
{
    show_filetype = checked ? true : false;

    if(model->rowCount())
    {
        if(model->horizontalHeaderItem(Column::FILETYPE)->text().isEmpty())
            model->setHorizontalHeaderItem(Column::FILETYPE, new QStandardItem("Filetype"));
    }
    ui->tableView->setColumnHidden(Column::FILETYPE, !show_filetype);
}


void Widget::on_btn_md5_toggled(bool checked)
{
    md5 = checked ? true : false;
    if(model->rowCount())
    {
        if(model->horizontalHeaderItem(Column::MD5)->text().isEmpty())
            model->setHorizontalHeaderItem(Column::MD5, new QStandardItem("MD5"));
    }
    ui->tableView->setColumnHidden(Column::MD5, !md5);

    if(allHashboxesUnchecked() && doubles_found)
    {
        ui->btn_hide_doubles->setDisabled(true);
        ui->btn_hide_doubles->setChecked(false);
    }
    else if(doubles_found)
        ui->btn_hide_doubles->show();
}


void Widget::on_btn_sha1_toggled(bool checked)
{
    sha1 = checked ? true : false;
    if(model->rowCount())
    {
        if(model->horizontalHeaderItem(Column::SHA1)->text().isEmpty())
            model->setHorizontalHeaderItem(Column::SHA1, new QStandardItem("SHA1"));
    }
    ui->tableView->setColumnHidden(Column::SHA1, !sha1);

    if(allHashboxesUnchecked() && doubles_found)
    {
        ui->btn_hide_doubles->setDisabled(true);
        ui->btn_hide_doubles->setChecked(false);
    }
    else if(doubles_found)
        ui->btn_hide_doubles->show();
}


void Widget::on_btn_sha256_toggled(bool checked)
{
    sha256 = checked ? true : false;
    if(model->rowCount())
    {
        if(model->horizontalHeaderItem(Column::SHA256)->text().isEmpty())
            model->setHorizontalHeaderItem(Column::SHA256, new QStandardItem("SHA256"));
    }
    ui->tableView->setColumnHidden(Column::SHA256, !sha256);

    if(allHashboxesUnchecked() && doubles_found)
    {
        ui->btn_hide_doubles->setDisabled(true);
        ui->btn_hide_doubles->setChecked(false);
    }
    else if(doubles_found)
        ui->btn_hide_doubles->show();
}


void Widget::on_btn_about_toggled(bool checked)
{
    toggleFrameButtons(ui->frame_btn_hashes->children());
    toggleFrameButtons(ui->frame_btn_options->children());

    if(checked)
    {
        ui->btn_options->setChecked(false);
        ui->stackedWidget->setCurrentWidget(ui->page_about);
    }
    else
    {
        if(ui->tableView->model()->rowCount() > 0)
        {
            ui->stackedWidget->setCurrentWidget(ui->page_main);
        }
        else
            ui->stackedWidget->setCurrentWidget(ui->page_drop);
    }
}


void Widget::on_btn_options_toggled(bool checked)
{
    toggleFrameButtons(ui->frame_btn_hashes->children());
    toggleFrameButtons(ui->frame_btn_options->children());

    if(checked)
    {
        ui->btn_about->setChecked(false);
        ui->stackedWidget->setCurrentWidget(ui->page_options);
    }
    else
    {
        if(ui->tableView->model()->rowCount() > 0)
        {
            ui->stackedWidget->setCurrentWidget(ui->page_main);
        }
        else
            ui->stackedWidget->setCurrentWidget(ui->page_drop);
    }
}


void Widget::readSettings()
{
    QSettings settings(QCoreApplication::applicationDirPath() + "/db/settings.ini", QSettings::IniFormat);

    // hashing settings
    md5 = settings.value("md5").toBool();
    ui->btn_md5->setChecked(md5);

    sha1 = settings.value("sha1").toBool();
    ui->btn_sha1->setChecked(sha1);

    sha256 = settings.value("sha256").toBool();
    ui->btn_sha256->setChecked(sha256);

    // column option settings
    show_filesize = settings.value("filesize").toBool();
    ui->btn_filesize->setChecked(show_filesize);

    show_mimetype = settings.value("mime_type").toBool();
    ui->btn_mimetype->setChecked(show_mimetype);

    show_filetype = settings.value("filetype").toBool();
    ui->btn_filetype->setChecked(show_filetype);

    show_extension = settings.value("extension").toBool();
    ui->btn_extension->setChecked(show_extension);

    show_dirpath = settings.value("dirpath").toBool();
    ui->btn_dirpath->setChecked(show_dirpath);

    show_fullpath = settings.value("fullpath").toBool();
    ui->btn_fullpath->setChecked(show_fullpath);
}


void Widget::writeSettings()
{
    QSettings settings(QCoreApplication::applicationDirPath() + "/db/settings.ini", QSettings::IniFormat);

    settings.setValue("md5", md5);
    settings.setValue("sha1", sha1);
    settings.setValue("sha256", sha256);

    settings.setValue("filesize", show_filesize);
    settings.setValue("mime_type", show_mimetype);
    settings.setValue("filetype", show_filetype);

    settings.setValue("extension", show_extension);
    settings.setValue("dirpath", show_dirpath);
    settings.setValue("fullpath", show_fullpath);
}


void Widget::on_btn_yara_toggled(bool checked)
{
    yara = checked ? true : false;

    if(model->rowCount())
    {
        if(model->horizontalHeaderItem(Column::YARA)->text().isEmpty())
            model->setHorizontalHeaderItem(Column::YARA, new QStandardItem("YARA"));
    }
    ui->tableView->setColumnHidden(Column::YARA, !yara);

    if(checked && !yara_init)
    {
        yara_init = true;
        ui->lbl_yara_error->hide();
        ui->lbl_no_rules_found->hide();
        ui->textEdit_error->hide();
        ui->textEdit_warning->hide();
        ui->textEdit_success->hide();
        emit fileProcessor->startInitializingYara();
        ui->stackedWidget->setCurrentWidget(ui->page_yara);
    }

    if(checked)
    {
        ui->btn_yara_status->show();
        ui->btn_yara_status_main->show();
    }
    else
    {
        ui->btn_yara_status->hide();
        ui->btn_yara_status->setChecked(false);
        ui->btn_yara_status_main->hide();

        ui->btn_reload_rules->hide();
        ui->btn_open_yaradir->hide();

        if(model->rowCount() > 0)
            ui->stackedWidget->setCurrentWidget(ui->page_main);
        else
            ui->stackedWidget->setCurrentWidget(ui->page_drop);
    }
}


void Widget::on_btn_yara_status_toggled(bool checked)
{
    if(checked)
    {
        ui->stackedWidget->setCurrentWidget(ui->page_yara);
        ui->btn_open_yaradir->show();
        ui->btn_reload_rules->show();
    }
    else
    {
        if(model->rowCount() > 0)
            ui->stackedWidget->setCurrentWidget(ui->page_main);
        else
            ui->stackedWidget->setCurrentWidget(ui->page_drop);

        ui->btn_open_yaradir->hide();
        ui->btn_reload_rules->hide();
    }
}

void Widget::on_btn_yara_status_main_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_yara);
    ui->btn_yara_status->setChecked(true);
}


void Widget::onYaraError(QString error)
{
    ui->stackedWidget->setCurrentWidget(ui->page_yara);
    ui->lbl_yara_error->show();
    ui->textEdit_error->show();
    ui->textEdit_error->append(error);
}


void Widget::onYaraWarning(QString warning)
{
    ui->textEdit_warning->show();
    ui->textEdit_warning->append(warning);
}


void Widget::onYaraSuccess(QString success)
{
    ui->textEdit_success->show();
    ui->textEdit_success->append(success);
}


void Widget::on_btn_reload_rules_clicked()
{
    ui->lbl_yara_error->hide();
    ui->lbl_no_rules_found->hide();
    ui->textEdit_error->hide();
    ui->textEdit_warning->hide();
    ui->textEdit_success->hide();

    if(!yara_init)
    {
        yara_init = true;
        emit fileProcessor->startInitializingYara();
    }
    else
    {
        ui->btn_reload_rules->setDisabled(true);
        ui->textEdit_error->clear();
        ui->textEdit_warning->clear();
        ui->textEdit_success->clear();

        emit fileProcessor->startLoadingCompilingYaraRules(QCoreApplication::applicationDirPath() + "/YARA");
    }
}


void Widget::on_btn_open_yaradir_clicked()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/YARA"));
}


void Widget::onFinishedLoadingYaraRules()
{
    ui->btn_reload_rules->setEnabled(true);

    ui->btn_yara_status->show();

    if(ui->textEdit_error->isVisible())
    {
        ui->btn_yara->setDisabled(true);
        ui->btn_yara_status->setIcon(yara_icon_red);
        ui->btn_yara_status_main->setIcon(yara_icon_red);
        ui->btn_yara_status->setChecked(true);
        ui->stackedWidget->setCurrentWidget(ui->page_yara);
    }
    else if(ui->textEdit_warning->isVisible())
    {
        ui->btn_yara->setEnabled(true);
        ui->btn_yara_status->setIcon(yara_icon_orange);
        ui->btn_yara_status_main->setIcon(yara_icon_orange);

        if(!ui->btn_yara_status->isChecked())
        {
            if(model->rowCount() > 0)
                ui->stackedWidget->setCurrentWidget(ui->page_main);
            else
                ui->stackedWidget->setCurrentWidget(ui->page_drop);
        }
    }
    else if(ui->textEdit_success->isVisible())
    {
        ui->btn_yara->setEnabled(true);
        ui->btn_yara_status->setIcon(yara_icon_green);
        ui->btn_yara_status_main->setIcon(yara_icon_green);

        if(!ui->btn_yara_status->isChecked())
        {
            if(model->rowCount() > 0)
                ui->stackedWidget->setCurrentWidget(ui->page_main);
            else
                ui->stackedWidget->setCurrentWidget(ui->page_drop);
        }
    }
    else
    {
        ui->stackedWidget->setCurrentWidget(ui->page_yara);
        ui->lbl_no_rules_found->show();
        ui->btn_yara->setDisabled(true);
        ui->btn_yara_status->setIcon(yara_icon_grey);
        ui->btn_yara_status_main->setIcon(yara_icon_grey);
        ui->btn_yara_status->setChecked(true);
    }
}


bool Widget::dir_contains_file(const QDir &dir)
{
    if(!dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files).isEmpty())
        return true;

    for(const auto &sub_dir : dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs))
    {
        if(dir_contains_file(QDir(sub_dir.absoluteFilePath())))
            return true;
    }

    return false;
}



