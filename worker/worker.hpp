#ifndef WORKER_H
#define WORKER_H

#include <grpcpp/grpcpp.h>
#include "raytracer.grpc.pb.h"
#include <memory>
#include <vector>
#include <string>
#include "color.hpp"
#include "hittable.hpp"
#include "camera.hpp"

using namespace raytracer;

class RaytracerWorker {
public:
    RaytracerWorker(std::shared_ptr<grpc::Channel> channel, std::string hostname);
    void run();

private:
    enum class TaskFetchResult {
        TaskReceived,
        NoMoreTasks,
        Retry
    };

    bool health_check();
    bool register_with_master();
    TaskFetchResult request_task(RenderTask& task);
    bool submit_result(const Tile& tile, const std::vector<color>& pixels);
    std::unique_ptr<camera> build_camera_from_proto(const raytracer::Camera& proto_cam) const;

    std::string hostname_;
    std::unique_ptr<RaytracerService::Stub> stub_;
    std::string worker_id_;
    RenderConfig config_;
    SceneData scene_cache_;
    std::shared_ptr<hittable> world_;
    std::unique_ptr<camera> camera_;
};

#endif 
