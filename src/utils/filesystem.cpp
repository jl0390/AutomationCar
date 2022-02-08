#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <string.h>

#ifndef WIN32
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>
#endif

#include "filesystem.h"

namespace fs {
    std::string locateFile(const std::string& filepathSuffix, const std::vector<std::string>& directories)
    {
        const int MAX_DEPTH = 10;
        bool found = false;
        std::string filepath;

        for (int i = 0; i < (int)directories.size(); i++)
        {
            const std::string &dir = directories[i];
            if (!dir.empty() && dir.back() != '/')
            {
#ifdef _MSC_VER
                filepath = dir + "\\" + filepathSuffix;
#else
                filepath = dir + "/" + filepathSuffix;
#endif
            }
            else
                filepath = dir + filepathSuffix;

            for (int i = 0; i < MAX_DEPTH && !found; i++)
            {
                std::ifstream checkFile(filepath);
                found = checkFile.is_open();
                if (found)
                    break;
                filepath = "../" + filepath; // Try again in parent dir
            }

            if (found)
            {
                break;
            }

            filepath.clear();
        }

        return filepath;
    }

    std::string locateFile(const std::string& filepathSuffix, const std::string& dirname)
    {
        std::vector<std::string> dirs;
        dirs.push_back(dirname);
        return locateFile(filepathSuffix, dirs);
    }

    // fileExtension
    std::string fileExtension(const std::string& path)
    {
        std::string ext = path.substr(path.find_last_of(".") + 1);

        std::transform(ext.begin(), ext.end(), ext.begin(), tolower);

        return ext;
    }

    std::string baseName(const std::string &path)
    {
#if WIN32
        char fname[_MAX_PATH];
        _splitpath(path.c_str(), NULL, NULL, fname, NULL);
#else
        std::string fname = basename((char*)path.c_str());
        std::string::size_type pos = fname.find_last_of('.');
        if (pos != std::string::npos)
            fname=fname.substr(0,pos);
#endif
        return std::string(fname);
    }

    bool fileExist(const char *fname)
    {
        std::ifstream ifs(fname);
        if (ifs.is_open()) {
            ifs.close();
#ifndef WIN32
            DIR *dir = opendir(fname);
            if (dir != NULL) {
                closedir(dir);
                return false;
            }
#endif
            return true;
        }
        return false;
    }
}
