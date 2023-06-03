#include "widget.h"

#include <QApplication>
#include <QFontDatabase>
#include <QFont>


QString readCSSFile(QString path)
{
    QFile file(path);

    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
        QString css = in.readAll();
        file.close();

        QString text_color = "rgba(255,255,255,.75)";

        QString lighter = "rgb(64,65,66)";
        QString background_color = "rgb(42,43,44)";
        QString darker = "rgb(32,33,34)";

        QString blue = "#76a4bd";
        QString red = "#ff4b5c";
        QString green = "#81bd76";

        css.replace("%BACKGROUND_COLOR%", background_color);
        css.replace("%TEXT_COLOR%", text_color);

        css.replace("%BLUE%", blue);
        css.replace("%RED%", red);
        css.replace("%GREEN%", green);
        css.replace("%LIGHTER%", lighter);
        css.replace("%DARKER%", darker);

        return css;
    }

    return "Couldn't read css file.";
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString css = readCSSFile(":/style/style.css");
    a.setStyleSheet(css);

    int font_id = QFontDatabase::addApplicationFont(":/fonts/Roboto-Regular.ttf");

    if(font_id != -1)
    {
        QString font_family = QFontDatabase::applicationFontFamilies(font_id).at(0);
        QFont custom_font(font_family);
        custom_font.setPointSize(10);
        a.setFont(custom_font);
    }
    else
        qDebug() << "Failed to load custom font";

    Widget w;
    w.setWindowIcon(QIcon(":/img/icon.png"));
    w.show();
    return a.exec();
}
