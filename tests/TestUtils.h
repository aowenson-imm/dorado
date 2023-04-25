#pragma once

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

static std::string get_data_dir(const std::string& sub_dir) {
    const std::filesystem::path data_path = std::filesystem::path("./tests/data/") / sub_dir;

    if (!std::filesystem::exists(data_path)) {
        std::cerr << "Datapath " << data_path
                  << " does not exist, exiting.\n"
                     "Unit tests must be run from the build working directory inside dorado"
                  << std::endl;
        exit(1);
    }
    return data_path.string();
}

// Reads into a string.
static std::string ReadFileIntoString(const std::filesystem::path& path) {
    const auto num_bytes = std::filesystem::file_size(path);
    std::string content;
    content.resize(num_bytes);
    std::ifstream in_file(path.c_str(), std::ios::in | std::ios::binary);
    in_file.read(content.data(), content.size());
    return content;
}

#define get_fast5_data_dir() get_data_dir("fast5")

#define get_pod5_data_dir() get_data_dir("pod5")

#define get_stereo_data_dir() get_data_dir("stereo")

#define get_aligner_data_dir() get_data_dir("aligner_test")
