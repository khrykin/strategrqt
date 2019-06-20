#include "filesystemiomanager.h"
#include "jsonserializer.h"
#include "mainwindow.h"
#include "notifierbackend.h"
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>

FileSystemIOManager::FileSystemIOManager(QWidget *parent) : parent(parent) {}

std::optional<Strategy> FileSystemIOManager::open() {
  auto openFilepath = QFileDialog::getOpenFileName(
      parent, QObject::tr("Open Strategy"), destinationDir(), searchPattern);

  QSettings().setValue(Settings::lastOpenedDirectoryKey, openFilepath);
  return read(openFilepath);
}

void FileSystemIOManager::save(const Strategy &strategy) {
  if (filepath.isEmpty()) {
    saveAs(strategy);
    return;
  }

  write(strategy);
}

void FileSystemIOManager::saveAs(const Strategy &strategy) {
  auto saveAsFilepath = QFileDialog::getSaveFileName(
      parent, QObject::tr("Save Strategy As"), destinationDir(), searchPattern);

  QSettings().setValue(Settings::lastOpenedDirectoryKey, saveAsFilepath);

  if (saveAsFilepath.isEmpty()) {
    return;
  }

  filepath = saveAsFilepath;
  QSettings().setValue(Settings::lastOpenedDirectoryKey,
                       fileInfo().absolutePath());
  write(strategy);
}

std::optional<Strategy> FileSystemIOManager::read(QString readFilepath) {
  if (readFilepath.isEmpty()) {
    return std::nullopt;
  }

  filepath = readFilepath;

  QFile file(filepath);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    QMessageBox::warning(parent, QObject::tr("Open Strategy"),
                         QObject::tr("Cannot read file %1:\n%2.")
                             .arg(filepath)
                             .arg(file.errorString()));
    return std::nullopt;
  };

  QTextStream in(&file);
  auto strategy = JSONSerializer::read(in.readAll());
  file.close();

  if (!strategy) {
    QMessageBox::warning(parent, QObject::tr("Open Strategy"),
                         QObject::tr("Cannot read file %1:\n%2.")
                             .arg(filepath)
                             .arg(file.errorString()));
  } else {
    updateLastOpened();
  }

  return strategy;
}

std::optional<Strategy> FileSystemIOManager::lastOpened() {
  QSettings settings;
  if (!settings.value(Settings::lastOpenedStrategyKey).isNull()) {
    auto filepath = settings.value(Settings::lastOpenedStrategyKey).toString();
    return read(filepath);
  }

  return std::nullopt;
}

void FileSystemIOManager::resetFilepath() { filepath = ""; }

void FileSystemIOManager::clearRecent() {
  QSettings().setValue(Settings::recentKey, QStringList());
}

bool FileSystemIOManager::isSaved() const { return _isSaved; }

void FileSystemIOManager::setIsSaved(bool isSaved) { _isSaved = isSaved; }

QFileInfo FileSystemIOManager::fileInfo() { return QFileInfo(filepath); }

QStringList FileSystemIOManager::recentPaths() {
  auto recentFilesSetting = QSettings().value(Settings::recentKey);
  return recentFilesSetting.toStringList();
}

QStringList FileSystemIOManager::recentFileNames() {
  QStringList result;
  for (auto &path : recentPaths()) {
    result.append(QFileInfo(path).baseName());
  }

  return result;
}

void FileSystemIOManager::write(const Strategy &strategy) {
  auto json = JSONSerializer(strategy).write();
  QFile file(filepath);
  if (file.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)) {
    QTextStream stream(&file);
    stream << json << endl;
    updateLastOpened();
  } else {
    QMessageBox::warning(parent, QObject::tr("Save Strategy"),
                         QObject::tr("Cannot write to file %1:\n%2.")
                             .arg(filepath)
                             .arg(file.errorString()));
  }

  setIsSaved(true);

  file.close();
}

QString FileSystemIOManager::destinationDir() {
  QSettings settings;
  if (!settings.value(Settings::lastOpenedDirectoryKey).isNull()) {
    auto lastOpenedDirectory =
        settings.value(Settings::lastOpenedDirectoryKey).toString();

    return lastOpenedDirectory;
  }

  return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
}

void FileSystemIOManager::updateLastOpened() {
  auto absoluteFilePath = fileInfo().absoluteFilePath();

  QSettings settings;
  settings.setValue(Settings::lastOpenedStrategyKey, absoluteFilePath);
  settings.setValue(Settings::lastOpenedDirectoryKey,
                    fileInfo().absolutePath());

  auto files = recentPaths();
  files.removeAll(absoluteFilePath);
  files.prepend(absoluteFilePath);

  while (files.size() > Settings::numberOfRecent) {
    files.removeLast();
  }

  settings.setValue(Settings::recentKey, files);

  foreach (QWidget *widget, QApplication::topLevelWidgets()) {
    MainWindow *mainWindow = qobject_cast<MainWindow *>(widget);
    if (mainWindow) {
      mainWindow->updateRecentFileActions();
    }
  }
}
