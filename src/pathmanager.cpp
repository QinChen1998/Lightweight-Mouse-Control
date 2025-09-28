#include "pathmanager.h"
#include <QStandardPaths>
#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QDir>

// 路径管理器构造函数：初始化数据目录路径
PathManager::PathManager(QObject *parent)
    : QObject(parent)
{
    // Set default data directory
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_dataDirectory = defaultDir + "/MousePaths";
}

// 保存鼠标路径：将路径数据序列化为.mpath文件
bool PathManager::savePath(const QList<MousePoint>& path, const QString& name)
{
    if (path.isEmpty()) {
        qWarning() << "Path is empty, cannot save";
        return false;
    }

    QString filename = name.isEmpty() ? generateDateBasedFilename() : name;
    if (!filename.endsWith(".mpath")) {
        filename += ".mpath";
    }

    QString fullPath = ensureDataDirectory() + "/" + filename;

    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << fullPath;
        return false;
    }

    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_5_15);

    // Write file header
    int pointCount = path.size();
    QDateTime saveTime = QDateTime::currentDateTime();

    stream << QString("MPATH_V2"); // Updated file format version for relative time
    stream << saveTime; // Save timestamp
    stream << pointCount; // Number of points

    if (stream.status() != QDataStream::Ok) {
        qWarning() << "Failed to write file header, stream status:" << stream.status();
        file.close();
        return false;
    }

    // Convert absolute timestamps to relative time differences and write points
    QList<MousePoint> convertedPath;
    for (int i = 0; i < path.size(); ++i) {
        const MousePoint& originalPoint = path[i];

        if (i == 0) {
            // First point: deltaMs = 0
            MousePoint convertedPoint(originalPoint.position(), 0);
            convertedPath.append(convertedPoint);
        } else {
            // Calculate time difference from previous point
            qint64 timeDiff = originalPoint.timestamp().toMSecsSinceEpoch() -
                             path[i-1].timestamp().toMSecsSinceEpoch();
            quint32 deltaMs = static_cast<quint32>(qMax(0LL, qMin(timeDiff, static_cast<qint64>(UINT32_MAX))));

            MousePoint convertedPoint(originalPoint.position(), deltaMs);
            convertedPath.append(convertedPoint);
        }
    }

    // Write all converted points
    for (int i = 0; i < convertedPath.size(); ++i) {
        const MousePoint& point = convertedPath[i];
        stream << point;

        if (stream.status() != QDataStream::Ok) {
            qWarning() << "Failed to write point" << i << "stream status:" << stream.status();
            file.close();
            return false;
        }
    }

    file.close();
    qDebug() << "Successfully saved" << path.size() << "points to" << filename << "using relative time format";

    emit pathSaved(filename);
    return true;
}

// 加载鼠标路径：从.mpath文件反序列化路径数据
QList<MousePoint> PathManager::loadPath(const QString& filename)
{
    QList<MousePoint> path;
    m_lastError.clear();

    QString fullPath = m_dataDirectory + "/" + filename;
    QFile file(fullPath);

    qDebug() << "Attempting to load path from:" << fullPath;

    if (!file.exists()) {
        m_lastError = QString("File does not exist: %1").arg(fullPath);
        qWarning() << "Path file does not exist:" << fullPath;
        return path;
    }

    if (file.size() == 0) {
        m_lastError = "File is empty (0 bytes)";
        qWarning() << "File is empty:" << fullPath;
        return path;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = QString("Cannot open file for reading: %1").arg(file.errorString());
        qWarning() << "Failed to open file for reading:" << fullPath << "Error:" << file.errorString();
        return path;
    }

    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_5_15);

    // Read file header
    QString version;
    QDateTime saveTime;
    int pointCount;

    stream >> version >> saveTime >> pointCount;

    qDebug() << "File header - Version:" << version << "SaveTime:" << saveTime << "PointCount:" << pointCount;
    qDebug() << "Stream status after reading header:" << stream.status();

    if (stream.status() != QDataStream::Ok) {
        m_lastError = "File is corrupted or not a valid mouse path file";
        qWarning() << "Failed to read file header, stream status:" << stream.status();
        return path;
    }

    if (version != "MPATH_V1" && version != "MPATH_V2") {
        m_lastError = QString("Unsupported file format: %1 (Expected: MPATH_V1 or MPATH_V2)").arg(version);
        qWarning() << "Unsupported file format:" << version << "Expected: MPATH_V1 or MPATH_V2";
        return path;
    }

    if (pointCount < 0 || pointCount > 1000000) {
        m_lastError = QString("Invalid point count: %1 (Expected: 0-1000000)").arg(pointCount);
        qWarning() << "Invalid point count:" << pointCount;
        return path;
    }

    if (pointCount == 0) {
        m_lastError = "This mouse path file contains no recorded points. The recording may have been too short or failed to capture mouse movements.";
        qWarning() << "Path file contains no points:" << filename;
        return path;
    }

    // Read all points based on file version
    if (version == "MPATH_V1") {
        // Legacy format: points contain absolute timestamps
        for (int i = 0; i < pointCount; ++i) {
            MousePoint point;
            stream >> point;
            if (stream.status() != QDataStream::Ok) {
                m_lastError = QString("Failed to read mouse point %1 of %2 (File may be corrupted)").arg(i + 1).arg(pointCount);
                qWarning() << "Failed to read point" << i << "Stream status:" << stream.status();
                return path;
            }
            path.append(point);
        }
    } else if (version == "MPATH_V2") {
        // New format: points contain relative time differences
        QDateTime baseTime = QDateTime::currentDateTime();
        qint64 currentTimeMs = baseTime.toMSecsSinceEpoch();

        for (int i = 0; i < pointCount; ++i) {
            MousePoint relativePoint;
            stream >> relativePoint;
            if (stream.status() != QDataStream::Ok) {
                m_lastError = QString("Failed to read mouse point %1 of %2 (File may be corrupted)").arg(i + 1).arg(pointCount);
                qWarning() << "Failed to read point" << i << "Stream status:" << stream.status();
                return path;
            }

            // Convert relative time to absolute timestamp for compatibility
            if (i == 0) {
                // First point: use base time
                MousePoint absolutePoint(relativePoint.position(), baseTime);
                path.append(absolutePoint);
            } else {
                // Subsequent points: add deltaMs to previous timestamp
                currentTimeMs += relativePoint.deltaMs();
                QDateTime absoluteTime = QDateTime::fromMSecsSinceEpoch(currentTimeMs);
                MousePoint absolutePoint(relativePoint.position(), absoluteTime);
                path.append(absolutePoint);
            }
        }
    }

    file.close();

    qDebug() << "Successfully loaded" << path.size() << "points from" << filename;
    emit pathLoaded(filename);
    return path;
}

// 获取可用路径列表：扫描数据目录中的所有.mpath文件
QStringList PathManager::getAvailablePaths() const
{
    QDir dir(m_dataDirectory);
    if (!dir.exists()) {
        return QStringList();
    }

    QStringList filters;
    filters << "*.mpath";

    QStringList files = dir.entryList(filters, QDir::Files, QDir::Time | QDir::Reversed);
    return files;
}

// 删除单个路径文件
bool PathManager::deletePath(const QString& filename)
{
    QString fullPath = m_dataDirectory + "/" + filename;
    QFile file(fullPath);

    if (file.exists() && file.remove()) {
        emit pathDeleted(filename);
        return true;
    }

    return false;
}

// 批量删除多个路径文件
bool PathManager::deletePaths(const QStringList& filenames)
{
    QStringList successfullyDeleted;
    QStringList failedToDelete;

    for (const QString& filename : filenames) {
        QString fullPath = m_dataDirectory + "/" + filename;
        QFile file(fullPath);

        if (file.exists() && file.remove()) {
            successfullyDeleted.append(filename);
        } else {
            failedToDelete.append(filename);
        }
    }

    if (!successfullyDeleted.isEmpty()) {
        emit pathsDeleted(successfullyDeleted);
    }

    return failedToDelete.isEmpty();
}

// 重命名路径文件：检查冲突并执行重命名操作
bool PathManager::renamePath(const QString& oldFilename, const QString& newName)
{
    m_lastError.clear();

    if (oldFilename.isEmpty() || newName.isEmpty()) {
        m_lastError = "Both old filename and new name must be provided";
        return false;
    }

    // Construct file paths
    QString oldFullPath = m_dataDirectory + "/" + oldFilename;
    QString newFilename = newName;

    // Ensure new filename has .mpath extension
    if (!newFilename.endsWith(".mpath")) {
        newFilename += ".mpath";
    }

    QString newFullPath = m_dataDirectory + "/" + newFilename;

    // Check if old file exists
    QFile oldFile(oldFullPath);
    if (!oldFile.exists()) {
        m_lastError = QString("Source file does not exist: %1").arg(oldFilename);
        return false;
    }

    // Check if new filename already exists
    QFile newFile(newFullPath);
    if (newFile.exists()) {
        m_lastError = QString("A file with the name '%1' already exists").arg(newFilename);
        return false;
    }

    // Perform the rename operation
    if (oldFile.rename(newFullPath)) {
        qDebug() << "Successfully renamed" << oldFilename << "to" << newFilename;
        emit pathRenamed(oldFilename, newFilename);
        return true;
    } else {
        m_lastError = QString("Failed to rename file: %1").arg(oldFile.errorString());
        qWarning() << "Failed to rename file:" << oldFile.errorString();
        return false;
    }
}

// 获取路径文件详细信息：创建时间、点数、文件大小
QString PathManager::getPathInfo(const QString& filename) const
{
    QString fullPath = m_dataDirectory + "/" + filename;
    QFileInfo fileInfo(fullPath);

    if (!fileInfo.exists()) {
        qDebug() << "File does not exist for path info";
        return QString();
    }

    QFile file(fullPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open file for path info:" << file.errorString();
        return QString();
    }

    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_5_15);

    QString version;
    QDateTime saveTime;
    int pointCount;

    stream >> version >> saveTime >> pointCount;

    file.close();

    QString info = QString("Created: %1\nPoints: %2\nSize: %3 bytes")
                   .arg(saveTime.toString())
                   .arg(pointCount)
                   .arg(fileInfo.size());

    return info;
}

// 设置数据目录路径
void PathManager::setDataDirectory(const QString& directory)
{
    m_dataDirectory = directory;
}

// 获取当前数据目录路径
QString PathManager::dataDirectory() const
{
    return m_dataDirectory;
}

// 生成基于日期时间的文件名
QString PathManager::generateDateBasedFilename() const
{
    QDateTime now = QDateTime::currentDateTime();
    QString filename = now.toString("yyyy-MM-dd_hh-mm-ss") + ".mpath";
    return filename;
}

// 获取文件的友好显示名称：将日期格式转换为可读格式
QString PathManager::getDisplayName(const QString& filename) const
{
    QString baseName = QFileInfo(filename).baseName();

    // If it's a date-based filename, format it nicely
    QDateTime dateTime = QDateTime::fromString(baseName, "yyyy-MM-dd_hh-mm-ss");
    if (dateTime.isValid()) {
        return dateTime.toString("yyyy-MM-dd hh:mm:ss");
    }

    return baseName;
}

// 确保数据目录存在：不存在则创建
QString PathManager::ensureDataDirectory()
{
    QDir dir;
    if (!dir.exists(m_dataDirectory)) {
        dir.mkpath(m_dataDirectory);
    }
    return m_dataDirectory;
}

// 检查文件是否为有效的路径文件（.mpath格式）
bool PathManager::isValidPathFile(const QString& filename) const
{
    return filename.endsWith(".mpath");
}

// 获取最后一次错误信息
QString PathManager::getLastError() const
{
    return m_lastError;
}
