#pragma once

#include <spdlog/spdlog.h>

#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace dorado::tests {

std::filesystem::path get_data_dir(const std::string& sub_dir);

// Reads into a string.
std::string ReadFileIntoString(const std::filesystem::path& path);

// Reads into a vector<uint8_t>.
std::vector<uint8_t> ReadFileIntoVector(const std::filesystem::path& path);

#define get_fast5_data_dir() get_data_dir("fast5")

#define get_pod5_data_dir() get_data_dir("pod5")

#define get_single_pod5_file_path() (get_data_dir("pod5") / "single_na24385.pod5")

#define get_nested_pod5_data_dir() get_data_dir("nested_pod5_folder")

#define get_stereo_data_dir() get_data_dir("stereo")

#define get_aligner_data_dir() get_data_dir("aligner_test")

// Wrapper around a temporary directory since one doesn't exist in the standard
struct TempDir {
    TempDir(std::filesystem::path path) : m_path(std::move(path)) {}
    ~TempDir() { std::filesystem::remove_all(m_path); }

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    std::filesystem::path m_path;
};

class TraceLogger {
public:
    TraceLogger() { spdlog::set_level(spdlog::level::trace); }
    ~TraceLogger() { spdlog::set_level(spdlog::level::off); }
};

}  // namespace dorado::tests

using namespace dorado::tests;
