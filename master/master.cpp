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
      work_queue_(create_work_queue(scene_data_, image_width, image_height, tile_size, samples, depth)),
      total_tiles_(work_queue_.size()), 
      tiles_completed_(0), 
      image_width_(image_width),
      output_path_(output_path) {
    
    final_image_pixels_.resize(image_width * image_height);
    std::cout << "Master: " << total_tiles_ << " tiles created." << std::endl;
}

grpc::Status RaytracerServiceImpl::HealthCheck(grpc::ServerContext* context, const google::protobuf::Empty* request, HealthCheckResponse* response) {
    response->set_status(HealthCheckResponse::SERVING);
    return grpc::Status::OK;
}

grpc::Status RaytracerServiceImpl::RequestWork(grpc::ServerContext* context, const google::protobuf::Empty* request, 
                   grpc::ServerWriter<RenderTask>* writer) {
    std::cout << "Worker connected and requesting work stream." << std::endl;
    RenderTask task;
    while (get_next_task(task)) {
        if (!writer->Write(task)) {
            break;
        }
    }
    std::cout << "No more work to distribute. Closing stream for this worker." << std::endl;
    return grpc::Status::OK;
}

grpc::Status RaytracerServiceImpl::SubmitResult(grpc::ServerContext* context, const TileResult* result, 
                    google::protobuf::Empty* response) {
    std::unique_lock<std::mutex> lock(mtx_);
    const auto& tile = result->tile();
    
    int i = 0;
    for (int y = 0; y < tile.height(); ++y) {
        for (int x = 0; x < tile.width(); ++x) {
            int index = (tile.y0() + y) * image_width_ + (tile.x0() + x);
            final_image_pixels_[index] = color(
                static_cast<unsigned char>(result->pixel_data()[i]) / 255.999,
                static_cast<unsigned char>(result->pixel_data()[i+1]) / 255.999,
                static_cast<unsigned char>(result->pixel_data()[i+2]) / 255.999
            );
            i += 3;
        }
    }

    int completed_count = ++tiles_completed_;
    std::cout << "Progress: " << completed_count << " / " << total_tiles_ << " tiles completed." << std::endl;
    
    if (completed_count == total_tiles_) {
        all_done_cv_.notify_one();
    }
    return grpc::Status::OK;
}

void RaytracerServiceImpl::wait_for_completion() {
    std::unique_lock<std::mutex> lock(mtx_);
    all_done_cv_.wait(lock, [this]{ return tiles_completed_ == total_tiles_; });
    std::cout << "All tiles rendered. Saving image to " << output_path_ << std::endl;
    save_image();
}

bool RaytracerServiceImpl::get_next_task(RenderTask& task) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (work_queue_.empty()) {
        return false;
    }
    task = work_queue_.front();
    work_queue_.pop();
    return true;
}

void RaytracerServiceImpl::save_image() {
    std::ofstream out_file(output_path_);
    if (!out_file) {
        std::cerr << "Error: Could not open output file " << output_path_ << std::endl;
        return;
    }

    const int height = final_image_pixels_.size() / image_width_;
    out_file << "P3\n" << image_width_ << ' ' << height << "\n255\n";
    for (const auto& pixel : final_image_pixels_) {
        write_color(out_file, pixel);
    }
}

std::queue<RenderTask> RaytracerServiceImpl::create_work_queue(const SceneData& scene_data, int image_width, int image_height, int tile_size, int samples, int depth) {
    std::queue<RenderTask> queue;
    for (int y = 0; y < image_height; y += tile_size) {
        for (int x = 0; x < image_width; x += tile_size) {
            RenderTask task;
            task.mutable_scene()->CopyFrom(scene_data);
            task.mutable_tile()->set_x0(x);
            task.mutable_tile()->set_y0(y);
            task.mutable_tile()->set_width(std::min(tile_size, image_width - x));
            task.mutable_tile()->set_height(std::min(tile_size, image_height - y));
            task.set_samples_per_pixel(samples);
            task.set_max_depth(depth);
            queue.push(task);
        }
    }
    return queue;
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
