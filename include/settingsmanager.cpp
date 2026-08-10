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
void SettingsManager::addRecentFile(const QString &path){
    QStringList files = getRecentFiles();
    files.removeAll(path);
    files.prepend(path);
    while(files.size()>10){
        files.removeLast();

    }
    settings.setValue("recentFiles", files);
}
QStringList SettingsManager::getRecentFiles(){
    return settings.value("recentFiles", QStringList()).toStringList();
}
void SettingsManager::setCustomPalette(const QString &path){
    settings.setValue("customPalette", path);
}
QString SettingsManager::getCustomPalette(){
    return settings.value("customPalette", "").toString();
}
void SettingsManager::setLastSaveDirectory(const QString &path){
    settings.setValue("lastSaveDirectory", path);
}
QString SettingsManager::getLastSaveDirectory(){
    return settings.value("lastSaveDirectory", "").toString();
}