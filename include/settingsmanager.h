#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H
#include <QSettings>
#include <QStringList>
class SettingsManager
{
public:
    static SettingsManager& instance();

    void setLastProject(const QString &path);
    QString getLastProject();
    void addRecentFile(const QString &path);
    QStringList getRecentFiles();
    void setCustomPalette(const QString &path);
    QString getCustomPalette();
    void setLastSaveDirectory(const QString &path);
    QString getLastSaveDirectory();
private:
    SettingsManager();
    QSettings settings;
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;
};

#endif // SETTINGSMANAGER_H
