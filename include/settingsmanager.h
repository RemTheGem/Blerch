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
private:
    SettingsManager();
    QSettings settings;
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;
};

#endif // SETTINGSMANAGER_H
