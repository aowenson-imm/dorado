#include <iostream>
#include <filesystem>
#include <argparse.hpp>

#include "nn/ModelRunner.h"
#include "decode/CPUDecoder.h"
#include "decode/GPUDecoder.h"
#include "data_loader/Fast5DataLoader.h"
#include "read_pipeline/ScalerNode.h"
#include "read_pipeline/BasecallerNode.h"
#include "read_pipeline/WriterNode.h"


void setup(const std::string& model_path, const std::string& data_path, const std::string& device,
        size_t chunk_size, size_t overlap, size_t batch_size, size_t num_runners, bool emit_sam) {

    std::vector<Runner> runners;
    auto decode_options = DecoderOptions();

    if (device == "cpu") {
        for (int i = 0; i < num_runners; i++) {
            runners.push_back(std::make_shared<ModelRunner<CPUDecoder>>(model_path, device, chunk_size, batch_size, decode_options));
        }
    } else {
        for (int i = 0; i < num_runners; i++) {
            runners.push_back(std::make_shared<ModelRunner<GPUDecoder>>(model_path, device, chunk_size, batch_size, decode_options));
        }
    }

    WriterNode writer_node(emit_sam);
    BasecallerNode basecaller_node(writer_node, runners, batch_size, chunk_size, overlap);
    ScalerNode scaler_node(basecaller_node);
    Fast5DataLoader loader(scaler_node, "cpu");
    loader.load_reads(data_path);
}


int main(int argc, char *argv[]) {

    argparse::ArgumentParser parser("dorado", "0.0.1a0");

    parser.add_argument("model")
            .help("the basecaller model to run.");

    parser.add_argument("data")
            .help("the data directory.");

    parser.add_argument("-x", "--device")
#ifdef __APPLE__
            .default_value(std::string{"metal"});
#else
            .default_value(std::string{"cuda:0"});
#endif

    parser.add_argument("-b", "--batchsize")
            .default_value(1024)
            .scan<'i', int>();

    parser.add_argument("-c", "--chunksize")
            .default_value(8000)
            .scan<'i', int>();

    parser.add_argument("-o", "--overlap")
            .default_value(150)
            .scan<'i', int>();

    parser.add_argument("-r", "--num_runners")
            .default_value(1)
            .scan<'i', int>();

    parser.add_argument("--emit-sam")
            .default_value(false)
            .implicit_value(true);

    try {
        parser.parse_args(argc, argv);
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        std::exit(1);
    }

    std::filesystem::path exe_path{argv[0]};
    auto libdir = exe_path.parent_path() /  ".." / "lib";
    setenv("HDF5_PLUGIN_PATH", libdir.c_str(), 0);

    std::cerr << "> Creating basecall pipeline" << std::endl;
    try {
        setup(
            parser.get<std::string>("model"),
            parser.get<std::string>("data"),
            parser.get<std::string>("-x"),
            parser.get<int>("-c"),
            parser.get<int>("-o"),
            parser.get<int>("-b"),
            parser.get<int>("-r"),
            parser.get<bool>("--emit-sam")
        );
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::exit(1);
    }

    std::cerr << "> Finished" << std::endl;

}
