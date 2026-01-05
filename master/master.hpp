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
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include "color.hpp"

using namespace raytracer;

class RaytracerServiceImpl final : public RaytracerService::Service {
private:
    struct RenderSettings {
        int image_width;
        int image_height;
        int tile_size;
        int samples_per_pixel;
        int max_depth;
    };

    struct AssignedTask {
        RenderTask task;
        std::string worker_id;
        std::chrono::steady_clock::time_point leased_at;
    };

    static std::queue<RenderTask> create_work_queue(int image_width, int image_height, int tile_size, int samples, int depth);
    RenderConfig build_config_proto() const;
    void reclaim_expired_tasks_locked();
    bool validate_worker(const std::string& worker_id) const;

public:
    RaytracerServiceImpl(const scene& sc, int image_width, int image_height, int tile_size, int samples, int depth, std::string output_path);

    grpc::Status HealthCheck(grpc::ServerContext* context, const google::protobuf::Empty* request, HealthCheckResponse* response) override;
    grpc::Status RegisterWorker(grpc::ServerContext* context, const WorkerRegistrationRequest* request, WorkerRegistrationResponse* response) override;
    grpc::Status RequestTask(grpc::ServerContext* context, const WorkRequest* request, TaskAssignment* response) override;
    grpc::Status SubmitResult(grpc::ServerContext* context, const SubmitResultRequest* request, google::protobuf::Empty* response) override;

    void wait_for_completion();

private:
    void save_image();

    const SceneData scene_data_;
    std::queue<RenderTask> work_queue_;
    std::unordered_map<int32_t, AssignedTask> in_progress_;
    std::unordered_set<std::string> registered_workers_;
    const int total_tiles_;
    std::atomic<int> tiles_completed_;
    const int image_width_;
    const int image_height_;
    const RenderSettings settings_;
    const std::string output_path_;
    std::atomic<int> next_worker_id_;
    const std::chrono::seconds lease_timeout_;
    
    std::mutex mtx_;
    std::condition_variable all_done_cv_;
    std::vector<color> final_image_pixels_;
};

void RunServer(const scene& sc, int image_width, int image_height, int tile_size, int samples, int depth, const std::string& address, const std::string& output_path);

#endif 
