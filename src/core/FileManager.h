// FileManager.h - all file handling: data directory, safe saving, loading and record encoding.
#pragma once
#include <string>
#include <type_traits>
#include <vector>

#include "Serializable.h"

class FileManager {
public:
    enum class ReadStatus { Ok, Missing, Failed };

    explicit FileManager(std::string dataDirectory = "Data");

    const std::string& dataDirectory() const { return dataDirectory_; }
    std::string pathFor(const std::string& fileName) const;
    bool ensureDataDirectory(std::string& error) const;

    ReadStatus readLines(const std::string& fileName, std::vector<std::string>& lines, std::string& error) const;
    bool writeLinesAtomic(const std::string& fileName, const std::vector<std::string>& lines, std::string& error) const;
    bool backupCorrupted(const std::string& fileName, std::string& backupName) const;

    // Record encoding: fields are separated by '|'. '\', '|' and line breaks are escaped.
    static std::string join(const std::vector<std::string>& fields);
    static bool split(const std::string& line, std::vector<std::string>& fields);

    // Saves any vector of Serializable objects (polymorphic through the interface).
    template <typename T>
    bool saveRecords(const std::string& fileName, const std::vector<T>& records, std::string& error) const {
        static_assert(std::is_base_of<Serializable, T>::value, "records must implement Serializable");
        std::vector<std::string> lines;
        lines.reserve(records.size());
        for (const T& record : records) {
            const Serializable& s = record;
            lines.push_back(s.serialize());
        }
        return writeLinesAtomic(fileName, lines, error);
    }

    // Loads records using T::parse(fields). Malformed lines are skipped, counted, reported in
    // `warnings`, and the original file is copied to a .corrupt-*.bak file before it can be rewritten.
    template <typename T>
    bool loadRecords(const std::string& fileName, std::vector<T>& out, std::vector<std::string>& warnings,
                     std::string& fatalError) const {
        std::vector<std::string> lines;
        std::string error;
        const ReadStatus status = readLines(fileName, lines, error);
        if (status == ReadStatus::Missing) return true;          // first run: nothing to load
        if (status == ReadStatus::Failed) {
            fatalError = error;
            return false;
        }
        std::size_t bad = 0;
        for (const std::string& line : lines) {
            std::vector<std::string> fields;
            if (!split(line, fields)) { ++bad; continue; }
            auto parsed = T::parse(fields);
            if (!parsed) { ++bad; continue; }
            out.push_back(std::move(*parsed));
        }
        if (bad > 0) {
            std::string backup;
            const bool saved = backupCorrupted(fileName, backup);
            warnings.push_back(fileName + ": " + std::to_string(bad) + " malformed record(s) skipped. " +
                               (saved ? "Original copied to " + backup + "." : "Could not create a backup copy!"));
        }
        return true;
    }

private:
    static std::string escape(const std::string& text);
    static bool unescape(const std::string& text, std::string& out);

    std::string dataDirectory_;
};
