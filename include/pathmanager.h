#ifndef PATHMANAGER_H
#define PATHMANAGER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QDir>
#include <QDateTime>
#include "mousepoint.h"

class PathManager : public QObject
{
    Q_OBJECT

public:
    explicit PathManager(QObject *parent = nullptr);

    // Save/Load functions
    bool savePath(const QList<MousePoint>& path, const QString& name = QString());
    QList<MousePoint> loadPath(const QString& filename);

    // Path management
    QStringList getAvailablePaths() const;
    bool deletePath(const QString& filename);
    bool deletePaths(const QStringList& filenames);
    bool renamePath(const QString& oldFilename, const QString& newName);
    QString getPathInfo(const QString& filename) const;

    // Directory management
    void setDataDirectory(const QString& directory);
    QString dataDirectory() const;

    // Filename utilities
    QString generateDateBasedFilename() const;
    QString getDisplayName(const QString& filename) const;

    // Error handling
    QString getLastError() const;

signals:
    void pathSaved(const QString& filename);
    void pathLoaded(const QString& filename);
    void pathDeleted(const QString& filename);
    void pathsDeleted(const QStringList& filenames);
    void pathRenamed(const QString& oldFilename, const QString& newFilename);

private:
    QString m_dataDirectory;
    QString m_lastError;
    QString ensureDataDirectory();
    bool isValidPathFile(const QString& filename) const;
};

#endif // PATHMANAGER_H