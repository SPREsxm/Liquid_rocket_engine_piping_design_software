#include "Application.h"

#include <QDir>
#include <QIcon>
#include <QSettings>
#include <QTranslator>
#include <QWidget>

QTranslator* Application::s_translator = nullptr;
QString Application::s_currentLanguage;

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
{
    setApplicationName("LiquidRocketPipingDesigner");
    setApplicationDisplayName("Liquid Rocket Engine Piping Designer");
    setWindowIcon(QIcon(":/app_icon.svg"));

    // Load saved language preference
    QString savedLang = QSettings().value("Preferences/Language", "en_US").toString();
    switchLanguage(savedLang);
}

void Application::switchLanguage(const QString& locale)
{
    if (s_currentLanguage == locale && !s_translator == !(locale == "en_US"))
        return; // no change needed

    // Remove old translator
    if (s_translator) {
        removeTranslator(s_translator);
        delete s_translator;
        s_translator = nullptr;
    }

    s_currentLanguage = locale;

    // Load translation for non-English languages
    if (locale != "en_US") {
        s_translator = new QTranslator;
        QString qmPath = QCoreApplication::applicationDirPath()
                         + "/translations/lrep_" + locale + ".qm";
        if (s_translator->load(qmPath)) {
            installTranslator(s_translator);
        } else {
            delete s_translator;
            s_translator = nullptr;
        }
    }

    // Persist preference
    QSettings().setValue("Preferences/Language", locale);

    // Notify all top-level widgets to refresh
    for (auto* widget : topLevelWidgets())
        sendEvent(widget, new QEvent(QEvent::LanguageChange));
}

QString Application::currentLanguage()
{
    return s_currentLanguage.isEmpty() ? "en_US" : s_currentLanguage;
}
