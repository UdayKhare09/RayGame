#pragma once
#include "Types.h"
#include "Camera.h"
#include "Scene.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vector>
#include <string>
#include <VkBootstrap.h>
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_vulkan.h"

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool init(SDL_Window* window);
    void cleanup();
    int draw(Camera& camera, float time, Scene& scene);
    void resize();

private:
    struct Init {
        SDL_Window* window;
        vkb::Instance instance;
        vkb::InstanceDispatchTable inst_disp;
        VkSurfaceKHR surface;
        vkb::Device device;
        vkb::DispatchTable disp;
        vkb::Swapchain swapchain;
    } init_data;

    struct RenderData {
        VkQueue graphics_queue;
        VkQueue present_queue;

        std::vector<VkImage> swapchain_images;
        std::vector<VkImageView> swapchain_image_views;
        std::vector<VkFramebuffer> framebuffers;

        VkRenderPass render_pass;
        VkDescriptorSetLayout descriptor_set_layout;
        VkPipelineLayout pipeline_layout;
        VkPipeline graphics_pipeline;

        VkCommandPool command_pool;
        std::vector<VkCommandBuffer> command_buffers;

        std::vector<VkSemaphore> available_semaphores;
        std::vector<VkSemaphore> finished_semaphore;
        std::vector<VkFence> in_flight_fences;
        size_t current_frame = 0;

        std::vector<VkBuffer> uniform_buffers;
        std::vector<VkDeviceMemory> uniform_buffers_memory;
        std::vector<void*> uniform_buffers_mapped;

        std::vector<VkBuffer> scene_buffers;
        std::vector<VkDeviceMemory> scene_buffers_memory;
        std::vector<void*> scene_buffers_mapped;

        VkDescriptorPool descriptor_pool;
        VkDescriptorPool imgui_descriptor_pool;
        std::vector<VkDescriptorSet> descriptor_sets;
    } render_data;

    int device_initialization();
    int create_swapchain();
    int get_queues();
    int create_render_pass();
    int create_descriptor_set_layout();
    int create_graphics_pipeline();
    int create_framebuffers();
    int create_command_pool();
    int create_uniform_buffers();
    int create_scene_buffers();
    int create_descriptor_pool();
    int create_descriptor_sets();
    int create_command_buffers();
    int create_sync_objects();
    int recreate_swapchain();
    
    // ImGui
    int init_imgui();
    
    int record_command_buffer(uint32_t imageIndex, const Camera& camera, float time, const Scene& scene);
    void update_uniform_buffer(const Camera& camera, float time, const Scene& scene);
    void update_scene_buffer(const Scene& scene);
    
    std::vector<char> readFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
};
