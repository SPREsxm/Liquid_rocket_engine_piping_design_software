#include "Application.h"

#include <QIcon>

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
{
    setApplicationName("LiquidRocketPipingDesigner");
    setApplicationDisplayName("Liquid Rocket Engine Piping Designer");
    setWindowIcon(QIcon(":/app_icon.svg"));
}
