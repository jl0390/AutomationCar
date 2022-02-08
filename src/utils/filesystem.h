#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include <string>

namespace fs {
    std::string locateFile(const std::string& filepathSuffix, const std::vector<std::string>& directories);
    std::string locateFile(const std::string& filepathSuffix, const std::string& dirname);
    std::string fileExtension(const std::string& path);
    std::string baseName(const std::string &path);
    bool fileExist(const char *fname);
}

#endif //__FILESYSTEM_H__
