#include "Renderer.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <imgui.h>

const int MAX_FRAMES_IN_FLIGHT = 2;

Renderer::Renderer() {}

Renderer::~Renderer() {
    cleanup();
}

bool Renderer::init(SDL_Window* window) {
    std::cout << "Initializing Renderer..." << std::endl;
    init_data.window = window;
    if (device_initialization() != 0) { std::cerr << "Device init failed" << std::endl; return false; }
    std::cout << "Device initialized." << std::endl;
    if (create_swapchain() != 0) { std::cerr << "Swapchain creation failed" << std::endl; return false; }
    std::cout << "Swapchain created." << std::endl;
    if (get_queues() != 0) { std::cerr << "Get queues failed" << std::endl; return false; }
    if (create_render_pass() != 0) { std::cerr << "Render pass creation failed" << std::endl; return false; }
    if (create_descriptor_set_layout() != 0) { std::cerr << "Descriptor set layout creation failed" << std::endl; return false; }
    if (create_graphics_pipeline() != 0) { std::cerr << "Graphics pipeline creation failed" << std::endl; return false; }
    std::cout << "Graphics pipeline created." << std::endl;
    if (create_framebuffers() != 0) { std::cerr << "Framebuffer creation failed" << std::endl; return false; }
    if (create_command_pool() != 0) { std::cerr << "Command pool creation failed" << std::endl; return false; }
    if (create_uniform_buffers() != 0) { std::cerr << "Uniform buffer creation failed" << std::endl; return false; }
    if (create_scene_buffers() != 0) { std::cerr << "Scene buffer creation failed" << std::endl; return false; }
    if (create_descriptor_pool() != 0) { std::cerr << "Descriptor pool creation failed" << std::endl; return false; }
    if (create_descriptor_sets() != 0) { std::cerr << "Descriptor sets creation failed" << std::endl; return false; }
    if (create_command_buffers() != 0) { std::cerr << "Command buffers creation failed" << std::endl; return false; }
    if (create_sync_objects() != 0) { std::cerr << "Sync objects creation failed" << std::endl; return false; }
    if (init_imgui() != 0) { std::cerr << "ImGui init failed" << std::endl; return false; }
    std::cout << "Renderer initialized successfully." << std::endl;
    return true;
}

int Renderer::init_imgui() {
    // 1: Create descriptor pool for ImGui
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    if (init_data.disp.createDescriptorPool(&pool_info, nullptr, &render_data.imgui_descriptor_pool) != VK_SUCCESS) {
        return -1;
    }

    // 2: Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    if (!ImGui_ImplSDL2_InitForVulkan(init_data.window)) return -1;
    
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = init_data.instance;
    init_info.PhysicalDevice = init_data.device.physical_device;
    init_info.Device = init_data.device;
    init_info.QueueFamily = init_data.device.get_queue_index(vkb::QueueType::graphics).value();
    init_info.Queue = render_data.graphics_queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = render_data.imgui_descriptor_pool;
    init_info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
    init_info.ImageCount = init_data.swapchain.image_count;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = nullptr;
    
    // New API for RenderPass
    init_info.PipelineInfoMain.RenderPass = render_data.render_pass;
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    if (!ImGui_ImplVulkan_Init(&init_info)) return -1;

    return 0;
}

int Renderer::device_initialization() {
    vkb::InstanceBuilder instance_builder;
    auto instance_ret = instance_builder.use_default_debug_messenger().request_validation_layers().build();
    if (!instance_ret) return -1;
    init_data.instance = instance_ret.value();
    init_data.inst_disp = init_data.instance.make_table();

    if (!SDL_Vulkan_CreateSurface(init_data.window, init_data.instance, &init_data.surface)) return -1;

    vkb::PhysicalDeviceSelector phys_device_selector(init_data.instance);
    auto phys_device_ret = phys_device_selector.set_surface(init_data.surface).select();
    if (!phys_device_ret) return -1;
    vkb::PhysicalDevice physical_device = phys_device_ret.value();

    vkb::DeviceBuilder device_builder{physical_device};
    auto device_ret = device_builder.build();
    if (!device_ret) return -1;
    init_data.device = device_ret.value();
    init_data.disp = init_data.device.make_table();

    return 0;
}

int Renderer::create_swapchain() {
    int w, h;
    SDL_Vulkan_GetDrawableSize(init_data.window, &w, &h);

    vkb::SwapchainBuilder swapchain_builder{init_data.device};
    auto swap_ret = swapchain_builder.set_old_swapchain(init_data.swapchain)
                        .set_desired_extent(w, h)
                        .build();
    if (!swap_ret) return -1;
    vkb::destroy_swapchain(init_data.swapchain);
    init_data.swapchain = swap_ret.value();
    return 0;
}

int Renderer::get_queues() {
    auto gq = init_data.device.get_queue(vkb::QueueType::graphics);
    if (!gq.has_value()) return -1;
    render_data.graphics_queue = gq.value();

    auto pq = init_data.device.get_queue(vkb::QueueType::present);
    if (!pq.has_value()) return -1;
    render_data.present_queue = pq.value();
    return 0;
}

int Renderer::create_render_pass() {
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = init_data.swapchain.image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (init_data.disp.createRenderPass(&render_pass_info, nullptr, &render_data.render_pass) != VK_SUCCESS) return -1;
    return 0;
}

std::vector<char> Renderer::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("failed to open file: " + filename);
    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(file_size));
    file.close();
    return buffer;
}

VkShaderModule Renderer::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule;
    if (init_data.disp.createShaderModule(&create_info, nullptr, &shaderModule) != VK_SUCCESS) return VK_NULL_HANDLE;
    return shaderModule;
}

int Renderer::create_descriptor_set_layout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding sceneLayoutBinding{};
    sceneLayoutBinding.binding = 1;
    sceneLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    sceneLayoutBinding.descriptorCount = 1;
    sceneLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding bindings[] = {uboLayoutBinding, sceneLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;

    if (init_data.disp.createDescriptorSetLayout(&layoutInfo, nullptr, &render_data.descriptor_set_layout) != VK_SUCCESS) return -1;
    return 0;
}

int Renderer::create_graphics_pipeline() {
    auto vert_code = readFile("shaders/raytracer.vert.spv");
    auto frag_code = readFile("shaders/raytracer.frag.spv");

    VkShaderModule vert_module = createShaderModule(vert_code);
    VkShaderModule frag_module = createShaderModule(frag_code);
    if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE) return -1;

    VkPipelineShaderStageCreateInfo vert_stage_info = {};
    vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage_info.module = vert_module;
    vert_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_stage_info = {};
    frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage_info.module = frag_module;
    frag_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_stage_info, frag_stage_info};

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)init_data.swapchain.extent.width;
    viewport.height = (float)init_data.swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = init_data.swapchain.extent;

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &render_data.descriptor_set_layout;

    if (init_data.disp.createPipelineLayout(&pipeline_layout_info, nullptr, &render_data.pipeline_layout) != VK_SUCCESS) return -1;

    std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_info = {};
    dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_info.pDynamicStates = dynamic_states.data();

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_info;
    pipeline_info.layout = render_data.pipeline_layout;
    pipeline_info.renderPass = render_data.render_pass;
    pipeline_info.subpass = 0;

    if (init_data.disp.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &render_data.graphics_pipeline) != VK_SUCCESS) return -1;

    init_data.disp.destroyShaderModule(frag_module, nullptr);
    init_data.disp.destroyShaderModule(vert_module, nullptr);
    return 0;
}

int Renderer::create_framebuffers() {
    render_data.swapchain_images = init_data.swapchain.get_images().value();
    render_data.swapchain_image_views = init_data.swapchain.get_image_views().value();

    render_data.framebuffers.resize(render_data.swapchain_image_views.size());

    for (size_t i = 0; i < render_data.swapchain_image_views.size(); i++) {
        VkImageView attachments[] = {render_data.swapchain_image_views[i]};

        VkFramebufferCreateInfo framebuffer_info = {};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_data.render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = init_data.swapchain.extent.width;
        framebuffer_info.height = init_data.swapchain.extent.height;
        framebuffer_info.layers = 1;

        if (init_data.disp.createFramebuffer(&framebuffer_info, nullptr, &render_data.framebuffers[i]) != VK_SUCCESS) return -1;
    }
    return 0;
}

int Renderer::create_command_pool() {
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = init_data.device.get_queue_index(vkb::QueueType::graphics).value();
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (init_data.disp.createCommandPool(&pool_info, nullptr, &render_data.command_pool) != VK_SUCCESS) return -1;
    return 0;
}

void Renderer::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    init_data.disp.createBuffer(&bufferInfo, nullptr, &buffer);

    VkMemoryRequirements memRequirements;
    init_data.disp.getBufferMemoryRequirements(buffer, &memRequirements);

    VkPhysicalDeviceMemoryProperties memProperties;
    init_data.inst_disp.getPhysicalDeviceMemoryProperties(init_data.device.physical_device, &memProperties);

    uint32_t typeIndex = -1;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            typeIndex = i;
            break;
        }
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = typeIndex;

    init_data.disp.allocateMemory(&allocInfo, nullptr, &bufferMemory);
    init_data.disp.bindBufferMemory(buffer, bufferMemory, 0);
}

int Renderer::create_uniform_buffers() {
    VkDeviceSize bufferSize = sizeof(Uniforms);
    render_data.uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    render_data.uniform_buffers_memory.resize(MAX_FRAMES_IN_FLIGHT);
    render_data.uniform_buffers_mapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        create_buffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, render_data.uniform_buffers[i], render_data.uniform_buffers_memory[i]);
        init_data.disp.mapMemory(render_data.uniform_buffers_memory[i], 0, bufferSize, 0, &render_data.uniform_buffers_mapped[i]);
    }
    return 0;
}

struct SphereGPU {
    float center[3];
    float radius;
    float color[3];
    float roughness;
};

struct PointLightGPU {
    float position[3];
    float intensity;
    float color[3];
    float padding;
};

struct SpotLightGPU {
    float position[3];
    float intensity;
    float direction[3];
    float cutOff;
    float color[3];
    float outerCutOff;
};

struct SceneGPU {
    SphereGPU spheres[100];
    PointLightGPU pointLight;
    SpotLightGPU spotLight;
    int sphereCount;
    float sunDirection[3];
};

int Renderer::create_scene_buffers() {
    VkDeviceSize bufferSize = sizeof(SceneGPU);
    render_data.scene_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    render_data.scene_buffers_memory.resize(MAX_FRAMES_IN_FLIGHT);
    render_data.scene_buffers_mapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        create_buffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, render_data.scene_buffers[i], render_data.scene_buffers_memory[i]);
        init_data.disp.mapMemory(render_data.scene_buffers_memory[i], 0, bufferSize, 0, &render_data.scene_buffers_mapped[i]);
    }
    return 0;
}

int Renderer::create_descriptor_pool() {
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)}
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (init_data.disp.createDescriptorPool(&poolInfo, nullptr, &render_data.descriptor_pool) != VK_SUCCESS) return -1;
    return 0;
}

int Renderer::create_descriptor_sets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, render_data.descriptor_set_layout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = render_data.descriptor_pool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    render_data.descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
    if (init_data.disp.allocateDescriptorSets(&allocInfo, render_data.descriptor_sets.data()) != VK_SUCCESS) return -1;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = render_data.uniform_buffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(Uniforms);

        VkDescriptorBufferInfo sceneBufferInfo{};
        sceneBufferInfo.buffer = render_data.scene_buffers[i];
        sceneBufferInfo.offset = 0;
        sceneBufferInfo.range = sizeof(SceneGPU);

        VkWriteDescriptorSet descriptorWrites[2]{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = render_data.descriptor_sets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = render_data.descriptor_sets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &sceneBufferInfo;

        init_data.disp.updateDescriptorSets(2, descriptorWrites, 0, nullptr);
    }
    return 0;
}

int Renderer::create_command_buffers() {
    render_data.command_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = render_data.command_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)render_data.command_buffers.size();

    if (init_data.disp.allocateCommandBuffers(&allocInfo, render_data.command_buffers.data()) != VK_SUCCESS) return -1;
    return 0;
}

int Renderer::create_sync_objects() {
    render_data.available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render_data.finished_semaphore.resize(init_data.swapchain.image_count);
    render_data.in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (init_data.disp.createSemaphore(&semaphore_info, nullptr, &render_data.available_semaphores[i]) != VK_SUCCESS ||
            init_data.disp.createFence(&fence_info, nullptr, &render_data.in_flight_fences[i]) != VK_SUCCESS) {
            return -1;
        }
    }
    for (size_t i = 0; i < init_data.swapchain.image_count; i++) {
        if (init_data.disp.createSemaphore(&semaphore_info, nullptr, &render_data.finished_semaphore[i]) != VK_SUCCESS) return -1;
    }
    return 0;
}

int Renderer::recreate_swapchain() {
    init_data.disp.deviceWaitIdle();

    for (auto framebuffer : render_data.framebuffers) {
        init_data.disp.destroyFramebuffer(framebuffer, nullptr);
    }
    init_data.swapchain.destroy_image_views(render_data.swapchain_image_views);
    for (auto semaphore : render_data.finished_semaphore) {
        init_data.disp.destroySemaphore(semaphore, nullptr);
    }
    render_data.finished_semaphore.clear();

    if (create_swapchain() != 0) return -1;
    if (create_framebuffers() != 0) return -1;

    render_data.finished_semaphore.resize(init_data.swapchain.image_count);
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (size_t i = 0; i < init_data.swapchain.image_count; i++) {
        if (init_data.disp.createSemaphore(&semaphore_info, nullptr, &render_data.finished_semaphore[i]) != VK_SUCCESS) return -1;
    }
    return 0;
}

void Renderer::update_uniform_buffer(const Camera& camera, float time, const Scene& scene) {
    Uniforms ubo{};
    ubo.resolution[0] = (float)init_data.swapchain.extent.width;
    ubo.resolution[1] = (float)init_data.swapchain.extent.height;
    ubo.time = time;
    ubo.sunEnabled = scene.sunEnabled ? 1.0f : 0.0f;

    ubo.cameraPos[0] = camera.position.x;
    ubo.cameraPos[1] = camera.position.y;
    ubo.cameraPos[2] = camera.position.z;

    Vec3 fwd = camera.getForward();
    ubo.cameraDir[0] = fwd.x;
    ubo.cameraDir[1] = fwd.y;
    ubo.cameraDir[2] = fwd.z;

    memcpy(render_data.uniform_buffers_mapped[render_data.current_frame], &ubo, sizeof(ubo));
}

void Renderer::update_scene_buffer(const Scene& scene) {
    SceneGPU gpuScene{};
    gpuScene.sphereCount = std::min((int)scene.spheres.size(), 100);
    for (int i = 0; i < gpuScene.sphereCount; i++) {
        gpuScene.spheres[i].center[0] = scene.spheres[i].center.x;
        gpuScene.spheres[i].center[1] = scene.spheres[i].center.y;
        gpuScene.spheres[i].center[2] = scene.spheres[i].center.z;
        gpuScene.spheres[i].radius = scene.spheres[i].radius;
        gpuScene.spheres[i].color[0] = scene.spheres[i].color.x;
        gpuScene.spheres[i].color[1] = scene.spheres[i].color.y;
        gpuScene.spheres[i].color[2] = scene.spheres[i].color.z;
        gpuScene.spheres[i].roughness = scene.spheres[i].roughness;
    }

    // Point Light
    gpuScene.pointLight.position[0] = scene.pointLight.position.x;
    gpuScene.pointLight.position[1] = scene.pointLight.position.y;
    gpuScene.pointLight.position[2] = scene.pointLight.position.z;
    gpuScene.pointLight.intensity = scene.pointLight.intensity;
    gpuScene.pointLight.color[0] = scene.pointLight.color.x;
    gpuScene.pointLight.color[1] = scene.pointLight.color.y;
    gpuScene.pointLight.color[2] = scene.pointLight.color.z;

    // Spot Light
    gpuScene.spotLight.position[0] = scene.spotLight.position.x;
    gpuScene.spotLight.position[1] = scene.spotLight.position.y;
    gpuScene.spotLight.position[2] = scene.spotLight.position.z;
    gpuScene.spotLight.intensity = scene.spotLight.intensity;
    gpuScene.spotLight.direction[0] = scene.spotLight.direction.x;
    gpuScene.spotLight.direction[1] = scene.spotLight.direction.y;
    gpuScene.spotLight.direction[2] = scene.spotLight.direction.z;
    gpuScene.spotLight.cutOff = scene.spotLight.cutOff;
    gpuScene.spotLight.color[0] = scene.spotLight.color.x;
    gpuScene.spotLight.color[1] = scene.spotLight.color.y;
    gpuScene.spotLight.color[2] = scene.spotLight.color.z;
    gpuScene.spotLight.outerCutOff = scene.spotLight.outerCutOff;

    // Sun Direction
    gpuScene.sunDirection[0] = scene.sunDirection.x;
    gpuScene.sunDirection[1] = scene.sunDirection.y;
    gpuScene.sunDirection[2] = scene.sunDirection.z;

    memcpy(render_data.scene_buffers_mapped[render_data.current_frame], &gpuScene, sizeof(gpuScene));
}

int Renderer::record_command_buffer(uint32_t imageIndex, const Camera& camera, float time, const Scene& scene) {
    VkCommandBuffer commandBuffer = render_data.command_buffers[render_data.current_frame];

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (init_data.disp.beginCommandBuffer(commandBuffer, &begin_info) != VK_SUCCESS) return -1;

    VkRenderPassBeginInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_data.render_pass;
    render_pass_info.framebuffer = render_data.framebuffers[imageIndex];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = init_data.swapchain.extent;
    VkClearValue clearColor{{1.0f, 0.0f, 1.0f, 1.0f}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clearColor;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)init_data.swapchain.extent.width;
    viewport.height = (float)init_data.swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = init_data.swapchain.extent;

    init_data.disp.cmdSetViewport(commandBuffer, 0, 1, &viewport);
    init_data.disp.cmdSetScissor(commandBuffer, 0, 1, &scissor);

    init_data.disp.cmdBeginRenderPass(commandBuffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    init_data.disp.cmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render_data.graphics_pipeline);
    init_data.disp.cmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render_data.pipeline_layout, 0, 1, &render_data.descriptor_sets[render_data.current_frame], 0, nullptr);
    init_data.disp.cmdDraw(commandBuffer, 3, 1, 0, 0);

    // Draw ImGui
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    init_data.disp.cmdEndRenderPass(commandBuffer);

    if (init_data.disp.endCommandBuffer(commandBuffer) != VK_SUCCESS) return -1;
    return 0;
}

int Renderer::draw(Camera& camera, float time, Scene& scene) {

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    // UI
    {
        ImGui::Begin("Settings");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        
        ImGui::Checkbox("Sun Enabled", &scene.sunEnabled);
        ImGui::DragFloat3("Sun Direction", &scene.sunDirection.x, 0.01f, -1.0f, 1.0f);

        if (ImGui::CollapsingHeader("Point Light")) {
            ImGui::DragFloat3("PL Position", &scene.pointLight.position.x, 0.1f);
            ImGui::ColorEdit3("PL Color", &scene.pointLight.color.x);
            ImGui::DragFloat("PL Intensity", &scene.pointLight.intensity, 0.1f, 0.0f, 100.0f);
        }

        if (ImGui::CollapsingHeader("Spot Light")) {
            ImGui::DragFloat3("SL Position", &scene.spotLight.position.x, 0.1f);
            ImGui::DragFloat3("SL Direction", &scene.spotLight.direction.x, 0.01f, -1.0f, 1.0f);
            ImGui::ColorEdit3("SL Color", &scene.spotLight.color.x);
            ImGui::DragFloat("SL Intensity", &scene.spotLight.intensity, 0.1f, 0.0f, 100.0f);
            ImGui::DragFloat("SL CutOff", &scene.spotLight.cutOff, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("SL OuterCutOff", &scene.spotLight.outerCutOff, 0.01f, 0.0f, 1.0f);
        }
        
        ImGui::Separator();
        ImGui::Text("Camera");
        ImGui::DragFloat3("Position", &camera.position.x, 0.1f);
        ImGui::DragFloat("Yaw", &camera.yaw, 0.01f);
        ImGui::DragFloat("Pitch", &camera.pitch, 0.01f);

        ImGui::Separator();
        ImGui::Text("Scene");
        if (ImGui::Button("Add Sphere")) {
            scene.spheres.push_back({{0, 5, 0}, 1.0f, {1, 1, 1}});
        }
        
        for (int i = 0; i < scene.spheres.size(); i++) {
            ImGui::PushID(i);
            if (ImGui::TreeNode("Sphere")) {
                ImGui::DragFloat3("Center", &scene.spheres[i].center.x, 0.1f);
                ImGui::DragFloat("Radius", &scene.spheres[i].radius, 0.1f);
                ImGui::ColorEdit3("Color", &scene.spheres[i].color.x);
                ImGui::SliderFloat("Roughness", &scene.spheres[i].roughness, 0.0f, 1.0f);
                if (ImGui::Button("Remove")) {
                    scene.spheres.erase(scene.spheres.begin() + i);
                    ImGui::TreePop();
                    ImGui::PopID();
                    continue;
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }

        ImGui::End();
    }

    ImGui::Render();

    init_data.disp.waitForFences(1, &render_data.in_flight_fences[render_data.current_frame], VK_TRUE, UINT64_MAX);

    uint32_t image_index = 0;
    VkResult result = init_data.disp.acquireNextImageKHR(init_data.swapchain, UINT64_MAX, render_data.available_semaphores[render_data.current_frame], VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return recreate_swapchain();
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        return -1;
    }

    init_data.disp.resetFences(1, &render_data.in_flight_fences[render_data.current_frame]);
    init_data.disp.resetCommandBuffer(render_data.command_buffers[render_data.current_frame], 0);
    
    update_uniform_buffer(camera, time, scene);
    update_scene_buffer(scene);
    record_command_buffer(image_index, camera, time, scene);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore wait_semaphores[] = {render_data.available_semaphores[render_data.current_frame]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = wait_semaphores;
    submitInfo.pWaitDstStageMask = wait_stages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &render_data.command_buffers[render_data.current_frame];
    VkSemaphore signal_semaphores[] = {render_data.finished_semaphore[image_index]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signal_semaphores;

    if (init_data.disp.queueSubmit(render_data.graphics_queue, 1, &submitInfo, render_data.in_flight_fences[render_data.current_frame]) != VK_SUCCESS) return -1;

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    VkSwapchainKHR swapChains[] = {init_data.swapchain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapChains;
    present_info.pImageIndices = &image_index;

    result = init_data.disp.queuePresentKHR(render_data.present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return recreate_swapchain();
    } else if (result != VK_SUCCESS) {
        return -1;
    }

    render_data.current_frame = (render_data.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    return 0;
}

void Renderer::resize() {
    recreate_swapchain();
}

void Renderer::cleanup() {
    init_data.disp.deviceWaitIdle();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    init_data.disp.destroyDescriptorPool(render_data.imgui_descriptor_pool, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        init_data.disp.destroySemaphore(render_data.available_semaphores[i], nullptr);
        init_data.disp.destroyFence(render_data.in_flight_fences[i], nullptr);
        init_data.disp.destroyBuffer(render_data.uniform_buffers[i], nullptr);
        init_data.disp.freeMemory(render_data.uniform_buffers_memory[i], nullptr);
        init_data.disp.destroyBuffer(render_data.scene_buffers[i], nullptr);
        init_data.disp.freeMemory(render_data.scene_buffers_memory[i], nullptr);
    }
    for (auto semaphore : render_data.finished_semaphore) {
        init_data.disp.destroySemaphore(semaphore, nullptr);
    }

    init_data.disp.destroyDescriptorPool(render_data.descriptor_pool, nullptr);
    init_data.disp.destroyDescriptorSetLayout(render_data.descriptor_set_layout, nullptr);
    init_data.disp.destroyCommandPool(render_data.command_pool, nullptr);

    for (auto framebuffer : render_data.framebuffers) {
        init_data.disp.destroyFramebuffer(framebuffer, nullptr);
    }

    init_data.disp.destroyPipeline(render_data.graphics_pipeline, nullptr);
    init_data.disp.destroyPipelineLayout(render_data.pipeline_layout, nullptr);
    init_data.disp.destroyRenderPass(render_data.render_pass, nullptr);

    init_data.swapchain.destroy_image_views(render_data.swapchain_image_views);

    vkb::destroy_swapchain(init_data.swapchain);
    vkb::destroy_device(init_data.device);
    vkb::destroy_surface(init_data.instance, init_data.surface);
    vkb::destroy_instance(init_data.instance);
}
