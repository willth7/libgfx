//   Copyright 2022 Will Thomas
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

//TODO
//	optimize per-frame functions
//	optimize per-resize functions
//	staging buffer?
//	textures
//	flexibility in drawing and binding buffers
//	have functions not segfault if objects aren't initialized

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

GLFWwindow* glfw_win;
uint32_t glfw_win_w;
uint32_t glfw_win_h;

VkInstance vk_inst;
VkSurfaceKHR vk_srfc;
VkDevice vk_devc;
VkQueue vk_que;

VkCommandPool vk_cmd_pool;
VkCommandBuffer vk_cmd_draw;
VkSemaphore vk_smph_img;
VkSemaphore vk_smph_drw;
VkFence vk_fnc;

VkShaderModule vk_vrtx_shdr;
VkShaderModule vk_frag_shdr;

VkRenderPass vk_rndr;
VkSwapchainKHR vk_swap;
VkPipelineLayout vk_pipe_layt;
VkPipeline vk_pipe;
VkFramebuffer* vk_frme;
uint32_t vk_frme_i;

VkImage* vk_swap_img;
VkImageView* vk_swap_img_v;
uint32_t vk_swap_img_cnt;

VkImage vk_dpth_img;
VkDeviceMemory vk_dpth_mem;
VkImageView vk_dpth_img_v;

VkBuffer vk_vrtx_bfr;
VkDeviceMemory vk_vrtx_mem;
VkMemoryRequirements vk_vrtx_req;
void* vk_vrtx;
uint64_t vk_vrtx_sz;
VkPipelineVertexInputStateCreateInfo vk_vrtx_in;

VkVertexInputBindingDescription* vk_vrtx_bind;
uint32_t vk_vrtx_bind_n;
VkVertexInputAttributeDescription* vk_vrtx_attr;
uint32_t vk_vrtx_attr_n;

VkBuffer vk_indx_bfr;
VkDeviceMemory vk_indx_mem;
VkMemoryRequirements vk_indx_req;
void* vk_indx;
uint64_t vk_indx_sz;

VkBuffer vk_unif_bfr;
VkDeviceMemory vk_unif_mem;
VkMemoryRequirements vk_unif_req;
void* vk_unif;
uint64_t vk_unif_sz;

VkPushConstantRange vk_push_rng;
uint32_t vk_push_rng_n;
void* vk_push;
uint64_t vk_push_sz;

VkDescriptorPool vk_desc_pool;
VkDescriptorSet vk_desc_set;
VkDescriptorSetLayout vk_desc_layt;
uint32_t vk_desc_layt_n;

VkClearValue vk_clr[2];

void init_vk_inst() {
	VkInstanceCreateInfo instinfo;
		instinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instinfo.pNext = 0;
		instinfo.flags = 0;
		instinfo.pApplicationInfo = 0;
		instinfo.enabledLayerCount = 0;
		instinfo.ppEnabledLayerNames = 0;
		instinfo.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&instinfo.enabledExtensionCount);
	vkCreateInstance(&instinfo, 0, &vk_inst);
}

void init_vk_devc() {
	int32_t gpuinfo;
	vkEnumeratePhysicalDevices(vk_inst, &gpuinfo, 0);
	VkPhysicalDevice* gpus = malloc(sizeof(VkPhysicalDevice) * gpuinfo);
	vkEnumeratePhysicalDevices(vk_inst, &gpuinfo, gpus);
	VkPhysicalDevice gpu = gpus[0];
	free(gpus);
	
	VkPhysicalDeviceProperties gpuprop;
	vkGetPhysicalDeviceProperties(gpu, &gpuprop);
	VkPhysicalDeviceFeatures gpufeat;
	vkGetPhysicalDeviceFeatures(gpu, &gpufeat);
	
	glfwCreateWindowSurface(vk_inst, glfw_win, 0, &vk_srfc);
	
	float prio = 0.f;
	VkDeviceQueueCreateInfo queinfo;
		queinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queinfo.pNext = 0;
		queinfo.flags = 0;
		queinfo.queueFamilyIndex = 0;
		queinfo.queueCount = 1;
		queinfo.pQueuePriorities = &prio;
	
	const char* ext = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	VkDeviceCreateInfo devcinfo;
		devcinfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		devcinfo.pNext = 0;
		devcinfo.flags = 0;
		devcinfo.queueCreateInfoCount = 1;
		devcinfo.pQueueCreateInfos = &queinfo;
		devcinfo.enabledLayerCount = 0;
		devcinfo.ppEnabledLayerNames = 0;
		devcinfo.enabledExtensionCount = 1;
		devcinfo.ppEnabledExtensionNames = &ext;
		devcinfo.pEnabledFeatures = &gpufeat;
	vkCreateDevice(gpu, &devcinfo, 0, &vk_devc);
	vkGetDeviceQueue(vk_devc, 0, 0, &vk_que);
}

void init_vk_rndr() {
	VkAttachmentDescription atch[2];
		atch[0].flags = 0;
		atch[0].format = VK_FORMAT_B8G8R8A8_SRGB;
		atch[0].samples = VK_SAMPLE_COUNT_1_BIT;
		atch[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		atch[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		atch[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		atch[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		atch[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		atch[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		atch[1].flags = 0;
		atch[1].format = VK_FORMAT_D32_SFLOAT;
		atch[1].samples = VK_SAMPLE_COUNT_1_BIT;
		atch[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		atch[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		atch[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		atch[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		atch[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		atch[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	VkAttachmentReference colref;
		colref.attachment = 0;
		colref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentReference depref;
		depref.attachment = 1;
		depref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	VkSubpassDescription subdesc;
		subdesc.flags = 0;
		subdesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subdesc.inputAttachmentCount = 0;
		subdesc.pInputAttachments = 0;
		subdesc.colorAttachmentCount = 1;
		subdesc.pColorAttachments = &colref;
		subdesc.pResolveAttachments = 0;
		subdesc.pDepthStencilAttachment = &depref;
		subdesc.preserveAttachmentCount = 0;
		subdesc.pPreserveAttachments = 0;
	VkRenderPassCreateInfo rndrinfo;
		rndrinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rndrinfo.pNext = 0;
		rndrinfo.flags = 0;
		rndrinfo.attachmentCount = 2;
		rndrinfo.pAttachments = atch;
		rndrinfo.subpassCount = 1;
		rndrinfo.pSubpasses = &subdesc;
		rndrinfo.dependencyCount = 0;
		rndrinfo.pDependencies = 0;
	vkCreateRenderPass(vk_devc, &rndrinfo, 0, &vk_rndr);
}

void init_vk_cmd_pool() {
	VkCommandPoolCreateInfo cmd_poolinfo;
		cmd_poolinfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmd_poolinfo.pNext = 0;
		cmd_poolinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		cmd_poolinfo.queueFamilyIndex = 0;
	vkCreateCommandPool(vk_devc, &cmd_poolinfo, 0, &vk_cmd_pool);
	
	VkCommandBufferAllocateInfo cmdinfo;
		cmdinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdinfo.pNext = 0;
		cmdinfo.commandPool = vk_cmd_pool;
		cmdinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdinfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(vk_devc, &cmdinfo, &vk_cmd_draw);
}

void init_vk_smph() {
	VkSemaphoreCreateInfo smphinfo;
		smphinfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		smphinfo.pNext = 0;
		smphinfo.flags = 0;
	vkCreateSemaphore(vk_devc, &smphinfo, 0, &vk_smph_img);
	vkCreateSemaphore(vk_devc, &smphinfo, 0, &vk_smph_drw);
	
	VkFenceCreateInfo fncinfo;
		fncinfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fncinfo.pNext = 0;
		fncinfo.flags = 0;
	vkCreateFence(vk_devc, &fncinfo, 0, &vk_fnc);
}

void init_vk_swap() {
	VkSwapchainKHR swap_anc = vk_swap;
	VkSwapchainCreateInfoKHR swapinfo;
		swapinfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapinfo.pNext = 0;
		swapinfo.flags = 0;
		swapinfo.surface = vk_srfc;
		swapinfo.minImageCount = 3;
		swapinfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
		swapinfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		swapinfo.imageExtent.width = glfw_win_w;
		swapinfo.imageExtent.height = glfw_win_h;
		swapinfo.imageArrayLayers = 1;
		swapinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapinfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapinfo.queueFamilyIndexCount = 0;
		swapinfo.pQueueFamilyIndices = 0;
		swapinfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapinfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapinfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
		swapinfo.clipped = 1;
		swapinfo.oldSwapchain = swap_anc;
	vkCreateSwapchainKHR(vk_devc, &swapinfo, 0, &vk_swap);
	if (swap_anc != 0) {
		vkDestroySwapchainKHR(vk_devc, swap_anc, 0);
	}
	
	vkGetSwapchainImagesKHR(vk_devc, vk_swap, &vk_swap_img_cnt, 0);
	vk_swap_img = malloc(sizeof(VkImage) * vk_swap_img_cnt);
	vkGetSwapchainImagesKHR(vk_devc, vk_swap, &vk_swap_img_cnt, vk_swap_img);
	vk_swap_img_v = malloc(sizeof(VkImageView) * vk_swap_img_cnt);
	VkImageViewCreateInfo imgvinfo;
		imgvinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imgvinfo.pNext = 0;
		imgvinfo.flags = 0;
		imgvinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imgvinfo.format = VK_FORMAT_B8G8R8A8_SRGB;
		imgvinfo.components.r = VK_COMPONENT_SWIZZLE_R;
		imgvinfo.components.g = VK_COMPONENT_SWIZZLE_G;
		imgvinfo.components.b = VK_COMPONENT_SWIZZLE_B;
		imgvinfo.components.a = VK_COMPONENT_SWIZZLE_A;
		imgvinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgvinfo.subresourceRange.baseMipLevel = 0;
		imgvinfo.subresourceRange.levelCount = 1;
		imgvinfo.subresourceRange.baseArrayLayer = 0;
		imgvinfo.subresourceRange.layerCount = 1;
	for (uint32_t i = 0; i < vk_swap_img_cnt; i++) {
		imgvinfo.image = vk_swap_img[i];
		vkCreateImageView(vk_devc, &imgvinfo, 0, &vk_swap_img_v[i]);
	}
}

void init_vk_dpth() {
	VkImageCreateInfo imginfo;
		imginfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imginfo.pNext = 0;
		imginfo.flags = 0;
		imginfo.imageType = VK_IMAGE_TYPE_2D;
		imginfo.format = VK_FORMAT_D32_SFLOAT;
		imginfo.extent.width = glfw_win_w;
		imginfo.extent.height = glfw_win_h;
		imginfo.mipLevels = 1;
		imginfo.arrayLayers = 1;
		imginfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imginfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imginfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imginfo.sharingMode = 0;
		imginfo.queueFamilyIndexCount = 0;
		imginfo.pQueueFamilyIndices = 0;
		imginfo.initialLayout = 0;
	vkCreateImage(vk_devc, &imginfo, 0, &vk_dpth_img);
	
	VkMemoryRequirements memreq;
	vkGetImageMemoryRequirements(vk_devc, vk_dpth_img, &memreq);
	VkMemoryAllocateInfo meminfo;
		meminfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		meminfo.pNext = 0;
		meminfo.allocationSize = memreq.size;
		meminfo.memoryTypeIndex = 0;
	vkAllocateMemory(vk_devc, &meminfo, 0, &vk_dpth_mem);
	vkBindImageMemory(vk_devc, vk_dpth_img, vk_dpth_mem, 0);
	
	VkImageViewCreateInfo imgvinfo;
		imgvinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imgvinfo.pNext = 0;
		imgvinfo.flags = 0;
		imgvinfo.image = vk_dpth_img;
		imgvinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imgvinfo.format = VK_FORMAT_D32_SFLOAT;
		imgvinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		imgvinfo.subresourceRange.baseMipLevel = 0;
		imgvinfo.subresourceRange.levelCount = 1;
		imgvinfo.subresourceRange.baseArrayLayer = 0;
		imgvinfo.subresourceRange.layerCount = 1;
	vkCreateImageView(vk_devc, &imgvinfo, 0, &vk_dpth_img_v);
}

void init_vk_pipe() {
	VkPipelineShaderStageCreateInfo shdinfo[2];
		shdinfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shdinfo[0].pNext = 0;
		shdinfo[0].flags = 0;
		shdinfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shdinfo[0].module = vk_vrtx_shdr;
		shdinfo[0].pName = "main";
		shdinfo[0].pSpecializationInfo = 0;
		shdinfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shdinfo[1].pNext = 0;
		shdinfo[1].flags = 0;
		shdinfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shdinfo[1].module = vk_frag_shdr;
		shdinfo[1].pName = "main";
		shdinfo[1].pSpecializationInfo = 0;
	
	VkPipelineInputAssemblyStateCreateInfo inasminfo;
		inasminfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inasminfo.pNext = 0;
		inasminfo.flags = 0;
		inasminfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		inasminfo.primitiveRestartEnable = 1;
		
	VkViewport vprt;
		vprt.x = 0.f;
		vprt.y = 0.f;
		vprt.width = (float) glfw_win_w;
		vprt.height = (float) glfw_win_h;
		vprt.minDepth = 0.f;
		vprt.maxDepth = 1.f;
	
	VkRect2D scsr;
		scsr.offset.x = 0;
		scsr.offset.y = 0;
		scsr.extent.width = glfw_win_w;
		scsr.extent.height = glfw_win_h;
		
	VkPipelineViewportStateCreateInfo vprtinfo;
		vprtinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		vprtinfo.pNext = 0;
		vprtinfo.flags = 0;
		vprtinfo.viewportCount = 1;
		vprtinfo.pViewports = &vprt;
		vprtinfo.scissorCount = 1;
		vprtinfo.pScissors = &scsr;
	
	VkPipelineRasterizationStateCreateInfo rstrinfo;
		rstrinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rstrinfo.pNext = 0;
		rstrinfo.flags = 0;
		rstrinfo.depthClampEnable = 0;
		rstrinfo.rasterizerDiscardEnable = 0;
		rstrinfo.polygonMode = VK_POLYGON_MODE_FILL;
		rstrinfo.cullMode = VK_CULL_MODE_NONE;
		rstrinfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rstrinfo.depthBiasEnable = 0;
		rstrinfo.depthBiasConstantFactor = 0.f;
		rstrinfo.depthBiasClamp = 0.f;
		rstrinfo.depthBiasSlopeFactor = 0.f;
		rstrinfo.lineWidth = 1.f;
		
	VkPipelineMultisampleStateCreateInfo multinfo;
		multinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multinfo.pNext = 0;
		multinfo.flags = 0;
		multinfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multinfo.sampleShadingEnable = 0;
		multinfo.minSampleShading = 1.f;
		multinfo.pSampleMask = 0;
		multinfo.alphaToCoverageEnable = 0;
		multinfo.alphaToOneEnable = 0;
		
	VkPipelineDepthStencilStateCreateInfo dpthinfo = {};
		dpthinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		dpthinfo.pNext = 0;
		dpthinfo.flags = 0;
		dpthinfo.depthTestEnable = 1;
		dpthinfo.depthWriteEnable = 1;
		dpthinfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		dpthinfo.depthBoundsTestEnable = 0;
		dpthinfo.stencilTestEnable = 0;
		dpthinfo.minDepthBounds = 0.f;
		dpthinfo.maxDepthBounds = 1.f;
		
	VkPipelineColorBlendAttachmentState colblndatch;
		colblndatch.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colblndatch.blendEnable = 0;
		colblndatch.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colblndatch.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colblndatch.colorBlendOp = VK_BLEND_OP_ADD;
		colblndatch.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colblndatch.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colblndatch.alphaBlendOp = VK_BLEND_OP_ADD;
		
	VkPipelineColorBlendStateCreateInfo colblndinfo;
		colblndinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colblndinfo.pNext = 0;
		colblndinfo.flags = 0;
		colblndinfo.logicOpEnable = 0;
		colblndinfo.logicOp = VK_LOGIC_OP_COPY;
		colblndinfo.attachmentCount = 1;
		colblndinfo.pAttachments = &colblndatch;
		colblndinfo.blendConstants[0] = 0.f;
		colblndinfo.blendConstants[1] = 0.f;
		colblndinfo.blendConstants[2] = 0.f;
		colblndinfo.blendConstants[3] = 0.f;
		
	VkDynamicState dyn[] = { VK_DYNAMIC_STATE_VIEWPORT };
	VkPipelineDynamicStateCreateInfo dyninfo;
		dyninfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dyninfo.pNext = 0;
		dyninfo.flags = 0;
		dyninfo.dynamicStateCount = 2;
		dyninfo.pDynamicStates = dyn;
	
	VkPipelineLayoutCreateInfo pipelaytinfo;
		pipelaytinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelaytinfo.pNext = 0;
		pipelaytinfo.flags = 0;
		pipelaytinfo.setLayoutCount = vk_desc_layt_n;
		pipelaytinfo.pSetLayouts = &vk_desc_layt;
		pipelaytinfo.pushConstantRangeCount = vk_push_rng_n;
		pipelaytinfo.pPushConstantRanges = &vk_push_rng;
	vkCreatePipelineLayout(vk_devc, &pipelaytinfo, 0, &vk_pipe_layt);
	
	VkGraphicsPipelineCreateInfo pipeinfo;
		pipeinfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeinfo.pNext = 0;
		pipeinfo.flags = 0;
		pipeinfo.stageCount = 2;
		pipeinfo.pStages = shdinfo;
		pipeinfo.pVertexInputState = &vk_vrtx_in;
		pipeinfo.pInputAssemblyState = &inasminfo;
		pipeinfo.pTessellationState = 0;
		pipeinfo.pViewportState = &vprtinfo;
		pipeinfo.pRasterizationState = &rstrinfo;
		pipeinfo.pMultisampleState = &multinfo;
		pipeinfo.pDepthStencilState = &dpthinfo;
		pipeinfo.pColorBlendState = &colblndinfo;
		pipeinfo.pDynamicState = &dyninfo;
		pipeinfo.layout = vk_pipe_layt;
		pipeinfo.renderPass = vk_rndr;
		pipeinfo.subpass = 0;
		pipeinfo.basePipelineHandle = 0;
		pipeinfo.basePipelineIndex = 0;
	vkCreateGraphicsPipelines(vk_devc, 0, 1, &pipeinfo, 0, &vk_pipe);
}

void init_vk_frme() {
	VkImageView atch[2];
	atch[1] = vk_dpth_img_v;
	VkFramebufferCreateInfo fbfrinfo;
		fbfrinfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbfrinfo.pNext = 0;
		fbfrinfo.flags = 0;
		fbfrinfo.renderPass = vk_rndr;
		fbfrinfo.attachmentCount = 2;
		fbfrinfo.pAttachments = atch;
		fbfrinfo.width = glfw_win_w;
		fbfrinfo.height = glfw_win_h;
		fbfrinfo.layers = 1;
		vk_frme = malloc(sizeof(VkFramebuffer) * vk_swap_img_cnt);
	for (uint32_t i = 0; i < vk_swap_img_cnt; i++) {
		atch[0] = vk_swap_img_v[i];
		vkCreateFramebuffer(vk_devc, &fbfrinfo, 0, &vk_frme[i]);
	}
}

void init_vk_cmd_draw(uint64_t n) {
	VkCommandBufferBeginInfo cbfrinfo;
		cbfrinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cbfrinfo.pNext = 0;
		cbfrinfo.flags = 0;
		cbfrinfo.pInheritanceInfo = 0;
	vkBeginCommandBuffer(vk_cmd_draw, &cbfrinfo);
	
	VkRenderPassBeginInfo rndrinfo;
		rndrinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rndrinfo.pNext = 0;
		rndrinfo.renderPass = vk_rndr;
		rndrinfo.framebuffer = vk_frme[vk_frme_i];
		rndrinfo.renderArea.offset.x = 0;
		rndrinfo.renderArea.offset.y = 0;
		rndrinfo.renderArea.extent.width = glfw_win_w;
		rndrinfo.renderArea.extent.height = glfw_win_h;
		rndrinfo.clearValueCount = 2;
		rndrinfo.pClearValues = vk_clr;
	vkCmdBeginRenderPass(vk_cmd_draw, &rndrinfo, VK_SUBPASS_CONTENTS_INLINE);
	
	vkCmdBindPipeline(vk_cmd_draw, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipe);
	
	VkViewport vprt;
		vprt.x = 0.f;
		vprt.y = 0.f;
		vprt.width = (float) glfw_win_w;
		vprt.height = (float) glfw_win_h;
		vprt.minDepth = 0.f;
		vprt.maxDepth = 1.f;
	vkCmdSetViewport(vk_cmd_draw, 0, 1, &vprt);
	
	VkDeviceSize offset = {0};
	vkCmdBindVertexBuffers(vk_cmd_draw, 0, 1, &vk_vrtx_bfr, &offset);
	vkCmdBindIndexBuffer(vk_cmd_draw, vk_indx_bfr, 0, VK_INDEX_TYPE_UINT32);
	if (vk_desc_set != 0) vkCmdBindDescriptorSets(vk_cmd_draw, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipe_layt, 0, 1, &vk_desc_set, 0, 0);
	vkCmdPushConstants(vk_cmd_draw, vk_pipe_layt, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, vk_push_sz, vk_push);
	vkCmdDrawIndexed(vk_cmd_draw, n, 1, 0, 0, 0);

	vkCmdEndRenderPass(vk_cmd_draw);
	vkEndCommandBuffer(vk_cmd_draw);
}

void rfsh_vk_vrtx() {
	void* data;
	vkMapMemory(vk_devc, vk_vrtx_mem, 0, vk_vrtx_req.size, 0, &data);
	memcpy(data, vk_vrtx, vk_vrtx_sz);
	vkUnmapMemory(vk_devc, vk_vrtx_mem);
	vkBindBufferMemory(vk_devc, vk_vrtx_bfr, vk_vrtx_mem, 0);
}

void rfsh_vk_indx() {
	void* data;
	vkMapMemory(vk_devc, vk_indx_mem, 0, vk_indx_req.size, 0, &data);
	memcpy(data, vk_indx, vk_indx_sz);
	vkUnmapMemory(vk_devc, vk_indx_mem);
	vkBindBufferMemory(vk_devc, vk_indx_bfr, vk_indx_mem, 0);
}

void rfsh_vk_unif() {
	void* data;
	vkMapMemory(vk_devc, vk_unif_mem, 0, vk_unif_req.size, 0, &data);
	memcpy(data, vk_unif, vk_unif_sz);
	vkUnmapMemory(vk_devc, vk_unif_mem);
	vkBindBufferMemory(vk_devc, vk_unif_bfr, vk_unif_mem, 0);
}

void gfx_init(GLFWwindow* win) {
	glfw_win = win;
	glfwGetWindowSize(glfw_win, &glfw_win_w, &glfw_win_h);
	init_vk_inst();
	init_vk_devc();
	init_vk_rndr();
	init_vk_cmd_pool();
	init_vk_smph();
}

void gfx_set() {
	init_vk_swap();
	init_vk_dpth();
	init_vk_pipe();
	init_vk_frme();
}

void gfx_resz(void* v, uint32_t w, uint32_t h) {
	for (uint32_t i = 0; i < vk_swap_img_cnt; i++) {
		vkDestroyFramebuffer(vk_devc, vk_frme[i], 0);
	}
	free(vk_frme);
	
	vkDestroyPipeline(vk_devc, vk_pipe, 0);
	vkDestroyPipelineLayout(vk_devc, vk_pipe_layt, 0);
	
	vkDestroyImageView(vk_devc, vk_dpth_img_v, 0);
	vkDestroyImage(vk_devc, vk_dpth_img, 0);
	vkFreeMemory(vk_devc, vk_dpth_mem, 0);
	
	for (uint32_t i = 0; i < vk_swap_img_cnt; i++) {
		vkDestroyImageView(vk_devc, vk_swap_img_v[i], 0);
	}
	free(vk_swap_img);
	free(vk_swap_img_v);
	
	glfw_win_w = w;
	glfw_win_h = h;
	
	gfx_set();
}

void gfx_init_vrtx(void* vrtx, uint64_t sz) {
	vk_vrtx = vrtx;
	vk_vrtx_sz = sz;
	
	VkBufferCreateInfo bfrinfo;
		bfrinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bfrinfo.pNext = 0;
		bfrinfo.flags = 0;
		bfrinfo.size = vk_vrtx_sz;
		bfrinfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bfrinfo.sharingMode = 0;
		bfrinfo.queueFamilyIndexCount = 0;
		bfrinfo.pQueueFamilyIndices = 0;
	vkCreateBuffer(vk_devc, &bfrinfo, 0, &vk_vrtx_bfr);
	
	vkGetBufferMemoryRequirements(vk_devc, vk_vrtx_bfr, &vk_vrtx_req);
	VkMemoryAllocateInfo meminfo;
		meminfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		meminfo.pNext = 0;
		meminfo.allocationSize = vk_vrtx_req.size;
		meminfo.memoryTypeIndex = 0;
	vkAllocateMemory(vk_devc, &meminfo, 0, &vk_vrtx_mem);
	
	vk_vrtx_in.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vk_vrtx_in.pNext = 0;
	vk_vrtx_in.flags = 0;
	vk_vrtx_in.vertexBindingDescriptionCount = vk_vrtx_bind_n;
	vk_vrtx_in.pVertexBindingDescriptions = vk_vrtx_bind;
	vk_vrtx_in.vertexAttributeDescriptionCount = vk_vrtx_attr_n;
	vk_vrtx_in.pVertexAttributeDescriptions = vk_vrtx_attr;
}

void gfx_init_vrtx_bind(uint32_t n) {
	vk_vrtx_bind = malloc(sizeof(VkVertexInputBindingDescription) * n);
	vk_vrtx_bind_n = n;
}

void gfx_set_vrtx_bind(uint32_t b, uint32_t s) {
	vk_vrtx_bind[b].binding = b;
	vk_vrtx_bind[b].stride = s;
	vk_vrtx_bind[b].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

void gfx_init_vrtx_attr(uint32_t n) {
	vk_vrtx_attr = malloc(sizeof(VkVertexInputAttributeDescription) * n);
	vk_vrtx_attr_n = n;
}

void gfx_set_vrtx_attr(uint32_t l, uint32_t b, uint32_t off) {
	vk_vrtx_attr[l].location = l;
	vk_vrtx_attr[l].binding = b;
	vk_vrtx_attr[l].format = VK_FORMAT_R32G32B32_SINT;
	vk_vrtx_attr[l].offset = off;
}

void gfx_init_indx(void* indx, uint64_t sz) {
	vk_indx = indx;
	vk_indx_sz = sz;
	
	VkBufferCreateInfo bfrinfo;
		bfrinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bfrinfo.pNext = 0;
		bfrinfo.flags = 0;
		bfrinfo.size = vk_indx_sz;
		bfrinfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		bfrinfo.sharingMode = 0;
		bfrinfo.queueFamilyIndexCount = 0;
		bfrinfo.pQueueFamilyIndices = 0;
	vkCreateBuffer(vk_devc, &bfrinfo, 0, &vk_indx_bfr);
	
	vkGetBufferMemoryRequirements(vk_devc, vk_indx_bfr, &vk_indx_req);
	VkMemoryAllocateInfo meminfo;
		meminfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		meminfo.pNext = 0;
		meminfo.allocationSize = vk_indx_req.size;
		meminfo.memoryTypeIndex = 0;
	vkAllocateMemory(vk_devc, &meminfo, 0, &vk_indx_mem);
}

void gfx_init_unif(void* unif, uint64_t sz) {
	vk_unif = unif;
	vk_unif_sz = sz;

	VkBufferCreateInfo bfrinfo;
		bfrinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bfrinfo.pNext = 0;
		bfrinfo.flags = 0;
		bfrinfo.size = vk_unif_sz;
		bfrinfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bfrinfo.sharingMode = 0;
		bfrinfo.queueFamilyIndexCount = 0;
		bfrinfo.pQueueFamilyIndices = 0;
	vkCreateBuffer(vk_devc, &bfrinfo, 0, &vk_unif_bfr);
	
	vkGetBufferMemoryRequirements(vk_devc, vk_unif_bfr, &vk_unif_req);
	VkMemoryAllocateInfo meminfo;
		meminfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		meminfo.pNext = 0;
		meminfo.allocationSize = vk_unif_req.size;
		meminfo.memoryTypeIndex = 0;
	vkAllocateMemory(vk_devc, &meminfo, 0, &vk_unif_mem);
}

void gfx_init_push(void* push, uint64_t sz, uint32_t n) {
	vk_push = push;
	vk_push_sz = sz;
	vk_push_rng.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	vk_push_rng.offset = 0;
	vk_push_rng.size = sz;
	vk_push_rng_n = 1;
}

void gfx_init_desc() {
	VkDescriptorPoolSize descpoolsz;
		descpoolsz.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descpoolsz.descriptorCount = 2;
	VkDescriptorPoolCreateInfo descpoolinfo;
		descpoolinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descpoolinfo.pNext = 0;
		descpoolinfo.flags = 0;
		descpoolinfo.maxSets = 2;
		descpoolinfo.poolSizeCount = 1;
		descpoolinfo.pPoolSizes = &descpoolsz;
	vkCreateDescriptorPool(vk_devc, &descpoolinfo, 0, &vk_desc_pool);
	
	VkDescriptorSetLayoutBinding desclaytbind;
		desclaytbind.binding = 0;
		desclaytbind.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		desclaytbind.descriptorCount = 1;
		desclaytbind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		desclaytbind.pImmutableSamplers = 0;
	VkDescriptorSetLayoutCreateInfo desclaytinfo;
		desclaytinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		desclaytinfo.pNext = 0;
		desclaytinfo.flags = 0;
		desclaytinfo.bindingCount = 1;
		desclaytinfo.pBindings = &desclaytbind;
	vkCreateDescriptorSetLayout(vk_devc, &desclaytinfo, 0, &vk_desc_layt);
	vk_desc_layt_n = 1;
	
	VkDescriptorSetAllocateInfo descalcinfo;
		descalcinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descalcinfo.pNext = 0;
		descalcinfo.descriptorPool = vk_desc_pool;
		descalcinfo.descriptorSetCount = 1;
		descalcinfo.pSetLayouts = &vk_desc_layt;
	vkAllocateDescriptorSets(vk_devc, &descalcinfo, &vk_desc_set);
	
	VkDescriptorBufferInfo descbfrinfo;
		descbfrinfo.buffer = vk_unif_bfr;
		descbfrinfo.offset = 0;
		descbfrinfo.range = vk_unif_sz;
	rfsh_vk_unif();
	VkWriteDescriptorSet descwrit;
		descwrit.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descwrit.pNext = 0;
		descwrit.dstSet = vk_desc_set;
		descwrit.dstBinding = 0;
		descwrit.dstArrayElement = 0;
		descwrit.descriptorCount = 1;
		descwrit.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descwrit.pImageInfo = 0;
		descwrit.pBufferInfo = &descbfrinfo;
		descwrit.pTexelBufferView = 0;
	vkUpdateDescriptorSets(vk_devc, 1, &descwrit, 0, 0);
}

void gfx_set_shdr(int8_t* pthv, int8_t* pthf) {
	FILE* f = fopen(pthv, "rb");
	fseek(f, 0, SEEK_END);
	uint64_t sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint32_t* src = malloc(sz);
	fread(src, sz, 1, f);
	fclose(f);
	VkShaderModuleCreateInfo shdinfo;
		shdinfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shdinfo.pNext = 0;
		shdinfo.flags = 0;
		shdinfo.codeSize = sz;
		shdinfo.pCode = src;
	vkCreateShaderModule(vk_devc, &shdinfo, 0, &vk_vrtx_shdr);
	free(src);
	
	f = fopen(pthf, "rb");
	fseek(f, 0, SEEK_END);
	sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	src = malloc(sz);
	fread(src, sz, 1, f);
	fclose(f);
		shdinfo.codeSize = sz;
		shdinfo.pCode = src;
	vkCreateShaderModule(vk_devc, &shdinfo, 0, &vk_frag_shdr);
	free(src);
}

void gfx_set_clr(uint8_t r, uint8_t g, uint8_t b) {
	vk_clr[0].color.float32[0] = (float) r / 255;
	vk_clr[0].color.float32[1] = (float) g / 255;
	vk_clr[0].color.float32[2] = (float) b / 255;
	vk_clr[0].color.float32[3] = 1.f;
	vk_clr[1].depthStencil.depth = 1.f;
	vk_clr[1].depthStencil.stencil = 0;
}

void gfx_draw(uint64_t n) {
	rfsh_vk_vrtx();
	rfsh_vk_indx();
	if (vk_unif != 0) rfsh_vk_unif();
	
	vkAcquireNextImageKHR(vk_devc, vk_swap, UINT64_MAX, vk_smph_img, 0, &vk_frme_i);
	
	init_vk_cmd_draw(n);
	
	VkPipelineStageFlags pipeflag = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkSubmitInfo sbmtinfo;
		sbmtinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		sbmtinfo.pNext = 0;
		sbmtinfo.waitSemaphoreCount = 1;
		sbmtinfo.pWaitSemaphores = &vk_smph_img;
		sbmtinfo.pWaitDstStageMask = &pipeflag;
		sbmtinfo.commandBufferCount = 1;
		sbmtinfo.pCommandBuffers = &vk_cmd_draw;
		sbmtinfo.signalSemaphoreCount = 1;
		sbmtinfo.pSignalSemaphores = &vk_smph_drw;
	vkQueueSubmit(vk_que, 1, &sbmtinfo, vk_fnc);
	
	VkPresentInfoKHR preinfo;
		preinfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		preinfo.pNext = 0;
		preinfo.waitSemaphoreCount = 1;
		preinfo.pWaitSemaphores = &vk_smph_drw;
		preinfo.swapchainCount = 1;
		preinfo.pSwapchains = &vk_swap;
		preinfo.pImageIndices = &vk_frme_i;
		preinfo.pResults = 0;
	vkQueuePresentKHR(vk_que, &preinfo);
	
	vkWaitForFences(vk_devc, 1, &vk_fnc, 1, UINT64_MAX);
	vkResetFences(vk_devc, 1, &vk_fnc);
}

void gfx_term() {
	for (uint32_t i = 0; i < vk_swap_img_cnt; i++) {
		vkDestroyFramebuffer(vk_devc, vk_frme[i], 0);
	}
	free(vk_frme);
	
	vkDestroyPipeline(vk_devc, vk_pipe, 0);
	vkDestroyPipelineLayout(vk_devc, vk_pipe_layt, 0);
	vkDestroyRenderPass(vk_devc, vk_rndr, 0);
	
	vkDestroyImageView(vk_devc, vk_dpth_img_v, 0);
	vkDestroyImage(vk_devc, vk_dpth_img, 0);
	vkFreeMemory(vk_devc, vk_dpth_mem, 0);
	
	for (uint32_t i = 0; i < vk_swap_img_cnt; i++) {
		vkDestroyImageView(vk_devc, vk_swap_img_v[i], 0);
	}
	free(vk_swap_img);
	free(vk_swap_img_v);
	vkDestroySwapchainKHR(vk_devc, vk_swap, 0);
	
	vkDestroyBuffer(vk_devc, vk_vrtx_bfr, 0);
	vkFreeMemory(vk_devc, vk_vrtx_mem, 0);
	if (vk_vrtx_bind != 0) free(vk_vrtx_bind);
	if (vk_vrtx_attr != 0) free(vk_vrtx_attr);
	
	vkDestroyBuffer(vk_devc, vk_indx_bfr, 0);
	vkFreeMemory(vk_devc, vk_indx_mem, 0);
	
	vkDestroyBuffer(vk_devc, vk_unif_bfr, 0);
	vkFreeMemory(vk_devc, vk_unif_mem, 0);
	
	vkDestroyShaderModule(vk_devc, vk_vrtx_shdr, 0);
	vkDestroyShaderModule(vk_devc, vk_frag_shdr, 0);
	
	vkFreeDescriptorSets(vk_devc, vk_desc_pool, 1, &vk_desc_set);
	vkDestroyDescriptorSetLayout(vk_devc, vk_desc_layt, 0);
	vkDestroyDescriptorPool(vk_devc, vk_desc_pool, 0);
	
	vkFreeCommandBuffers(vk_devc, vk_cmd_pool, 1, &vk_cmd_draw);
	vkDestroyCommandPool(vk_devc, vk_cmd_pool, 0);
	
	vkDestroySemaphore(vk_devc, vk_smph_img, 0);
	vkDestroySemaphore(vk_devc, vk_smph_drw, 0);
	vkDestroyFence(vk_devc, vk_fnc, 0);
	
	vkDestroyDevice(vk_devc, 0);
	vkDestroySurfaceKHR(vk_inst, vk_srfc, 0);
	vkDestroyInstance(vk_inst, 0);
}
