#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QStandardItemModel>
#include <QLabel>
#include <QSvgWidget>

#include "headersortingadapter.h"
#include "customsortfilterproxymodel.h"
#include "fileprocessor.h"
#include "itemprocessor.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onProcessingFinished(const QString &result);
    void onResultReady(const QString &result);
    void onCellItemChanged(QStandardItem *item);
    void onSortIndicatorChanged(int logical_index, Qt::SortOrder order);
    void onDoublesFound();
    void onMissingDoubles();
    void onPageChanged();

    void on_btn_clear_clicked();
    void on_btn_clipboard_clicked();
    void on_btn_save_clicked();

    void on_lineEdit_search_textChanged(const QString &arg1);

    void on_btn_match_case_toggled(bool checked);
    void on_btn_hide_doubles_toggled(bool checked);
    void on_btn_fullpath_toggled(bool checked);
    void on_btn_dirpath_toggled(bool checked);
    void on_btn_md5_toggled(bool checked);
    void on_btn_sha1_toggled(bool checked);
    void on_btn_sha256_toggled(bool checked);

    void on_btn_regex_toggled(bool checked);

    void on_btn_about_toggled(bool checked);

    void on_btn_options_toggled(bool checked);

    void showSelectedFiles();

    void on_btn_extension_toggled(bool checked);

    void on_btn_whole_word_toggled(bool checked);

    void onFileProcessorFileCountSum(int count);
    void onFileProcessorFileCount(int count);
    void onFileProcessorUpdateModel(int row, int col, const QString &data);
    void onFileProcessingFinished(const QHash<QString, QStringList> *file_list);

    void on_btn_filesize_toggled(bool checked);

    void on_btn_mimetype_toggled(bool checked);

    void on_btn_filetype_toggled(bool checked);

private:
    Ui::Widget *ui;
    QWidget *layer;
    ItemProcessor *processor;

    void readSettings();
    void writeSettings();

    void addLoadingGifToEmptyCells(QTableView *tableView);
    QMovie *movie;

    QIcon file_icon;

    void addDashToEmptyCells(QTableView *tableView);
    void copySelectedCells(QTableView *tableView, bool copy_headers, bool copy_to_file);
    bool allHashboxesUnchecked();
    void showFileStatistics();
    void setColumnHeaders();

    void toggleFrameButtons(const QObjectList &frame_children);

    void hideButtons();
    void showButtons();

    QHash<QString, QStringList> file_hash_list;

    QList<QUrl> urls;
    FileProcessor *fileProcessor;

    int file_count;
    int item_count;
    int processed_items;

    bool md5;
    bool sha1;
    bool sha256;
    bool show_filesize;
    bool show_extension;
    bool show_mimetype;
    bool show_filetype;
    bool show_dirpath;
    bool show_fullpath;


    bool regex_option_set;
    bool case_option_set;
    bool wholeword_option_set;

    bool doubles_found;

    bool page_main_visible;

    bool over_thousand_files;

    QStandardItemModel *model;
    HeaderSortingAdapter *headerSortingAdapter;
    CustomSortFilterProxyModel *proxyModel;

    QSvgWidget *drop_area_svg;
    QSvgWidget *info_text_svg;
};
#endif // WIDGET_H
