#define VK_USE_PLATFORM_WIN32_KHR

#include <stdio.h>
#include <vulkan/vulkan.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>
#include <windows.h>

const char *instanceExtensions[] = {
        "VK_KHR_win32_surface",
        "VK_KHR_surface"
};

const char *deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

HWND window = NULL;
VkInstance instance = NULL;
VkSurfaceKHR surface = NULL;
VkQueue presentQueue = NULL;
VkQueue graphicsQueue = NULL;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkPhysicalDevice *devices = NULL;
VkDevice device = NULL;

void createInstance() {
    VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Mordki trzy",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "ME",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_0,
    };
    VkInstanceCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = 0,
            .enabledExtensionCount = 2,
            .ppEnabledExtensionNames = instanceExtensions
    };

    if (vkCreateInstance(&createInfo, NULL, &instance)) {
        printf("Failed to create instance!");
    }
}

void pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    if (deviceCount == 0) {
        printf("No GPU");
    }
    devices = malloc(deviceCount * sizeof *devices);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
    int i = 0;
    for (i; i < deviceCount; ++i) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(devices[i], &properties);
        printf("%s\n", properties.deviceName);
    }
    physicalDevice = devices[1];
    free(devices);
}

typedef struct {
    uint32_t graphicsFamily;
    uint32_t presentFamily;
} QueueFamilyIndices;

QueueFamilyIndices findQueueFamilies() {
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties *queueFamilies = malloc(queueFamilyCount * sizeof *queueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

    int i = 0;
    for (i; i < queueFamilyCount; i++) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
    }
    free(queueFamilies);
    return indices;
}

void createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies();

    VkDeviceQueueCreateInfo queueCreateInfos[2];
    uint32_t uniqueQueueFamilies[2] = {
            indices.graphicsFamily,
            indices.presentFamily
    };

    float queuePriority = 1.0f;
    int i;
    for (i = 0; i < 2; i++) {
        VkDeviceQueueCreateInfo queueCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = uniqueQueueFamilies[i],
                .queueCount = 1,
                .pQueuePriorities = &queuePriority
        };
        queueCreateInfos[i] = queueCreateInfo;
    }

    VkPhysicalDeviceFeatures deviceFeatures = {
            VK_FALSE
    };
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
    VkDeviceCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pQueueCreateInfos = queueCreateInfos,
            .queueCreateInfoCount = 2,
            .pEnabledFeatures = &deviceFeatures,
            .enabledExtensionCount = 1,
            .ppEnabledExtensionNames = deviceExtensions,
            .enabledLayerCount = 0,
    };

    if (vkCreateDevice(physicalDevice, &createInfo, NULL, &device) != VK_SUCCESS) {
        printf("Failed to create logical device");
    }
    vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
    vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
}

void createSurface() {
    VkWin32SurfaceCreateInfoKHR createInfo = {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hwnd = window,
            .hinstance = GetModuleHandle(NULL),
    };

    if (vkCreateWin32SurfaceKHR(instance, &createInfo, NULL, &surface) != VK_SUCCESS) {
        printf("Failed to create window surface");
    }
}


//Swap chain
typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    uint32_t formatsCount;
    VkSurfaceFormatKHR *formats;
    uint32_t presentModesCount;
    VkPresentModeKHR *presentModes;
} SwapChainSupportDetails;

VkSwapchainKHR swapChain;
uint32_t swapChainImagesCount;
VkImage *swapChainImages;
VkImageView *swapChainImageViews;
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;

SwapChainSupportDetails querySwapChainSupport() {
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &details.formatsCount, NULL);
    details.formats = malloc(details.formatsCount * sizeof *details.formats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &details.formatsCount, details.formats);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &details.presentModesCount, NULL);
    details.presentModes = malloc(details.presentModesCount * sizeof *details.presentModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &details.presentModesCount,
                                              details.presentModes);

    return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(SwapChainSupportDetails details) {
    int i = 0;
    for (i; i < details.formatsCount; i++) {
        if (details.formats[i].format == VK_FORMAT_B8G8R8A8_SRGB
            && details.formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return details.formats[i];
        }
    }
    return details.formats[0];
}

VkPresentModeKHR chooseSwapPresentMode(SwapChainSupportDetails details) {
    int i = 0;
    for (i; i < details.presentModesCount; i++) {
        if (details.presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return details.presentModes[i];
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

uint32_t clamp(uint32_t value, uint32_t min, uint32_t max) {
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

VkExtent2D chooseSwapExtent(SwapChainSupportDetails details) {
    if (details.capabilities.currentExtent.width != ~(uint32_t) 0) {
        return details.capabilities.currentExtent;
    } else {
        uint32_t width, height;
        RECT rect;
        GetClientRect(window, &rect);
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
        VkExtent2D extent = {
                clamp(width, details.capabilities.minImageExtent.width, details.capabilities.maxImageExtent.width),
                clamp(height, details.capabilities.minImageExtent.height, details.capabilities.maxImageExtent.height)
        };
        return extent;
    }
}

void createSwapChain() {
    SwapChainSupportDetails details = querySwapChainSupport();
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(details);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(details);
    VkExtent2D extent = chooseSwapExtent(details);
    uint32_t imageCount = details.capabilities.minImageCount;

    VkSwapchainCreateInfoKHR createInfo = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = surface,
            .minImageCount = imageCount,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .preTransform = details.capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = presentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE
    };

    QueueFamilyIndices indices = findQueueFamilies();
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = NULL; // Optional
    }
    if (vkCreateSwapchainKHR(device, &createInfo, NULL, &swapChain) != VK_SUCCESS) {
        printf("Failed to create swap chain");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &swapChainImagesCount, NULL);
    swapChainImages = malloc(swapChainImagesCount * sizeof *swapChainImages);
    vkGetSwapchainImagesKHR(device, swapChain, &swapChainImagesCount, swapChainImages);
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void createImageViews() {
    swapChainImageViews = malloc(swapChainImagesCount * sizeof *swapChainImageViews);
    int i = 0;
    for (i; i < swapChainImagesCount; i++) {
        VkImageViewCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = swapChainImages[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = swapChainImageFormat,
                .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
                .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .subresourceRange.baseMipLevel = 0,
                .subresourceRange.levelCount = 1,
                .subresourceRange.baseArrayLayer = 0,
                .subresourceRange.layerCount = 1
        };
        if (vkCreateImageView(device, &createInfo, NULL, &swapChainImageViews[i]) != VK_SUCCESS) {
            printf("Failed to create image views");
        }
    }
}

char *readFile(const char *path, uint32_t *size) {
    FILE *shaderStream;
    if (fopen_s(&shaderStream, path, "rb")) {
        printf("failed to open file!");
    }
    fseek(shaderStream, 0, SEEK_END);
    size_t length = ftell(shaderStream);
    fseek(shaderStream, 0, SEEK_SET);
    char *shaderCode = (char *) malloc(length);
    fread(shaderCode, sizeof(char), length, shaderStream);
    fclose(shaderStream);

    *size = length;
    return shaderCode;
}

VkShaderModule createShaderModule(char *code, uint32_t size) {
    VkShaderModuleCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = size,
            .pCode = (const uint32_t *) code
    };

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        printf("Failed to create shader module");
    }
    return shaderModule;
}

VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
VkPipeline graphicsPipeline;

void createRenderPass() {
    VkAttachmentDescription colorAttachment = {
            .format = swapChainImageFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference colorAttachmentRef = {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef
    };

    VkSubpassDependency dependency = {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };

    VkRenderPassCreateInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &colorAttachment,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &dependency
    };

    if (vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass) != VK_SUCCESS) {
        printf("Failed to create render pass");
    }
}

void createGraphicsPipeline() {
    uint32_t vertSize, fragSize;
    char *vertShaderCode = readFile("./shaders/vert.spv", &vertSize);
    char *fragShaderCode = readFile("./shaders/frag.spv", &fragSize);
    VkShaderModule vertModule = createShaderModule(vertShaderCode, vertSize);
    VkShaderModule fragModule = createShaderModule(fragShaderCode, fragSize);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertModule,
            .pName = "main"
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragModule,
            .pName = "main"
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 0,
            .pVertexBindingDescriptions = NULL,
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions = NULL
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE
    };

    VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = 2,
            .pDynamicStates = dynamicStates
    };

    VkPipelineViewportStateCreateInfo viewportState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.0f,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 0.0f
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .sampleShadingEnable = VK_FALSE,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .minSampleShading = 1.0f,
            .pSampleMask = NULL,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                              VK_COLOR_COMPONENT_A_BIT,
            .blendEnable = VK_FALSE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment,
            .blendConstants[0] = 0.0f,
            .blendConstants[1] = 0.0f,
            .blendConstants[2] = 0.0f,
            .blendConstants[3] = 0.0f,
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 0,
            .pSetLayouts = NULL,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = NULL
    };

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout) != VK_SUCCESS) {
        printf("Failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = NULL,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = pipelineLayout,
            .renderPass = renderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1
    };

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline) != VK_SUCCESS) {
        printf("Failed to create graphics pipeline");
    }

    vkDestroyShaderModule(device, vertModule, NULL);
    vkDestroyShaderModule(device, fragModule, NULL);
}

VkFramebuffer *swapChainFramebuffers;

void createFrameBuffers() {
    swapChainFramebuffers = malloc(swapChainImagesCount * sizeof *swapChainFramebuffers);
    int i = 0;
    for (i; i < swapChainImagesCount; i++) {
        VkImageView attachments[] = {
                swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = renderPass,
                .attachmentCount = 1,
                .pAttachments = attachments,
                .width = swapChainExtent.width,
                .height = swapChainExtent.height,
                .layers = 1
        };

        if (vkCreateFramebuffer(device, &framebufferInfo, NULL, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            printf("Failed to create framebuffer");
        }
    }
}

VkCommandPool commandPool;
VkCommandBuffer commandBuffer;

void createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies();

    VkCommandPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = queueFamilyIndices.graphicsFamily
    };

    if (vkCreateCommandPool(device, &poolInfo, NULL, &commandPool) != VK_SUCCESS) {
        printf("Failed to create command pool");
    }
}

void createCommandBuffer() {
    VkCommandBufferAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
    };

    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        printf("Failed to allocate command buffer");
    }
}

void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = 0,
            .pInheritanceInfo = NULL
    };

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        printf("Failed to begin recording command buffer");
    }

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    VkRenderPassBeginInfo renderPassBeginInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = renderPass,
            .framebuffer = swapChainFramebuffers[imageIndex],
            .renderArea.offset = {0, 0},
            .renderArea.extent = swapChainExtent,
            .clearValueCount = 1,
            .pClearValues = &clearColor
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkViewport viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = (float) swapChainExtent.width,
            .height = (float) swapChainExtent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {
            .offset = {0, 0},
            .extent = swapChainExtent
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        printf("Failed to record command buffer");
    }
}

VkSemaphore imageAvailableSemaphore;
VkSemaphore renderFinishedSemaphore;
VkFence inFlightFence;

void createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    if (vkCreateSemaphore(device, &semaphoreInfo, NULL, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, NULL, &renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, NULL, &inFlightFence) != VK_SUCCESS) {
        printf("Failed to create semaphores");
    }
}

void handleEvents() {
    MSG msg = {0};

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void drawFrame() {
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFence);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    vkResetCommandBuffer(commandBuffer, 0);
    recordCommandBuffer(commandBuffer, imageIndex);

    VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO
    };

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
        printf("Failed to submit draw command buffers");
    }

    VkSwapchainKHR swapChains[] = {swapChain};
    VkPresentInfoKHR presentInfo = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = signalSemaphores,
            .swapchainCount = 1,
            .pSwapchains = swapChains,
            .pImageIndices = &imageIndex,
            .pResults = NULL
    };

    vkQueuePresentKHR(presentQueue, &presentInfo);

}

bool running = true;

void mainLoop() {
    while (running) {
        handleEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(device);
}

void cleanup() {
    vkDestroySemaphore(device, imageAvailableSemaphore, NULL);
    vkDestroySemaphore(device, renderFinishedSemaphore, NULL);
    vkDestroyFence(device, inFlightFence, NULL);
    vkDestroyCommandPool(device, commandPool, NULL);
    int i = 0;
    for (i; i < swapChainImagesCount; i++) {
        vkDestroyFramebuffer(device, swapChainFramebuffers[i], NULL);
    }
    vkDestroyPipeline(device, graphicsPipeline, NULL);
    vkDestroyPipelineLayout(device, pipelineLayout, NULL);
    vkDestroyRenderPass(device, renderPass, NULL);
    i = 0;
    for (i; i < swapChainImagesCount; i++) {
        vkDestroyImageView(device, swapChainImageViews[i], NULL);
    }
    vkDestroySwapchainKHR(device, swapChain, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroyInstance(instance, NULL);
}

void createWindow(char *name, uint32_t width, uint32_t height);

int main() {
    createWindow("asdf", 500, 500);
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFrameBuffers();
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();

    mainLoop();

    cleanup();
    return 0;
}

LRESULT WindowProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_QUIT:
        case WM_DESTROY:
            PostQuitMessage(0);
            running = false;
            return 0;
        case WM_SIZE:

            break;
    }
    return DefWindowProc(window, msg, wparam, lparam);
}

void createWindow(char *name, uint32_t width, uint32_t height) {
    WNDCLASS windowClass = {
            .hInstance = GetModuleHandle(NULL),
            .lpszClassName = name,
            .lpfnWndProc = WindowProc
    };
    RegisterClass(&windowClass);
    window = CreateWindow(name, name, WS_OVERLAPPEDWINDOW, 100, 100, width, height, NULL, NULL, GetModuleHandle(NULL),
                          0);
    ShowWindow(window, SHOW_FULLSCREEN);
}
