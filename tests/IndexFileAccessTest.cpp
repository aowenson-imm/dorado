#include "alignment/IndexFileAccess.h"

#include "TestUtils.h"
#include "alignment/Minimap2Index.h"
#include "utils/stream_utils.h"

#include <catch2/catch.hpp>

#include <filesystem>

#define TEST_GROUP "[alignment::IndexFileAccess]"

using namespace dorado::utils;

namespace {

// Format of each line is @SQ\tSN:{sequence_name}\tLN:{sequence_len}
// where read_id is used as the sequence name
// @read_0 1898
const std::string_view EXPECTED_REF_FILE_HEADER{"@SQ\tSN:read_0\tLN:1898"};
// >read_0 1000
// >read_1 1000
const std::string_view EXPECTED_2READ_REF_FILE_HEADER{
        "@SQ\tSN:read_0\tLN:1000\n@SQ\tSN:read_1\tLN:1000"};

const std::string& valid_reference_file() {
    static const std::string reference_file = []() {
        const std::string read_id{"aligner_node_test"};
        std::filesystem::path aligner_test_dir{get_aligner_data_dir()};
        auto ref = aligner_test_dir / "target.fq";
        return ref.string();
    }();
    return reference_file;
}

const std::string& valid_2read_reference_file() {
    static const std::string reference_file = []() {
        const std::string read_id{"aligner_node_test"};
        std::filesystem::path aligner_test_dir{get_aligner_data_dir()};
        auto ref = aligner_test_dir / "supplementary_aln_target.fa";
        return ref.string();
    }();
    return reference_file;
}

const dorado::alignment::Minimap2Options& invalid_options() {
    static const dorado::alignment::Minimap2Options result = []() {
        dorado::alignment::Minimap2Options options{dorado::alignment::dflt_options};
        options.bandwidth = options.bandwidth_long + 1;
        return options;
    }();
    return result;
}

dorado::alignment::IndexLoadResult load_index_no_stderr(
        dorado::alignment::IndexFileAccess& cut,
        const std::string& file,
        const dorado::alignment::Minimap2Options& options) {
    SuppressStderr no_stderr{};
    return cut.load_index(file, options, 1);
}

}  // namespace

namespace dorado::alignment::index_file_access {

TEST_CASE(TEST_GROUP " constructor does not throw", TEST_GROUP) {
    REQUIRE_NOTHROW(IndexFileAccess{});
}

TEST_CASE(TEST_GROUP " load_index with invalid file return reference_file_not_found ", TEST_GROUP) {
    IndexFileAccess cut{};

    REQUIRE(cut.load_index("invalid_file_path", dflt_options, 1) ==
            IndexLoadResult::reference_file_not_found);
}

TEST_CASE(TEST_GROUP " load_index with invalid options returns validation_error", TEST_GROUP) {
    IndexFileAccess cut{};

    REQUIRE(load_index_no_stderr(cut, valid_reference_file(), invalid_options()) ==
            IndexLoadResult::validation_error);
}

TEST_CASE(TEST_GROUP " load_index with valid arguments returns success", TEST_GROUP) {
    IndexFileAccess cut{};

    REQUIRE(cut.load_index(valid_reference_file(), dflt_options, 1) == IndexLoadResult::success);
}

SCENARIO(TEST_GROUP " Load and retrieve index files", TEST_GROUP) {
    IndexFileAccess cut{};

    GIVEN("No index loaded") {
        THEN("is_index_loaded returns false") {
            REQUIRE_FALSE(cut.is_index_loaded("blah", dflt_options));
        }
    }

    GIVEN("load_index called with valid file but invalid options") {
        load_index_no_stderr(cut, valid_reference_file(), invalid_options());
        THEN("is_index_loaded returns false") {
            REQUIRE_FALSE(cut.is_index_loaded(valid_reference_file(), invalid_options()));
        }
    }

    GIVEN("load_index called with valid file and default options") {
        Minimap2Options compatible_options{dflt_options};
        auto compatible_best_n = dflt_options.best_n_secondary + 1;
        compatible_options.best_n_secondary = compatible_best_n;

        cut.load_index(valid_reference_file(), dflt_options, 1);
        THEN("is_index_loaded returns true") {
            REQUIRE(cut.is_index_loaded(valid_reference_file(), dflt_options));
        }
        THEN("get_index returns non null index") {
            REQUIRE(cut.get_index(valid_reference_file(), dflt_options) != nullptr);
        }
        THEN("is_index_loaded with compatible mapping options returns false") {
            REQUIRE_FALSE(cut.is_index_loaded(valid_reference_file(), compatible_options));
        }
        AND_GIVEN("load_index called with same file and other valid indexing options") {
            Minimap2Options other_options{dflt_options};
            other_options.kmer_size = other_options.kmer_size + 1;
            cut.load_index(valid_reference_file(), other_options, 1);
            THEN("is_index_loaded with other options returns true") {
                REQUIRE(cut.is_index_loaded(valid_reference_file(), other_options));
            }
            THEN("get_index with other options returns a non-null index") {
                REQUIRE(cut.get_index(valid_reference_file(), other_options) != nullptr);
            }
            THEN("get_index with other options returns a different index") {
                auto default_options_index = cut.get_index(valid_reference_file(), dflt_options);
                REQUIRE(cut.get_index(valid_reference_file(), other_options) !=
                        default_options_index);
            }
            AND_GIVEN("unload_index called with original options") {
                cut.unload_index(valid_reference_file(), dflt_options);
                THEN("is_index_loaded with other options returns true") {
                    REQUIRE(cut.is_index_loaded(valid_reference_file(), other_options));
                }
                THEN("is_index_loaded with original options returns false") {
                    REQUIRE_FALSE(cut.is_index_loaded(valid_reference_file(), dflt_options));
                }
            }
        }

        AND_GIVEN("load_index called with same file and compatible mapping options") {
            CHECK(cut.load_index(valid_reference_file(), compatible_options, 1) ==
                  IndexLoadResult::success);
            THEN("get_index with compatible options returns a non-null index") {
                REQUIRE(cut.get_index(valid_reference_file(), compatible_options) != nullptr);
            }
            THEN("is_index_loaded with compatible options returns true") {
                REQUIRE(cut.is_index_loaded(valid_reference_file(), compatible_options));
            }
            THEN("is_index_loaded with original options returns true") {
                REQUIRE(cut.is_index_loaded(valid_reference_file(), dflt_options));
            }
            THEN("get_index with compatible options returns a Minimap2Index with updated mapping "
                 "options") {
                auto compatible_index = cut.get_index(valid_reference_file(), compatible_options);
                CHECK(compatible_index);

                REQUIRE(compatible_index->mapping_options().best_n == compatible_best_n);
            }
            THEN("get_index with original options returns a Minimap2Index with original mapping "
                 "options") {
                auto original_index = cut.get_index(valid_reference_file(), dflt_options);
                CHECK(original_index);

                REQUIRE(original_index->mapping_options().best_n == dflt_options.best_n_secondary);
            }
            THEN("get_index with compatible options returns a Minimap2Index with the same "
                 "underlying minimap index") {
                auto original_index = cut.get_index(valid_reference_file(), dflt_options);
                CHECK(original_index);
                auto compatible_index = cut.get_index(valid_reference_file(), compatible_options);
                CHECK(compatible_index);

                REQUIRE(compatible_index->index() == original_index->index());
            }
            AND_GIVEN("unload_index called with original options") {
                cut.unload_index(valid_reference_file(), dflt_options);
                THEN("is_index_loaded with compatible options returns false") {
                    REQUIRE_FALSE(cut.is_index_loaded(valid_reference_file(), compatible_options));
                }
                THEN("is_index_loaded with original options returns false") {
                    REQUIRE_FALSE(cut.is_index_loaded(valid_reference_file(), dflt_options));
                }
            }
        }
    }
}

TEST_CASE(TEST_GROUP " validate_options with invalid options returns false", TEST_GROUP) {
    bool result{};
    SuppressStderr::invoke([&result] { result = validate_options(invalid_options()); });

    REQUIRE_FALSE(result);
}

TEST_CASE(TEST_GROUP " validate_options with default options returns true", TEST_GROUP) {
    REQUIRE(validate_options(dflt_options));
}

TEST_CASE(TEST_GROUP " get_index called with compatible index returns non-null Minimap2Index",
          TEST_GROUP) {
    IndexFileAccess cut{};
    cut.load_index(valid_reference_file(), dflt_options, 1);
    Minimap2Options compatible_options{dflt_options};
    auto compatible_best_n = dflt_options.best_n_secondary + 1;
    compatible_options.best_n_secondary = compatible_best_n;

    REQUIRE(cut.get_index(valid_reference_file(), compatible_options) != nullptr);
}

TEST_CASE(
        TEST_GROUP
        " get_index called with compatible index returns index with the compatible mapping options",
        TEST_GROUP) {
    IndexFileAccess cut{};
    cut.load_index(valid_reference_file(), dflt_options, 1);
    Minimap2Options compatible_options{dflt_options};
    auto compatible_best_n = dflt_options.best_n_secondary + 1;
    compatible_options.best_n_secondary = compatible_best_n;

    auto index = cut.get_index(valid_reference_file(), compatible_options);

    REQUIRE(index->get_options() == compatible_options);
}

TEST_CASE(TEST_GROUP
          " generate_sequence_records_header with index for loaded index returns non-empty string",
          TEST_GROUP) {
    IndexFileAccess cut{};
    cut.load_index(valid_reference_file(), dflt_options, 1);
    auto header = cut.generate_sequence_records_header(valid_reference_file(), dflt_options);
    REQUIRE(!header.empty());
}

TEST_CASE(TEST_GROUP
          " generate_sequence_records_header with single read reference index returns expected "
          "header",
          TEST_GROUP) {
    IndexFileAccess cut{};
    cut.load_index(valid_reference_file(), dflt_options, 1);

    auto header = cut.generate_sequence_records_header(valid_reference_file(), dflt_options);

    REQUIRE(header == EXPECTED_REF_FILE_HEADER);
}

TEST_CASE(TEST_GROUP
          " generate_sequence_records_header with two read reference index returns expected header",
          TEST_GROUP) {
    IndexFileAccess cut{};
    cut.load_index(valid_2read_reference_file(), dflt_options, 1);

    auto header = cut.generate_sequence_records_header(valid_2read_reference_file(), dflt_options);

    REQUIRE(header == EXPECTED_2READ_REF_FILE_HEADER);
}

}  // namespace dorado::alignment::index_file_access