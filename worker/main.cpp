#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "cxxopts.hpp"
#include "worker.hpp"

int main(int argc, char** argv) {
    cxxopts::Options options("Raytracer Worker", "A worker node for the distributed raytracer.");
    options.add_options()
        ("a,address", "Master address", cxxopts::value<std::string>()->default_value("localhost:50051"));
    
    auto result = options.parse(argc, argv);
    auto master_address = result["address"].as<std::string>();
    
    try {
        RaytracerWorker worker(grpc::CreateChannel(master_address, grpc::InsecureChannelCredentials()));
        std::cout << "Worker attempting to connect to master at " << master_address << std::endl;
        worker.run();
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
