#include "FileManager.h"

#include <filesystem>
#include <fstream>

#include "Utils.h"

namespace fs = std::filesystem;

FileManager::FileManager(std::string dataDirectory) : dataDirectory_(std::move(dataDirectory)) {}

std::string FileManager::pathFor(const std::string& fileName) const {
    return (fs::path(dataDirectory_) / fileName).string();
}

bool FileManager::ensureDataDirectory(std::string& error) const {
    std::error_code ec;
    fs::create_directories(fs::path(dataDirectory_), ec);
    if (ec || !fs::is_directory(fs::path(dataDirectory_), ec)) {
        error = "Cannot create or access the data folder '" + dataDirectory_ + "'" +
                (ec ? ": " + ec.message() : ".");
        return false;
    }
    return true;
}

FileManager::ReadStatus FileManager::readLines(const std::string& fileName, std::vector<std::string>& lines,
                                               std::string& error) const {
    const fs::path path = pathFor(fileName);
    std::error_code ec;
    if (!fs::exists(path, ec)) {
        // A crash between the two rename steps of a save can leave only the backup behind: recover it.
        const fs::path backup = path.string() + ".bak";
        if (fs::exists(backup, ec)) {
            fs::rename(backup, path, ec);
        }
        if (!fs::exists(path, ec)) return ReadStatus::Missing;
    }
    std::ifstream in(path);
    if (!in) {
        error = "Cannot open '" + path.string() + "' for reading. The file was left untouched.";
        return ReadStatus::Failed;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (util::trim(line).empty() || line[0] == '#') continue;
        lines.push_back(line);
    }
    if (in.bad()) {
        error = "A read error occurred while reading '" + path.string() + "'.";
        return ReadStatus::Failed;
    }
    return ReadStatus::Ok;
}

bool FileManager::writeLinesAtomic(const std::string& fileName, const std::vector<std::string>& lines,
                                   std::string& error) const {
    if (!ensureDataDirectory(error)) return false;
    const fs::path path = pathFor(fileName);
    const fs::path temp = path.string() + ".tmp";
    const fs::path backup = path.string() + ".bak";
    std::error_code ec;

    {
        std::ofstream out(temp, std::ios::trunc);
        if (!out) {
            error = "Cannot write to '" + temp.string() + "'. Check disk space and permissions.";
            return false;
        }
        for (const std::string& line : lines) out << line << '\n';
        out.flush();
        if (!out) {
            out.close();
            fs::remove(temp, ec);
            error = "Writing to '" + temp.string() + "' failed. The existing data file was not changed.";
            return false;
        }
    }

    // Preferred: atomic replace. Fallback (older Windows runtimes refuse to overwrite): keep a backup.
    fs::rename(temp, path, ec);
    if (!ec) return true;

    ec.clear();
    fs::remove(backup, ec);
    ec.clear();
    if (fs::exists(path, ec)) {
        fs::rename(path, backup, ec);
        if (ec) {
            std::error_code ignore;
            fs::remove(temp, ignore);
            error = "Cannot replace '" + path.string() + "': " + ec.message();
            return false;
        }
    }
    ec.clear();
    fs::rename(temp, path, ec);
    if (ec) {
        std::error_code ignore;
        fs::rename(backup, path, ignore);        // restore the previous version
        fs::remove(temp, ignore);
        error = "Cannot finish saving '" + path.string() + "': " + ec.message();
        return false;
    }
    std::error_code ignore;
    fs::remove(backup, ignore);
    return true;
}

bool FileManager::backupCorrupted(const std::string& fileName, std::string& backupName) const {
    std::string stamp = util::currentTimestamp();
    for (char& c : stamp) {
        if (c == ' ' || c == ':') c = '-';
    }
    const fs::path source = pathFor(fileName);
    const fs::path target = source.string() + ".corrupt-" + stamp + ".bak";
    std::error_code ec;
    fs::copy_file(source, target, fs::copy_options::overwrite_existing, ec);
    if (ec) return false;
    backupName = target.filename().string();
    return true;
}

std::string FileManager::escape(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '|': out += "\\p"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            default: out += c;
        }
    }
    return out;
}

bool FileManager::unescape(const std::string& text, std::string& out) {
    out.clear();
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] != '\\') {
            out += text[i];
            continue;
        }
        if (++i >= text.size()) return false;
        switch (text[i]) {
            case '\\': out += '\\'; break;
            case 'p': out += '|'; break;
            case 'n': out += '\n'; break;
            case 'r': out += '\r'; break;
            default: return false;
        }
    }
    return true;
}

std::string FileManager::join(const std::vector<std::string>& fields) {
    std::string line;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) line += '|';
        line += escape(fields[i]);
    }
    return line;
}

bool FileManager::split(const std::string& line, std::vector<std::string>& fields) {
    fields.clear();
    std::size_t start = 0;
    while (true) {
        const std::size_t bar = line.find('|', start);
        const std::string raw = line.substr(start, bar == std::string::npos ? std::string::npos : bar - start);
        std::string value;
        if (!unescape(raw, value)) return false;
        fields.push_back(std::move(value));
        if (bar == std::string::npos) break;
        start = bar + 1;
    }
    return true;
}
