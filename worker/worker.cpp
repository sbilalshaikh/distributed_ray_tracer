#include "worker.hpp"

#include <grpcpp/grpcpp.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "renderer.hpp"
#include "hittable.hpp"
#include "serialization.hpp"

using grpc::ClientContext;
using grpc::Status;

RaytracerWorker::RaytracerWorker(std::shared_ptr<grpc::Channel> channel, std::string hostname)
    : hostname_(std::move(hostname)),
      stub_(RaytracerService::NewStub(std::move(channel))) {}

void RaytracerWorker::run() {
    if (!register_with_master()) {
        return;
    }

    if (!health_check()) {
        return;
    }

    while (true) {
        RenderTask task;
        TaskFetchResult result = request_task(task);
        if (result == TaskFetchResult::NoMoreTasks) {
            break;
        }
        if (result == TaskFetchResult::Retry) {
            continue;
        }

        const auto& tile = task.tile();
        std::cout << worker_id_ << " rendering tile (" << tile.x0() << ", " << tile.y0() << ")" << std::endl;

        uint64_t seed = static_cast<uint64_t>(tile.task_id()) * 7919ULL + 17ULL;

        renderer rend(*camera_, *world_);
        std::vector<color> pixels = rend.render_tile(
            tile.x0(),
            tile.y0(),
            tile.width(),
            tile.height(),
            task.samples_per_pixel(),
            task.max_depth(),
            seed
        );
        
        if (!submit_result(tile, pixels)) {
            break;
        }
    }

    std::cout << worker_id_ << " finished - no more work." << std::endl;
}

bool RaytracerWorker::register_with_master() {
    ClientContext context;
    WorkerRegistrationRequest request;
    request.set_hostname(hostname_);
    WorkerRegistrationResponse response;

    Status status = stub_->RegisterWorker(&context, request, &response);
    if (!status.ok()) {
        std::cerr << "Worker registration failed: " << status.error_message() << std::endl;
        return false;
    }

    worker_id_ = response.worker_id();
    config_ = response.config();
    scene_cache_ = response.scene();

    world_ = deserialize_scene(scene_cache_);
    if (!world_) {
        std::cerr << "Failed to build scene from master response." << std::endl;
        return false;
    }

    camera_ = build_camera_from_proto(scene_cache_.camera());
    if (!camera_) {
        std::cerr << "Failed to construct camera from master response." << std::endl;
        return false;
    }

    std::cout << "Registered as " << worker_id_ << " with master." << std::endl;
    return true;
}

bool RaytracerWorker::health_check() {
    ClientContext context;
    google::protobuf::Empty request;
    HealthCheckResponse response;
    
    Status status = stub_->HealthCheck(&context, request, &response);
    
    if (!status.ok()) {
        std::cerr << "Health check RPC failed: " << status.error_message() << std::endl;
        return false;
    }
    
    if (response.status() != HealthCheckResponse::SERVING) {
        std::cerr << "Master is not serving." << std::endl;
        return false;
    }
    
    return true;
}

RaytracerWorker::TaskFetchResult RaytracerWorker::request_task(RenderTask& task) {
    ClientContext context;
    WorkRequest request;
    request.set_worker_id(worker_id_);

    TaskAssignment response;
    Status status = stub_->RequestTask(&context, request, &response);

    if (!status.ok()) {
        if (status.error_code() == grpc::StatusCode::UNAUTHENTICATED) {
            std::cerr << "Master no longer recognizes " << worker_id_
                      << ". Attempting to re-register..." << std::endl;
            if (register_with_master()) {
                return TaskFetchResult::Retry;
            }
            return TaskFetchResult::Retry;
        }
        std::cerr << "Failed to request task: " << status.error_message() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return TaskFetchResult::Retry;
    }

    if (!response.has_assignment()) {
        return TaskFetchResult::NoMoreTasks;
    }

    task = response.task();
    return TaskFetchResult::TaskReceived;
}

bool RaytracerWorker::submit_result(const Tile& tile, const std::vector<color>& pixels) {
    ClientContext context;
    SubmitResultRequest request;
    request.set_worker_id(worker_id_);

    auto* result = request.mutable_result();
    result->mutable_tile()->CopyFrom(tile);
    
    std::string buffer;
    buffer.reserve(pixels.size() * 3);
    for (const auto& p : pixels) {
        buffer.push_back(static_cast<char>(static_cast<uint8_t>(255.999 * p.x())));
        buffer.push_back(static_cast<char>(static_cast<uint8_t>(255.999 * p.y())));
        buffer.push_back(static_cast<char>(static_cast<uint8_t>(255.999 * p.z())));
    }
    result->set_pixel_data(std::move(buffer));

    google::protobuf::Empty response;
    Status status = stub_->SubmitResult(&context, request, &response);
    if (!status.ok()) {
        if (status.error_code() == grpc::StatusCode::UNAUTHENTICATED) {
            std::cerr << "SubmitResult rejected (unauthenticated). Re-registering..." << std::endl;
            return register_with_master();
        }
        std::cerr << "SubmitResult failed: " << status.error_message() << std::endl;
        return false;
    }
    return true;
}

std::unique_ptr<camera> RaytracerWorker::build_camera_from_proto(const raytracer::Camera& proto_cam) const {
    const double aspect_ratio = static_cast<double>(config_.image_width()) / config_.image_height();
    return std::make_unique<camera>(
        proto_to_vec3(proto_cam.position()),
        proto_to_vec3(proto_cam.look_at()),
        proto_to_vec3(proto_cam.up()),
        proto_cam.vfov(),
        aspect_ratio,
        config_.image_width(),
        config_.image_height()
    );
}
