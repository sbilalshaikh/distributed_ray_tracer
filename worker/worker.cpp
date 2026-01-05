#include "worker.hpp"
#include <iostream>
#include <vector>

#include "camera.hpp"
#include "renderer.hpp"
#include "hittable.hpp"
#include "serialization.hpp"

using grpc::ClientContext;
using grpc::Status;

RaytracerWorker::RaytracerWorker(std::shared_ptr<grpc::Channel> channel)
    : stub_(RaytracerService::NewStub(channel)) {}

void RaytracerWorker::run() {
    if (!health_check()) {
        return;
    }

    std::cout << "Master is healthy. Requesting work stream..." << std::endl;
    
    ClientContext context;
    google::protobuf::Empty request;
    
    std::unique_ptr<grpc::ClientReader<RenderTask>> reader(
        stub_->RequestWork(&context, request));
        
    RenderTask task;
    while (reader->Read(&task)) {
        std::cout << "Received task for tile (" << task.tile().x0() << ", " << task.tile().y0() << ")" << std::endl;

        std::shared_ptr<hittable> world = deserialize_scene(task.scene());
        const auto& proto_cam = task.scene().camera();
        
        const int image_width = 1200; // These should eventually come from master
        const int image_height = 800;
        const double aspect_ratio = static_cast<double>(image_width) / image_height;

        camera cam(
            proto_to_vec3(proto_cam.position()),
            proto_to_vec3(proto_cam.look_at()),
            proto_to_vec3(proto_cam.up()),
            proto_cam.vfov(),
            aspect_ratio,
            image_width, 
            image_height
        );

        renderer rend(cam, *world);
        std::vector<color> pixels = rend.render_tile(
            task.tile().x0(),
            task.tile().y0(),
            task.tile().width(),
            task.tile().height(),
            task.samples_per_pixel(),
            task.max_depth(),
            0 // seed
        );
        
        submit_result(task.tile(), pixels);
    }
    
    Status status = reader->Finish();
    if (status.ok()) {
        std::cout << "Work stream finished gracefully. Exiting." << std::endl;
    } else {
        std::cerr << "Work stream failed: " << status.error_message() << std::endl;
    }
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

void RaytracerWorker::submit_result(const Tile& tile, const std::vector<color>& pixels) {
    ClientContext context;
    google::protobuf::Empty response;
    TileResult result;

    result.mutable_tile()->CopyFrom(tile);
    
    std::vector<uint8_t> byte_buffer;
    byte_buffer.reserve(pixels.size() * 3);
    for(const auto& p : pixels) {
        byte_buffer.push_back(static_cast<uint8_t>(255.999 * p.x()));
        byte_buffer.push_back(static_cast<uint8_t>(255.999 * p.y()));
        byte_buffer.push_back(static_cast<uint8_t>(255.999 * p.z()));
    }
    result.set_pixel_data(byte_buffer.data(), byte_buffer.size());

    stub_->SubmitResult(&context, result, &response);
}
