#pragma once

#include "QtGui/private/qzipwriter_p.h"

#include <qfileinfolist>
#include <string>

namespace utils {

#define FILE_MAX_SIZE 1024

class Utils {

  public:
    Utils()  = default;
    ~Utils() = default;

    /// @brief 压缩 zip 文件到当前路径
    /// @param file_path
    /// @param save_path
    /// @param remove 是否需要删除原文件
    static bool compressionZipFile(const std::string &file_path, bool remove = false);

    /// @brief 压缩 zip 文件
    /// @param file_path
    /// @param save_path
    /// @param remove 是否需要删除原文件
    static bool compressionZipFile(const std::string &file_path, const std::string &save_path, bool remove = false);

    /// @brief 解压缩 zip 文件
    /// @param selectZipFilePath
    /// @param save_path
    /// @param remove 是否需要删除原文件
    static bool decompressionZipFile(const std::string &file_path, const std::string &save_path, bool remove = false);

    /// @brief 获取当前时间
    static std::string now();

  private:
    static QFileInfoList ergodic_compression_file(QZipWriter *writer, const QString &rootPath, QString dirPath);

    /// @brief 删除文件或文件夹
    /// @param strPath 要删除的文件夹或文件的路径
    static bool delete_file_or_folder(const std::string &str_path);
};

} // namespace utils