#include "streamcore_demo_qt_window.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

#if defined(Q_OS_LINUX)
#include <X11/Xlib.h>
#endif

static void ConfigureStableProductDirectories()
{
    QString base_dir = QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation);
    if (base_dir.isEmpty())
    {
        base_dir = QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation);
    }
    if (base_dir.isEmpty())
    {
        base_dir = QDir::home().absoluteFilePath(
            QString::fromUtf8(".streamcore_demo_qt"));
    }

    QDir directory;
    const QString log_dir = QDir(base_dir).absoluteFilePath(
        QString::fromUtf8("streamcore/logs"));
    const QString crash_dir = QDir(base_dir).absoluteFilePath(
        QString::fromUtf8("streamcore/crash"));
    directory.mkpath(log_dir);
    directory.mkpath(crash_dir);

    if (qgetenv("STREAMCORE_DEMO_LOG_DIRECTORY").isEmpty())
    {
        qputenv("STREAMCORE_DEMO_LOG_DIRECTORY", log_dir.toUtf8());
    }
    if (qgetenv("STREAMCORE_DEMO_CRASH_DIRECTORY").isEmpty())
    {
        qputenv("STREAMCORE_DEMO_CRASH_DIRECTORY", crash_dir.toUtf8());
    }
}

int main(int argc, char* argv[])
{
#if defined(Q_OS_LINUX)
    XInitThreads();
#endif

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QString::fromUtf8("HBR"));
    QCoreApplication::setApplicationName(QString::fromUtf8("StreamCoreDemoQt"));
    QCoreApplication::setApplicationVersion(QString::fromUtf8("1.4.1"));
    ConfigureStableProductDirectories();

    StreamCoreDemoQtWindow window;

    window.show();
    return app.exec();
}
