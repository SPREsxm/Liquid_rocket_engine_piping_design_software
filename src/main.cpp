#include "app/Application.h"
#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{
    // High DPI support per architecture doc §3.4
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    Application app(argc, argv);
    app.setApplicationName("LiquidRocketPipingDesigner");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("LRE");

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
