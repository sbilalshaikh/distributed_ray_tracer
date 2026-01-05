#include "master.hpp"
#include <iostream>
#include <fstream>
#include <grpcpp/grpcpp.h>

#include "serialization.hpp"
#include "color.hpp"

using grpc::Server;
using grpc::ServerBuilder;

RaytracerServiceImpl::RaytracerServiceImpl(const scene& sc, int image_width, int image_height, int tile_size, int samples, int depth, std::string output_path)
    : scene_data_(serialize_scene(sc)), 
      work_queue_(create_work_queue(image_width, image_height, tile_size, samples, depth)),
      total_tiles_(static_cast<int>(work_queue_.size())), 
      tiles_completed_(0), 
      image_width_(image_width),
      image_height_(image_height),
      settings_{image_width, image_height, tile_size, samples, depth},
      output_path_(std::move(output_path)),
      next_worker_id_(1),
      lease_timeout_(std::chrono::seconds(120)) {
    
    final_image_pixels_.resize(static_cast<size_t>(image_width) * static_cast<size_t>(image_height));
    std::cout << "Master: " << total_tiles_ << " tiles created." << std::endl;
}

grpc::Status RaytracerServiceImpl::HealthCheck(grpc::ServerContext* context, const google::protobuf::Empty* request, HealthCheckResponse* response) {
    response->set_status(HealthCheckResponse::SERVING);
    return grpc::Status::OK;
}

grpc::Status RaytracerServiceImpl::RegisterWorker(grpc::ServerContext*,
                                                  const WorkerRegistrationRequest* request,
                                                  WorkerRegistrationResponse* response) {
    if (!request) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "missing registration request");
    }

    const int id = next_worker_id_.fetch_add(1);
    std::string worker_id = "worker-" + std::to_string(id);

    {
        std::lock_guard<std::mutex> lock(mtx_);
        registered_workers_.insert(worker_id);
    }

    response->set_worker_id(worker_id);
    response->mutable_scene()->CopyFrom(scene_data_);
    response->mutable_config()->CopyFrom(build_config_proto());
    std::cout << "Registered " << worker_id << " (" << request->hostname() << ")" << std::endl;

    return grpc::Status::OK;
}

grpc::Status RaytracerServiceImpl::RequestTask(grpc::ServerContext*,
                                               const WorkRequest* request,
                                               TaskAssignment* response) {
    if (!request || !response) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "missing request");
    }

    if (request->worker_id().empty()) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "missing worker id");
    }

    std::lock_guard<std::mutex> lock(mtx_);
    if (!validate_worker(request->worker_id())) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "worker not registered");
    }
    reclaim_expired_tasks_locked();

    if (work_queue_.empty()) {
        response->set_has_assignment(false);
        return grpc::Status::OK;
    }

    RenderTask task = work_queue_.front();
    work_queue_.pop();
    const int32_t task_id = task.tile().task_id();
    in_progress_[task_id] = AssignedTask{
        task,
        request->worker_id(),
        std::chrono::steady_clock::now()
    };

    response->set_has_assignment(true);
    response->mutable_task()->CopyFrom(task);
    return grpc::Status::OK;
}

grpc::Status RaytracerServiceImpl::SubmitResult(grpc::ServerContext*,
                                                const SubmitResultRequest* request,
                                                google::protobuf::Empty*) {
    if (!request) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "missing submit request");
    }

    const auto& worker_id = request->worker_id();
    if (worker_id.empty()) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "worker not registered");
    }

    const auto& tile = request->result().tile();
    const int32_t task_id = tile.task_id();

    std::unique_lock<std::mutex> lock(mtx_);
    if (!validate_worker(worker_id)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "worker not registered");
    }
    auto it = in_progress_.find(task_id);
    if (it == in_progress_.end()) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "task not leased or already completed");
    }

    if (it->second.worker_id != worker_id) {
        return grpc::Status(grpc::StatusCode::PERMISSION_DENIED, "task owned by another worker");
    }

    const std::string& pixels = request->result().pixel_data();
    const size_t expected_pixels =
        static_cast<size_t>(tile.width()) * static_cast<size_t>(tile.height()) * 3;
    if (pixels.size() != expected_pixels) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "pixel data size mismatch");
    }

    size_t i = 0;
    for (int y = 0; y < tile.height(); ++y) {
        for (int x = 0; x < tile.width(); ++x) {
            size_t index = static_cast<size_t>(tile.y0() + y) * static_cast<size_t>(image_width_) +
                           static_cast<size_t>(tile.x0() + x);
            final_image_pixels_[index] = color(
                static_cast<unsigned char>(pixels[i]) / 255.999,
                static_cast<unsigned char>(pixels[i + 1]) / 255.999,
                static_cast<unsigned char>(pixels[i + 2]) / 255.999
            );
            i += 3;
        }
    }

    in_progress_.erase(it);

    int completed_count = ++tiles_completed_;
    std::cout << "Progress: " << completed_count << " / " << total_tiles_ << " tiles completed." << std::endl;
    
    if (completed_count == total_tiles_) {
        all_done_cv_.notify_one();
    }
    return grpc::Status::OK;
}

void RaytracerServiceImpl::wait_for_completion() {
    std::unique_lock<std::mutex> lock(mtx_);
    all_done_cv_.wait(lock, [this]{ return tiles_completed_.load() == total_tiles_; });
    std::cout << "All tiles rendered. Saving image to " << output_path_ << std::endl;
    save_image();
}

void RaytracerServiceImpl::save_image() {
    std::ofstream out_file(output_path_);
    if (!out_file) {
        std::cerr << "Error: Could not open output file " << output_path_ << std::endl;
        return;
    }

    out_file << "P3\n" << image_width_ << ' ' << image_height_ << "\n255\n";
    for (const auto& pixel : final_image_pixels_) {
        write_color(out_file, pixel);
    }
}

std::queue<RenderTask> RaytracerServiceImpl::create_work_queue(int image_width, int image_height, int tile_size, int samples, int depth) {
    std::queue<RenderTask> queue;
    int32_t task_id = 0;
    for (int y = 0; y < image_height; y += tile_size) {
        for (int x = 0; x < image_width; x += tile_size) {
            RenderTask task;
            auto* tile = task.mutable_tile();
            tile->set_x0(x);
            tile->set_y0(y);
            tile->set_width(std::min(tile_size, image_width - x));
            tile->set_height(std::min(tile_size, image_height - y));
            tile->set_task_id(task_id++);
            task.set_samples_per_pixel(samples);
            task.set_max_depth(depth);
            queue.push(task);
        }
    }
    return queue;
}

RenderConfig RaytracerServiceImpl::build_config_proto() const {
    RenderConfig config;
    config.set_image_width(settings_.image_width);
    config.set_image_height(settings_.image_height);
    config.set_samples_per_pixel(settings_.samples_per_pixel);
    config.set_max_depth(settings_.max_depth);
    config.set_tile_size(settings_.tile_size);
    return config;
}

void RaytracerServiceImpl::reclaim_expired_tasks_locked() {
    const auto now = std::chrono::steady_clock::now();
    std::vector<int32_t> expired;
    for (const auto& [task_id, assigned] : in_progress_) {
        if (now - assigned.leased_at > lease_timeout_) {
            expired.push_back(task_id);
        }
    }

    for (int32_t task_id : expired) {
        auto it = in_progress_.find(task_id);
        if (it == in_progress_.end()) continue;
        work_queue_.push(it->second.task);
        in_progress_.erase(it);
        std::cout << "Reclaimed timed-out task " << task_id << std::endl;
    }
}

bool RaytracerServiceImpl::validate_worker(const std::string& worker_id) const {
    return registered_workers_.contains(worker_id);
}

void RunServer(const scene& sc, int image_width, int image_height, int tile_size, int samples, int depth, const std::string& address, const std::string& output_path) {
    RaytracerServiceImpl service(sc, image_width, image_height, tile_size, samples, depth, output_path);

    ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    
    std::unique_ptr<Server> server(builder.BuildAndStart());
    if (!server) {
        std::cerr << "Failed to start server on " << address << std::endl;
        return;
    }
    std::cout << "Master server listening on " << address << std::endl;

    service.wait_for_completion();
    server->Shutdown();
}
