#include "settingsmanager.h"

SettingsManager::SettingsManager()
    : settings("Blerch", "Blerch")
{

}
SettingsManager& SettingsManager::instance(){
    static SettingsManager instance;
    return instance;
}
void SettingsManager::setLastProject(const QString &path){
    settings.setValue("lastProject", path);
}
QString SettingsManager::getLastProject(){
    return settings.value("lastProject", "").toString();
}