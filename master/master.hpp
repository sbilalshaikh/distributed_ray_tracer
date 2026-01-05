#ifndef MASTER_H
#define MASTER_H

#include <grpcpp/grpcpp.h>
#include "raytracer.grpc.pb.h"
#include "scene.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include "color.hpp"

using namespace raytracer;

class RaytracerServiceImpl final : public RaytracerService::Service {
private:
    static std::queue<RenderTask> create_work_queue(const SceneData& scene_data, int image_width, int image_height, int tile_size, int samples, int depth);

public:
    RaytracerServiceImpl(const scene& sc, int image_width, int image_height, int tile_size, int samples, int depth, std::string output_path);

    grpc::Status HealthCheck(grpc::ServerContext* context, const google::protobuf::Empty* request, HealthCheckResponse* response) override;
    grpc::Status RequestWork(grpc::ServerContext* context, const google::protobuf::Empty* request, grpc::ServerWriter<RenderTask>* writer) override;
    grpc::Status SubmitResult(grpc::ServerContext* context, const TileResult* result, google::protobuf::Empty* response) override;

    void wait_for_completion();

private:
    bool get_next_task(RenderTask& task);
    void save_image();

    const SceneData scene_data_;
    std::queue<RenderTask> work_queue_;
    const int total_tiles_;
    std::atomic<int> tiles_completed_;
    const int image_width_;
    const std::string output_path_;
    
    std::mutex mtx_;
    std::condition_variable all_done_cv_;
    std::vector<color> final_image_pixels_;
};

void RunServer(const scene& sc, int image_width, int image_height, int tile_size, int samples, int depth, const std::string& address, const std::string& output_path);

#endif // MASTER_H
