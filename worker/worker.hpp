#ifndef WORKER_H
#define WORKER_H

#include <grpcpp/grpcpp.h>
#include "raytracer.grpc.pb.h"
#include <memory>
#include <vector>
#include "color.hpp"

using namespace raytracer;

class RaytracerWorker {
public:
    RaytracerWorker(std::shared_ptr<grpc::Channel> channel);
    void run();

private:
    bool health_check();
    void submit_result(const Tile& tile, const std::vector<color>& pixels);

    std::unique_ptr<RaytracerService::Stub> stub_;
};

#endif 
