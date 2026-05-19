#pragma once

#include <QApplication>

class QTranslator;

class Application : public QApplication {
    Q_OBJECT
public:
    Application(int& argc, char** argv);

    static void switchLanguage(const QString& locale);
    static QString currentLanguage();

private:
    static QTranslator* s_translator;
    static QString s_currentLanguage;
};
