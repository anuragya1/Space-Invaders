#include "sdl3_file_browser.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <system_error>

namespace si {

namespace {

namespace fs = std::filesystem;

std::string normalize_extension(std::string ext) {
    if (!ext.empty() && ext.front() != '.') ext.insert(ext.begin(), '.');
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

std::string path_key(const fs::path& path) {
    std::error_code ec;
    fs::path absolute = fs::absolute(path, ec);
    if (ec) return path.lexically_normal().string();

    fs::path canonical = fs::weakly_canonical(absolute, ec);
    if (!ec) return canonical.string();
    return absolute.lexically_normal().string();
}

std::string dir_label(const fs::path& dir) {
    std::error_code ec;
    fs::path cwd = fs::current_path(ec);
    if (!ec && path_key(dir) == path_key(cwd)) return ".";

    std::string name = dir.filename().string();
    if (name.empty()) name = dir.string();
    return name;
}

bool has_extension(const fs::path& path, const std::string& ext) {
    std::string pathExt = path.extension().string();
    std::transform(pathExt.begin(), pathExt.end(), pathExt.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return pathExt == ext;
}

void add_directory(std::vector<FileBrowserItem>& out,
                   std::vector<std::string>& seen,
                   const fs::path& dir,
                   const std::string& ext) {
    std::error_code ec;
    if (dir.empty() || !fs::exists(dir, ec) || !fs::is_directory(dir, ec)) {
        return;
    }

    const std::string label = dir_label(dir);
    fs::directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
    if (ec) return;

    for (const auto& entry : it) {
        std::error_code entryEc;
        if (!entry.is_regular_file(entryEc) || entryEc) continue;
        if (!has_extension(entry.path(), ext)) continue;

        const std::string key = path_key(entry.path());
        if (std::find(seen.begin(), seen.end(), key) != seen.end()) continue;
        seen.push_back(key);

        std::string itemLabel = entry.path().filename().string();
        if (!label.empty()) itemLabel += "  [" + label + "]";
        out.push_back({ itemLabel, entry.path().string() });
    }
}

void add_relative_dirs(std::vector<FileBrowserItem>& out,
                       std::vector<std::string>& seen,
                       const fs::path& root,
                       const std::vector<std::string>& dirs,
                       const std::string& ext) {
    for (const auto& rel : dirs) {
        if (!rel.empty()) add_directory(out, seen, root / rel, ext);
    }
}

}

std::vector<FileBrowserItem> find_browser_files(const FileBrowserOptions& options) {
    std::vector<FileBrowserItem> out;
    std::vector<std::string> seen;
    const std::string ext = normalize_extension(options.extension);
    if (ext.empty()) return out;

    std::error_code ec;
    const fs::path cwd = fs::current_path(ec);
    if (!ec) {
        if (options.includeCwd) add_directory(out, seen, cwd, ext);
        add_relative_dirs(out, seen, cwd, options.relativeDirs, ext);

        if (options.includeCwdParent) {
            add_directory(out, seen, cwd.parent_path(), ext);
        }

        if (options.includeBuildPrefixedDirs) {
            fs::directory_iterator it(cwd, fs::directory_options::skip_permission_denied, ec);
            if (!ec) {
                for (const auto& entry : it) {
                    std::error_code entryEc;
                    if (!entry.is_directory(entryEc) || entryEc) continue;
                    const std::string name = entry.path().filename().string();
                    if (name.rfind("build-", 0) == 0) {
                        add_directory(out, seen, entry.path(), ext);
                    }
                }
            }
        }
    }

    const char* base = SDL_GetBasePath();
    if (base && *base) {
        const fs::path baseDir(base);
        if (options.includeExeDir) add_directory(out, seen, baseDir, ext);
        add_relative_dirs(out, seen, baseDir, options.relativeDirs, ext);
        if (options.includeExeParent) {
            add_directory(out, seen, baseDir.parent_path(), ext);
        }
    }

    std::sort(out.begin(), out.end(),
        [](const FileBrowserItem& a, const FileBrowserItem& b) {
            return a.label < b.label;
        });
    return out;
}

std::vector<MenuItem> make_file_browser_menu(
        const std::vector<FileBrowserItem>& files,
        const FileBrowserOptions& options,
        const std::vector<MenuItem>& extraItems,
        int backTag) {
    std::vector<MenuItem> items;
    items.reserve(files.size() + extraItems.size() + 2);

    if (files.empty()) {
        items.push_back({ options.emptyLabel, -1, false });
    } else {
        for (std::size_t i = 0; i < files.size(); ++i) {
            items.push_back({ files[i].label, static_cast<int>(i) });
        }
    }

    items.insert(items.end(), extraItems.begin(), extraItems.end());
    items.push_back({ "Back", backTag });
    return items;
}

}
