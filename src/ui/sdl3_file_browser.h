#pragma once

#include "sdl3_menu.h"

#include <string>
#include <vector>

namespace si {

constexpr int FILE_BROWSER_BACK = -30000;

struct FileBrowserItem {
    std::string label;
    std::string path;
};

struct FileBrowserOptions {
    std::string extension;
    std::string emptyLabel = "(no files found)";
    std::vector<std::string> relativeDirs;
    bool includeCwd = true;
    bool includeCwdParent = false;
    bool includeExeDir = true;
    bool includeExeParent = false;
    bool includeBuildPrefixedDirs = false;
};

std::vector<FileBrowserItem> find_browser_files(const FileBrowserOptions& options);

std::vector<MenuItem> make_file_browser_menu(
    const std::vector<FileBrowserItem>& files,
    const FileBrowserOptions& options,
    const std::vector<MenuItem>& extraItems = {},
    int backTag = FILE_BROWSER_BACK);

}
