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
//	staging buffers?
//	command buffer threading

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct gfx_bfr_s {
	VkBuffer bfr;
	VkDeviceMemory mem;
	VkMemoryRequirements req;
} gfx_bfr_t;

typedef struct gfx_img_s {
	VkImage img;
	VkImageView v;
	VkDeviceMemory mem;
	VkMemoryRequirements req;
} gfx_img_t;

typedef struct gfx_s {
	VkInstance inst;
	VkDevice devc;
	VkQueue que;
	VkSemaphore smph_img;
	VkSemaphore smph_drw;
	VkFence fnc;

} gfx_t;

typedef struct gfx_win_s {
	VkSurfaceKHR srfc;
	uint32_t w;
	uint32_t h;
	VkRenderPass rndr;
	VkSwapchainKHR swap;
	VkImage* swap_img;
	VkImageView* swap_img_v;
	VkFramebuffer* frme;
	uint32_t img_n;
	uint32_t img_i;
	gfx_img_t dpth;
	VkClearValue clr[2];
} gfx_win_t;

typedef struct gfx_cmd_s {
	VkCommandPool pool;
	VkCommandBuffer draw;
} gfx_cmd_t;

typedef struct gfx_pipe_s {
	VkPipeline pipe;
	VkPipelineLayout layt;
	VkShaderModule vrtx_shdr;
	VkShaderModule frag_shdr;
} gfx_pipe_t;

typedef struct gfx_vrtx_s {
	VkBuffer* bfr;
	VkDeviceMemory* mem;
	VkMemoryRequirements* req;
	VkPipelineVertexInputStateCreateInfo in;
	VkVertexInputBindingDescription* bind;
	uint32_t b;
	VkVertexInputAttributeDescription* attr;
	uint32_t a;
} gfx_vrtx_t;

typedef struct gfx_txtr_s {
	gfx_img_t img;
	VkSampler smpl;
} gfx_txtr_t;

typedef struct gfx_dscr_s {
	VkDescriptorPool pool;
	VkDescriptorSet* set;
	VkDescriptorSetLayout* layt;
	uint32_t n;
} gfx_dscr_t;

gfx_t* gfx_init() {
	gfx_t* gfx = malloc(sizeof(gfx_t));
	VkInstanceCreateInfo instinfo;
		instinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instinfo.pNext = 0;
		instinfo.flags = 0;
		instinfo.pApplicationInfo = 0;
		instinfo.enabledLayerCount = 0;
		instinfo.ppEnabledLayerNames = 0;
		instinfo.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&instinfo.enabledExtensionCount);
	vkCreateInstance(&instinfo, 0, &(gfx->inst));

	int32_t gpuinfo;
	vkEnumeratePhysicalDevices(gfx->inst, &gpuinfo, 0);
	VkPhysicalDevice* gpus = malloc(sizeof(VkPhysicalDevice) * gpuinfo);
	vkEnumeratePhysicalDevices(gfx->inst, &gpuinfo, gpus);
	VkPhysicalDevice gpu = gpus[0];
	free(gpus);
	
	VkPhysicalDeviceProperties gpuprop;
	vkGetPhysicalDeviceProperties(gpu, &gpuprop);
	VkPhysicalDeviceFeatures gpufeat;
	vkGetPhysicalDeviceFeatures(gpu, &gpufeat);
	
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
	vkCreateDevice(gpu, &devcinfo, 0, &(gfx->devc));
	vkGetDeviceQueue(gfx->devc, 0, 0, &(gfx->que));
	
	VkSemaphoreCreateInfo smphinfo;
		smphinfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		smphinfo.pNext = 0;
		smphinfo.flags = 0;
	vkCreateSemaphore(gfx->devc, &smphinfo, 0, &(gfx->smph_img));
	vkCreateSemaphore(gfx->devc, &smphinfo, 0, &(gfx->smph_drw));
	
	VkFenceCreateInfo fncinfo;
		fncinfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fncinfo.pNext = 0;
		fncinfo.flags = 0;
	vkCreateFence(gfx->devc, &fncinfo, 0, &(gfx->fnc));
	
	return gfx;
}

gfx_win_t* gfx_win_init(gfx_t* gfx, GLFWwindow* glfw_win) {
	gfx_win_t* win = calloc(1, sizeof(gfx_win_t));
	
	glfwGetWindowSize(glfw_win, &(win->w), &(win->h));
	glfwCreateWindowSurface(gfx->inst, glfw_win, 0, &(win->srfc));
	
	return win;
}

gfx_cmd_t* gfx_cmd_init(gfx_t* gfx) {
	gfx_cmd_t* cmd = malloc(sizeof(gfx_cmd_t));
	
	VkCommandPoolCreateInfo cmd_poolinfo;
		cmd_poolinfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmd_poolinfo.pNext = 0;
		cmd_poolinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		cmd_poolinfo.queueFamilyIndex = 0;
	vkCreateCommandPool(gfx->devc, &cmd_poolinfo, 0, &(cmd->pool));
	
	VkCommandBufferAllocateInfo cmdinfo;
		cmdinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdinfo.pNext = 0;
		cmdinfo.commandPool = cmd->pool;
		cmdinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdinfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(gfx->devc, &cmdinfo, &(cmd->draw));
	
	return cmd;
}

void gfx_rndr_init(gfx_t* gfx, gfx_win_t* win) {
	VkAttachmentDescription atch[2];
		atch[0].flags = 0;
		atch[0].format = VK_FORMAT_B8G8R8A8_SRGB;
		atch[0].samples = VK_SAMPLE_COUNT_1_BIT;
		atch[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		atch[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		atch[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		atch[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		atch[0].initialLayout = 0;
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
	vkCreateRenderPass(gfx->devc, &rndrinfo, 0, &(win->rndr));
}

void gfx_shdr_init(gfx_t* gfx, gfx_win_t* win, int8_t* pthv, int8_t* pthf) {
	
}

void gfx_swap_init(gfx_t* gfx, gfx_win_t* win) {
	VkSwapchainKHR swap_anc = win->swap;
	VkSwapchainCreateInfoKHR swapinfo;
		swapinfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapinfo.pNext = 0;
		swapinfo.flags = 0;
		swapinfo.surface = win->srfc;
		swapinfo.minImageCount = 3;
		swapinfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
		swapinfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		swapinfo.imageExtent.width = win->w;
		swapinfo.imageExtent.height = win->h;
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
	vkCreateSwapchainKHR(gfx->devc, &swapinfo, 0, &(win->swap));
	
	if (swap_anc != 0) {
		vkDestroySwapchainKHR(gfx->devc, swap_anc, 0);
	}
	
	vkGetSwapchainImagesKHR(gfx->devc, win->swap, &(win->img_n), 0);
	win->swap_img = malloc(sizeof(VkImage) * win->img_n);
	vkGetSwapchainImagesKHR(gfx->devc, win->swap, &(win->img_n), win->swap_img);
	win->swap_img_v = malloc(sizeof(VkImageView) * win->img_n);
	
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
	for (uint32_t i = 0; i < win->img_n; i++) {
		imgvinfo.image = win->swap_img[i];
		vkCreateImageView(gfx->devc, &imgvinfo, 0, &(win->swap_img_v)[i]);
	}
}

void gfx_dpth_init(gfx_t* gfx, gfx_win_t* win) {
	VkImageCreateInfo imginfo;
		imginfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imginfo.pNext = 0;
		imginfo.flags = 0;
		imginfo.imageType = VK_IMAGE_TYPE_2D;
		imginfo.format = VK_FORMAT_D32_SFLOAT;
		imginfo.extent.width = win->w;
		imginfo.extent.height = win->h;
		imginfo.mipLevels = 1;
		imginfo.arrayLayers = 1;
		imginfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imginfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imginfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imginfo.sharingMode = 0;
		imginfo.queueFamilyIndexCount = 0;
		imginfo.pQueueFamilyIndices = 0;
		imginfo.initialLayout = 0;
	vkCreateImage(gfx->devc, &imginfo, 0, &(win->dpth.img));
	
	vkGetImageMemoryRequirements(gfx->devc, win->dpth.img, &(win->dpth.req));
	VkMemoryAllocateInfo meminfo;
		meminfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		meminfo.pNext = 0;
		meminfo.allocationSize = win->dpth.req.size;
		meminfo.memoryTypeIndex = 0;
	vkAllocateMemory(gfx->devc, &meminfo, 0, &(win->dpth.mem));
	vkBindImageMemory(gfx->devc, win->dpth.img, win->dpth.mem, 0);
	
	VkImageViewCreateInfo imgvinfo;
		imgvinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imgvinfo.pNext = 0;
		imgvinfo.flags = 0;
		imgvinfo.image = win->dpth.img;
		imgvinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imgvinfo.format = VK_FORMAT_D32_SFLOAT;
		imgvinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		imgvinfo.subresourceRange.baseMipLevel = 0;
		imgvinfo.subresourceRange.levelCount = 1;
		imgvinfo.subresourceRange.baseArrayLayer = 0;
		imgvinfo.subresourceRange.layerCount = 1;
	vkCreateImageView(gfx->devc, &imgvinfo, 0, &(win->dpth.v));
}

gfx_pipe_t* gfx_pipe_init(gfx_t* gfx, gfx_win_t* win, int8_t* pthv, int8_t* pthf, gfx_vrtx_t* vrtx, gfx_dscr_t* dscr, uint64_t push_sz) {
	gfx_pipe_t* pipe = malloc(sizeof(gfx_pipe_t));
	
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
	vkCreateShaderModule(gfx->devc, &shdinfo, 0, &(pipe->vrtx_shdr));
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
	vkCreateShaderModule(gfx->devc, &shdinfo, 0, &(pipe->frag_shdr));
	free(src);
	
	VkPipelineShaderStageCreateInfo stginfo[2];
		stginfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stginfo[0].pNext = 0;
		stginfo[0].flags = 0;
		stginfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stginfo[0].module = pipe->vrtx_shdr;
		stginfo[0].pName = "main";
		stginfo[0].pSpecializationInfo = 0;
		stginfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stginfo[1].pNext = 0;
		stginfo[1].flags = 0;
		stginfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stginfo[1].module = pipe->frag_shdr;
		stginfo[1].pName = "main";
		stginfo[1].pSpecializationInfo = 0;
	
	VkPipelineInputAssemblyStateCreateInfo inasminfo;
		inasminfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inasminfo.pNext = 0;
		inasminfo.flags = 0;
		inasminfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		inasminfo.primitiveRestartEnable = 1;
		
	VkViewport vprt;
		vprt.x = 0.f;
		vprt.y = 0.f;
		vprt.width = (float) win->w;
		vprt.height = (float) win->h;
		vprt.minDepth = 0.f;
		vprt.maxDepth = 1.f;
		
	VkRect2D scsr;
		scsr.offset.x = 0;
		scsr.offset.y = 0;
		scsr.extent.width = win->w;
		scsr.extent.height = win->h;
		
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
		
	VkPipelineDepthStencilStateCreateInfo dpthinfo;
		dpthinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		dpthinfo.pNext = 0;
		dpthinfo.flags = 0;
		dpthinfo.depthTestEnable = 1;
		dpthinfo.depthWriteEnable = 1;
		dpthinfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		dpthinfo.depthBoundsTestEnable = 0;
		dpthinfo.stencilTestEnable = 0;
		dpthinfo.front.failOp = 0;
		dpthinfo.front.passOp = 0;
		dpthinfo.front.depthFailOp = 0;
		dpthinfo.front.compareOp = 0;
		dpthinfo.front.compareMask = 0;
		dpthinfo.front.writeMask = 0;
		dpthinfo.front.reference = 0;
		dpthinfo.back.failOp = 0;
		dpthinfo.back.passOp = 0;
		dpthinfo.back.depthFailOp = 0;
		dpthinfo.back.compareOp = 0;
		dpthinfo.back.compareMask = 0;
		dpthinfo.back.writeMask = 0;
		dpthinfo.back.reference = 0;
		dpthinfo.minDepthBounds = 0.f;
		dpthinfo.maxDepthBounds = 1.f;
		
	VkPipelineColorBlendAttachmentState colblndatch;
		colblndatch.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colblndatch.blendEnable = 1;
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
		
	VkDynamicState dyn[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dyninfo;
		dyninfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dyninfo.pNext = 0;
		dyninfo.flags = 0;
		dyninfo.dynamicStateCount = 2;
		dyninfo.pDynamicStates = dyn;
		
	VkPushConstantRange pushrng;
		pushrng.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushrng.offset = 0;
		pushrng.size = push_sz;
	
	VkPipelineLayoutCreateInfo pipelaytinfo;
		pipelaytinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelaytinfo.pNext = 0;
		pipelaytinfo.flags = 0;
		pipelaytinfo.setLayoutCount = 0;
		pipelaytinfo.pSetLayouts = 0;
	if (dscr != 0) {
		pipelaytinfo.setLayoutCount = dscr->n;
		pipelaytinfo.pSetLayouts = dscr->layt;
	}
		pipelaytinfo.pushConstantRangeCount = 1;
		pipelaytinfo.pPushConstantRanges = &pushrng;
	vkCreatePipelineLayout(gfx->devc, &pipelaytinfo, 0, &(pipe->layt));
	
	VkGraphicsPipelineCreateInfo pipeinfo;
		pipeinfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeinfo.pNext = 0;
		pipeinfo.flags = 0;
		pipeinfo.stageCount = 2;
		pipeinfo.pStages = stginfo;
		pipeinfo.pVertexInputState = &(vrtx->in);
		pipeinfo.pInputAssemblyState = &inasminfo;
		pipeinfo.pTessellationState = 0;
		pipeinfo.pViewportState = &vprtinfo;
		pipeinfo.pRasterizationState = &rstrinfo;
		pipeinfo.pMultisampleState = &multinfo;
		pipeinfo.pDepthStencilState = &dpthinfo;
		pipeinfo.pColorBlendState = &colblndinfo;
		pipeinfo.pDynamicState = &dyninfo;
		pipeinfo.layout = pipe->layt;
		pipeinfo.renderPass = win->rndr;
		pipeinfo.subpass = 0;
		pipeinfo.basePipelineHandle = 0;
		pipeinfo.basePipelineIndex = 0;
	vkCreateGraphicsPipelines(gfx->devc, 0, 1, &pipeinfo, 0, &(pipe->pipe));
	
	return pipe;
}

void gfx_frme_init(gfx_t* gfx, gfx_win_t* win) {
	VkImageView atch[2];
	atch[1] = win->dpth.v;
	VkFramebufferCreateInfo fbfrinfo;
		fbfrinfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbfrinfo.pNext = 0;
		fbfrinfo.flags = 0;
		fbfrinfo.renderPass = win->rndr;
		fbfrinfo.attachmentCount = 2;
		fbfrinfo.pAttachments = atch;
		fbfrinfo.width = win->w;
		fbfrinfo.height = win->h;
		fbfrinfo.layers = 1;
		win->frme = malloc(sizeof(VkFramebuffer) * win->img_n);
	for (uint32_t i = 0; i < win->img_n; i++) {
		atch[0] = win->swap_img_v[i];
		vkCreateFramebuffer(gfx->devc, &fbfrinfo, 0, &(win->frme)[i]);
	}
}

void gfx_bfr_rfsh(gfx_t* gfx, gfx_bfr_t* bfr, void* data, uint64_t sz) {
	void* memdata;
	vkMapMemory(gfx->devc, bfr->mem, 0, bfr->req.size, 0, &memdata);
	memcpy(memdata, data, sz);
	vkUnmapMemory(gfx->devc, bfr->mem);
	vkBindBufferMemory(gfx->devc, bfr->bfr, bfr->mem, 0);
}

gfx_vrtx_t* gfx_vrtx_init(gfx_t* gfx, uint32_t b, uint32_t a, uint64_t sz) {
	gfx_vrtx_t* vrtx = malloc(sizeof(gfx_vrtx_t));
	vrtx->bfr = malloc(sizeof(VkBuffer) * b);
	vrtx->mem = malloc(sizeof(VkDeviceMemory) * b);
	vrtx->req = malloc(sizeof(VkMemoryRequirements) * b);
	vrtx->bind = malloc(sizeof(VkVertexInputBindingDescription) * b);
	vrtx->b = b;
	vrtx->attr = malloc(sizeof(VkVertexInputAttributeDescription) * a);
	vrtx->a = a;
	
	for (uint32_t i = 0; i < b; i++) {
		VkBufferCreateInfo bfrinfo;
			bfrinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bfrinfo.pNext = 0;
			bfrinfo.flags = 0;
			bfrinfo.size = sz;
			bfrinfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			bfrinfo.sharingMode = 0;
			bfrinfo.queueFamilyIndexCount = 0;
			bfrinfo.pQueueFamilyIndices = 0;
		vkCreateBuffer(gfx->devc, &bfrinfo, 0, &(vrtx->bfr[i]));
	
		vkGetBufferMemoryRequirements(gfx->devc, vrtx->bfr[i], &(vrtx->req[i]));
		VkMemoryAllocateInfo meminfo;
			meminfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			meminfo.pNext = 0;
			meminfo.allocationSize = vrtx->req[i].size;
			meminfo.memoryTypeIndex = 0;
		vkAllocateMemory(gfx->devc, &meminfo, 0, &(vrtx->mem[i]));
	}
	return vrtx;
}

void gfx_vrtx_bind(gfx_vrtx_t* vrtx, uint32_t b, uint32_t s) {
	vrtx->bind[b].binding = b;
	vrtx->bind[b].stride = s;
	vrtx->bind[b].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

void gfx_vrtx_attr(gfx_vrtx_t* vrtx, uint32_t l, uint32_t b, int8_t sz, uint32_t off) {
	vrtx->attr[l].location = l;
	vrtx->attr[l].binding = b;
	if (sz == -4) vrtx->attr[l].format = VK_FORMAT_R32_SINT;
	else if (sz == -8) vrtx->attr[l].format = VK_FORMAT_R32G32_SINT;
	else if (sz == -12) vrtx->attr[l].format = VK_FORMAT_R32G32B32_SINT;
	else if (sz == -16) vrtx->attr[l].format = VK_FORMAT_R32G32B32A32_SINT;
	else if (sz == 4) vrtx->attr[l].format = VK_FORMAT_R32_UINT;
	else if (sz == 8) vrtx->attr[l].format = VK_FORMAT_R32G32_UINT;
	else if (sz == 12) vrtx->attr[l].format = VK_FORMAT_R32G32B32_UINT;
	else if (sz == 16) vrtx->attr[l].format = VK_FORMAT_R32G32B32A32_UINT;
	vrtx->attr[l].offset = off;
}

void gfx_vrtx_in(gfx_vrtx_t* vrtx) {
	vrtx->in.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vrtx->in.pNext = 0;
	vrtx->in.flags = 0;
	vrtx->in.vertexBindingDescriptionCount = vrtx->b;
	vrtx->in.pVertexBindingDescriptions = vrtx->bind;
	vrtx->in.vertexAttributeDescriptionCount = vrtx->a;
	vrtx->in.pVertexAttributeDescriptions = vrtx->attr;
}

void gfx_vrtx_rfsh(gfx_t* gfx, gfx_vrtx_t* vrtx, uint32_t b, void* data, uint64_t sz) {
	void* memdata;
	vkMapMemory(gfx->devc, vrtx->mem[b], 0, vrtx->req[b].size, 0, &memdata);
	memcpy(memdata, data, sz);
	vkUnmapMemory(gfx->devc, vrtx->mem[b]);
	vkBindBufferMemory(gfx->devc, vrtx->bfr[b], vrtx->mem[b], 0);
}

gfx_bfr_t* gfx_indx_init(gfx_t* gfx, uint64_t sz) {
	gfx_bfr_t* indx = malloc(sizeof(gfx_bfr_t));

	VkBufferCreateInfo bfrinfo;
		bfrinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bfrinfo.pNext = 0;
		bfrinfo.flags = 0;
		bfrinfo.size = sz;
		bfrinfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		bfrinfo.sharingMode = 0;
		bfrinfo.queueFamilyIndexCount = 0;
		bfrinfo.pQueueFamilyIndices = 0;
	vkCreateBuffer(gfx->devc, &bfrinfo, 0, &(indx->bfr));
	
	vkGetBufferMemoryRequirements(gfx->devc, indx->bfr, &(indx->req));
	VkMemoryAllocateInfo meminfo;
		meminfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		meminfo.pNext = 0;
		meminfo.allocationSize = indx->req.size;
		meminfo.memoryTypeIndex = 0;
	vkAllocateMemory(gfx->devc, &meminfo, 0, &(indx->mem));
	
	return indx;
}

gfx_bfr_t* gfx_unif_init(gfx_t* gfx, uint64_t sz) {
	gfx_bfr_t* unif = malloc(sizeof(gfx_bfr_t));

	VkBufferCreateInfo bfrinfo;
		bfrinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bfrinfo.pNext = 0;
		bfrinfo.flags = 0;
		bfrinfo.size = sz;
		bfrinfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bfrinfo.sharingMode = 0;
		bfrinfo.queueFamilyIndexCount = 0;
		bfrinfo.pQueueFamilyIndices = 0;
	vkCreateBuffer(gfx->devc, &bfrinfo, 0, &(unif->bfr));
	
	vkGetBufferMemoryRequirements(gfx->devc, unif->bfr, &(unif->req));
	VkMemoryAllocateInfo meminfo;
		meminfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		meminfo.pNext = 0;
		meminfo.allocationSize = unif->req.size;
		meminfo.memoryTypeIndex = 0;
	vkAllocateMemory(gfx->devc, &meminfo, 0, &(unif->mem));
	
	return unif;
}

gfx_txtr_t* gfx_txtr_init(gfx_t* gfx, gfx_cmd_t* cmd, uint8_t* pix, uint32_t w, uint32_t h) {
	gfx_txtr_t* txtr = malloc(sizeof(gfx_txtr_t));
	gfx_bfr_t bfr;

	VkBufferCreateInfo bfrinfo;
		bfrinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bfrinfo.pNext = 0;
		bfrinfo.flags = 0;
		bfrinfo.size = w * h * 4;
		bfrinfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bfrinfo.sharingMode = 0;
		bfrinfo.queueFamilyIndexCount = 0;
		bfrinfo.pQueueFamilyIndices = 0;
	vkCreateBuffer(gfx->devc, &bfrinfo, 0, &(bfr.bfr));
	
	vkGetBufferMemoryRequirements(gfx->devc, bfr.bfr, &(bfr.req));
	VkMemoryAllocateInfo meminfo;
		meminfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		meminfo.pNext = 0;
		meminfo.allocationSize = bfr.req.size;
		meminfo.memoryTypeIndex = 0;
	vkAllocateMemory(gfx->devc, &meminfo, 0, &(bfr.mem));
	
	gfx_bfr_rfsh(gfx, &(bfr), pix, w * h * 4);
	
	VkImageCreateInfo imginfo;
		imginfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imginfo.pNext = 0;
		imginfo.flags = 0;
		imginfo.imageType = VK_IMAGE_TYPE_2D;
		imginfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imginfo.extent.width = w;
		imginfo.extent.height = h;
		imginfo.extent.depth = 1;
		imginfo.mipLevels = 1;
		imginfo.arrayLayers = 1;
		imginfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imginfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imginfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imginfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imginfo.queueFamilyIndexCount = 0;
		imginfo.pQueueFamilyIndices = 0;
		imginfo.initialLayout = 0;
	vkCreateImage(gfx->devc, &imginfo, 0, &(txtr->img.img));
	
	vkGetImageMemoryRequirements(gfx->devc, txtr->img.img, &(txtr->img.req));
		meminfo.allocationSize = txtr->img.req.size;
	vkAllocateMemory(gfx->devc, &meminfo, 0, &(txtr->img.mem));
	vkBindImageMemory(gfx->devc, txtr->img.img, txtr->img.mem, 0);
	
	VkCommandBuffer cmd_txtr;
	VkCommandBufferAllocateInfo cmdinfo;
		cmdinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdinfo.pNext = 0;
		cmdinfo.commandPool = cmd->pool;
		cmdinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdinfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(gfx->devc, &cmdinfo, &cmd_txtr);
	
	VkCommandBufferBeginInfo cbfrinfo;
		cbfrinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cbfrinfo.pNext = 0;
		cbfrinfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		cbfrinfo.pInheritanceInfo = 0;
	vkBeginCommandBuffer(cmd_txtr, &cbfrinfo);
	
	VkImageMemoryBarrier imgmembar;
		imgmembar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imgmembar.pNext = 0;
		imgmembar.srcAccessMask = 0;
		imgmembar.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imgmembar.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imgmembar.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imgmembar.srcQueueFamilyIndex = 0;
		imgmembar.dstQueueFamilyIndex = 0;
		imgmembar.image = txtr->img.img;
		imgmembar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgmembar.subresourceRange.baseMipLevel = 0;
		imgmembar.subresourceRange.levelCount = 1;
		imgmembar.subresourceRange.baseArrayLayer = 0;
		imgmembar.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(cmd_txtr, 0, 0, 0, 0, 0, 0, 0, 1, &imgmembar);
	
	VkBufferImageCopy cp;
		cp.bufferOffset = 0;
		cp.bufferRowLength = 0;
		cp.bufferImageHeight = 0;
		cp.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		cp.imageSubresource.mipLevel = 0;
		cp.imageSubresource.baseArrayLayer = 0;
		cp.imageSubresource.layerCount = 1;
		cp.imageOffset.x = 0;
		cp.imageOffset.y = 0;
		cp.imageOffset.z = 0;
		cp.imageExtent.width = w;
		cp.imageExtent.height = h;
		cp.imageExtent.depth = 1;
	vkCmdCopyBufferToImage(cmd_txtr, bfr.bfr, txtr->img.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cp);
	
	vkEndCommandBuffer(cmd_txtr);
	
	VkSubmitInfo sbmtinfo;
		sbmtinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		sbmtinfo.pNext = 0;
		sbmtinfo.waitSemaphoreCount = 0;
		sbmtinfo.pWaitSemaphores = 0;
		sbmtinfo.pWaitDstStageMask = 0;
		sbmtinfo.commandBufferCount = 1;
		sbmtinfo.pCommandBuffers = &cmd_txtr;
		sbmtinfo.signalSemaphoreCount = 0;
		sbmtinfo.pSignalSemaphores = 0;
	vkQueueSubmit(gfx->que, 1, &sbmtinfo, gfx->fnc);
	
	vkWaitForFences(gfx->devc, 1, &(gfx->fnc), 1, UINT64_MAX);
	vkResetFences(gfx->devc, 1, &(gfx->fnc));
	
	vkFreeCommandBuffers(gfx->devc, cmd->pool, 1, &cmd_txtr);
	
	VkImageViewCreateInfo imgvinfo;
		imgvinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imgvinfo.pNext = 0;
		imgvinfo.flags = 0;
		imgvinfo.image = txtr->img.img;
		imgvinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imgvinfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imgvinfo.components.r = 0;
		imgvinfo.components.g = 0;
		imgvinfo.components.b = 0;
		imgvinfo.components.a = 0;
		imgvinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgvinfo.subresourceRange.baseMipLevel = 0;
		imgvinfo.subresourceRange.levelCount = 1;
		imgvinfo.subresourceRange.baseArrayLayer = 0;
		imgvinfo.subresourceRange.layerCount = 1;
	vkCreateImageView(gfx->devc, &imgvinfo, 0, &(txtr->img.v));
	
	VkSamplerCreateInfo smplinfo;
		smplinfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		smplinfo.pNext = 0;
		smplinfo.flags = 0;
		smplinfo.magFilter = VK_FILTER_LINEAR;
		smplinfo.minFilter = VK_FILTER_LINEAR;
		smplinfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		smplinfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		smplinfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		smplinfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		smplinfo.mipLodBias = 0.f;
		smplinfo.anisotropyEnable = 1;
		smplinfo.maxAnisotropy = 0;
		smplinfo.compareEnable = 0;
		smplinfo.compareOp = VK_COMPARE_OP_ALWAYS;
		smplinfo.minLod = 0.f;
		smplinfo.maxLod = 0.f;
		smplinfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		smplinfo.unnormalizedCoordinates = 0;
	vkCreateSampler(gfx->devc, &smplinfo, 0, &(txtr->smpl));
	
	vkDestroyBuffer(gfx->devc, bfr.bfr, 0);
	vkFreeMemory(gfx->devc, bfr.mem, 0);
	
	return txtr;
}

gfx_dscr_t* gfx_dscr_init(gfx_t* gfx, uint32_t n) {
	gfx_dscr_t* dscr = malloc(sizeof(gfx_dscr_t));
	dscr->set = malloc(sizeof(VkDescriptorSet) * n);
	dscr->layt = malloc(sizeof(VkDescriptorSetLayout) * n);
	dscr->n = n;
	
	VkDescriptorPoolSize poolsz[2];
		poolsz[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolsz[0].descriptorCount = n;
		poolsz[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolsz[1].descriptorCount = n;
	VkDescriptorPoolCreateInfo poolinfo;
		poolinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolinfo.pNext = 0;
		poolinfo.flags = 0;
		poolinfo.maxSets = n;
		poolinfo.poolSizeCount = 2;
		poolinfo.pPoolSizes = poolsz;
	vkCreateDescriptorPool(gfx->devc, &poolinfo, 0, &(dscr->pool));
	
	VkDescriptorSetLayoutBinding bind[2];
		bind[0].binding = 0;
		bind[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bind[0].descriptorCount = 1;
		bind[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		bind[0].pImmutableSamplers = 0;
		bind[1].binding = 1;
		bind[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bind[1].descriptorCount = 1;
		bind[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		bind[1].pImmutableSamplers = 0;
	
	VkDescriptorSetLayoutCreateInfo laytinfo;
		laytinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		laytinfo.pNext = 0;
		laytinfo.flags = 0;
		laytinfo.bindingCount = 2;
		laytinfo.pBindings = bind;
	for (uint32_t i = 0; i < n; i++) {
		vkCreateDescriptorSetLayout(gfx->devc, &laytinfo, 0, &(dscr->layt[i]));
	}
	VkDescriptorSetAllocateInfo setalc;
		setalc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		setalc.pNext = 0;
		setalc.descriptorPool = dscr->pool;
		setalc.descriptorSetCount = n;
		setalc.pSetLayouts = dscr->layt;
	vkAllocateDescriptorSets(gfx->devc, &setalc, dscr->set);
	
	return dscr;
}

void gfx_dscr_writ(gfx_t* gfx, gfx_dscr_t* dscr, uint32_t i, gfx_bfr_t* unif, void* data, uint64_t sz, gfx_txtr_t* txtr) {
	VkWriteDescriptorSet writ[2];
	uint8_t n = 0;
	
	VkDescriptorBufferInfo bfr;
	if (unif != 0) {
		bfr.buffer = unif->bfr;
		bfr.offset = 0;
		bfr.range = sz;
		gfx_bfr_rfsh(gfx, unif, data, sz);
		
		writ[n].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writ[n].pNext = 0;
		writ[n].dstSet = dscr->set[i];
		writ[n].dstBinding = 0;
		writ[n].dstArrayElement = 0;
		writ[n].descriptorCount = 1;
		writ[n].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writ[n].pImageInfo = 0;
		writ[n].pBufferInfo = &bfr;
		writ[n].pTexelBufferView = 0;
		n++;
	}
	
	VkDescriptorImageInfo img;
	if (txtr != 0) {
		img.sampler = txtr->smpl;
		img.imageView = txtr->img.v;
		img.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		writ[n].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writ[n].pNext = 0;
		writ[n].dstSet = dscr->set[i];
		writ[n].dstBinding = 1;
		writ[n].dstArrayElement = 0;
		writ[n].descriptorCount = 1;
		writ[n].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writ[n].pImageInfo = &img;
		writ[n].pBufferInfo = 0;
		writ[n].pTexelBufferView = 0;
		n++;
	}
	
	vkUpdateDescriptorSets(gfx->devc, n, writ, 0, 0);
}

void gfx_clr(gfx_win_t* win, uint8_t r, uint8_t g, uint8_t b) {
	win->clr[0].color.float32[0] = (float) r / 255;
	win->clr[0].color.float32[1] = (float) g / 255;
	win->clr[0].color.float32[2] = (float) b / 255;
	win->clr[0].color.float32[3] = 1.f;
	win->clr[1].depthStencil.depth = 1.f;
	win->clr[1].depthStencil.stencil = 0;
}

void gfx_draw(gfx_t* gfx, gfx_win_t* win, gfx_pipe_t* pipe, gfx_cmd_t* cmd, gfx_bfr_t* indx, gfx_vrtx_t* vrtx, gfx_dscr_t* dscr, void* push, uint64_t push_sz, uint32_t n, uint32_t indx_off, uint32_t vrtx_off) {
	vkAcquireNextImageKHR(gfx->devc, win->swap, UINT64_MAX, gfx->smph_img, 0, &(win->img_i));
	
	VkCommandBufferBeginInfo cbfrinfo;
		cbfrinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cbfrinfo.pNext = 0;
		cbfrinfo.flags = 0;
		cbfrinfo.pInheritanceInfo = 0;
	vkBeginCommandBuffer(cmd->draw, &cbfrinfo);
	
	VkImageMemoryBarrier imgmembar;
		imgmembar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imgmembar.pNext = 0;
		imgmembar.srcAccessMask = 0;
		imgmembar.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imgmembar.oldLayout = 0;
		imgmembar.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imgmembar.srcQueueFamilyIndex = 0;
		imgmembar.dstQueueFamilyIndex = 0;
		imgmembar.image = win->swap_img[win->img_i];
		imgmembar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgmembar.subresourceRange.baseMipLevel = 0;
		imgmembar.subresourceRange.levelCount = 1;
		imgmembar.subresourceRange.baseArrayLayer = 0;
		imgmembar.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(cmd->draw, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, 0, 0, 0, 1, &imgmembar);
	
	VkRenderPassBeginInfo rndrinfo;
		rndrinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rndrinfo.pNext = 0;
		rndrinfo.renderPass = win->rndr;
		rndrinfo.framebuffer = win->frme[win->img_i];
		rndrinfo.renderArea.offset.x = 0;
		rndrinfo.renderArea.offset.y = 0;
		rndrinfo.renderArea.extent.width = win->w;
		rndrinfo.renderArea.extent.height = win->h;
		rndrinfo.clearValueCount = 2;
		rndrinfo.pClearValues = win->clr;
	vkCmdBeginRenderPass(cmd->draw, &rndrinfo, VK_SUBPASS_CONTENTS_INLINE);
	
	vkCmdBindPipeline(cmd->draw, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->pipe);
	
	VkViewport vprt;
		vprt.x = 0.f;
		vprt.y = 0.f;
		vprt.width = (float) win->w;
		vprt.height = (float) win->h;
		vprt.minDepth = 0.f;
		vprt.maxDepth = 1.f;
	vkCmdSetViewport(cmd->draw, 0, 1, &vprt);
		
	VkRect2D scsr;
		scsr.offset.x = 0;
		scsr.offset.y = 0;
		scsr.extent.width = win->w;
		scsr.extent.height = win->h;
	vkCmdSetScissor(cmd->draw, 0, 1, &scsr);
	
	VkDeviceSize offset = {0};
	vkCmdBindVertexBuffers(cmd->draw, 0, vrtx->b, vrtx->bfr, &offset);
	vkCmdBindIndexBuffer(cmd->draw, indx->bfr, 0, VK_INDEX_TYPE_UINT32);
	if (dscr != 0) vkCmdBindDescriptorSets(cmd->draw, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->layt, 0, dscr->n, dscr->set, 0, 0);
	vkCmdPushConstants(cmd->draw, pipe->layt, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, push_sz, push);
	vkCmdDrawIndexed(cmd->draw, n, 1, indx_off, vrtx_off, 0);
	
	vkCmdEndRenderPass(cmd->draw);
	
		imgmembar.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imgmembar.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		imgmembar.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imgmembar.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	vkCmdPipelineBarrier(cmd->draw, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, 0, 0, 0, 1, &imgmembar);
	
	vkEndCommandBuffer(cmd->draw);
	
	VkPipelineStageFlags pipeflag = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkSubmitInfo sbmtinfo;
		sbmtinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		sbmtinfo.pNext = 0;
		sbmtinfo.waitSemaphoreCount = 1;
		sbmtinfo.pWaitSemaphores = &gfx->smph_img;
		sbmtinfo.pWaitDstStageMask = &pipeflag;
		sbmtinfo.commandBufferCount = 1;
		sbmtinfo.pCommandBuffers = &(cmd->draw);
		sbmtinfo.signalSemaphoreCount = 1;
		sbmtinfo.pSignalSemaphores = &gfx->smph_drw;
	vkQueueSubmit(gfx->que, 1, &sbmtinfo, gfx->fnc);
	
	VkPresentInfoKHR preinfo;
		preinfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		preinfo.pNext = 0;
		preinfo.waitSemaphoreCount = 1;
		preinfo.pWaitSemaphores = &gfx->smph_drw;
		preinfo.swapchainCount = 1;
		preinfo.pSwapchains = &(win->swap);
		preinfo.pImageIndices = &(win->img_i);
		preinfo.pResults = 0;
	vkQueuePresentKHR(gfx->que, &preinfo);
	
	vkWaitForFences(gfx->devc, 1, &(gfx->fnc), 1, UINT64_MAX);
	vkResetFences(gfx->devc, 1, &(gfx->fnc));
}

void gfx_resz(gfx_t* gfx, gfx_win_t* win, uint32_t w, uint32_t h) {
	for (uint32_t i = 0; i < win->img_n; i++) {
		vkDestroyFramebuffer(gfx->devc, win->frme[i], 0);
	}
	free(win->frme);
	
	vkDestroyImageView(gfx->devc, win->dpth.v, 0);
	vkDestroyImage(gfx->devc, win->dpth.img, 0);
	vkFreeMemory(gfx->devc, win->dpth.mem, 0);
	
	for (uint32_t i = 0; i < win->img_n; i++) {
		vkDestroyImageView(gfx->devc, win->swap_img_v[i], 0);
	}
	free(win->swap_img);
	free(win->swap_img_v);
	
	win->w = w;
	win->h = h;
	
	gfx_swap_init(gfx, win);
	gfx_dpth_init(gfx, win);
	gfx_frme_init(gfx, win);
}

void gfx_bfr_free(gfx_t* gfx, gfx_bfr_t* bfr) {
	vkDestroyBuffer(gfx->devc, bfr->bfr, 0);
	vkFreeMemory(gfx->devc, bfr->mem, 0);
	free(bfr);
}

void gfx_img_free(gfx_t* gfx, gfx_img_t* img) {
	vkDestroyImageView(gfx->devc, img->v, 0);
	vkDestroyImage(gfx->devc, img->img, 0);
	vkFreeMemory(gfx->devc, img->mem, 0);
	free(img);
}

void gfx_vrtx_free(gfx_t* gfx, gfx_vrtx_t* vrtx) {
	for (uint32_t i = 0; i < vrtx->b; i++) {
		vkDestroyBuffer(gfx->devc, vrtx->bfr[i], 0);
		vkFreeMemory(gfx->devc, vrtx->mem[i], 0);
	}
	free(vrtx->bfr);
	free(vrtx->mem);
	free(vrtx->req);
	free(vrtx->bind);
	free(vrtx->attr);
	free(vrtx);
}

void gfx_txtr_free(gfx_t* gfx, gfx_txtr_t* txtr) {
	vkDestroyImageView(gfx->devc, txtr->img.v, 0);
	vkDestroyImage(gfx->devc, txtr->img.img, 0);
	vkFreeMemory(gfx->devc, txtr->img.mem, 0);
	vkDestroySampler(gfx->devc, txtr->smpl, 0);
	
	free(txtr);
}

void gfx_dscr_free(gfx_t* gfx, gfx_dscr_t* dscr) {
	vkFreeDescriptorSets(gfx->devc, dscr->pool, dscr->n, dscr->set);
	for (uint32_t i = 0; i < dscr->n; i++) {
		vkDestroyDescriptorSetLayout(gfx->devc, dscr->layt[i], 0);
	}
	vkDestroyDescriptorPool(gfx->devc, dscr->pool, 0);
	
	free(dscr->set);
	free(dscr->layt);
	free(dscr);
}

void gfx_pipe_free(gfx_t* gfx, gfx_pipe_t* pipe) {
	vkDestroyPipeline(gfx->devc, pipe->pipe, 0);
	vkDestroyPipelineLayout(gfx->devc, pipe->layt, 0);
	vkDestroyShaderModule(gfx->devc, pipe->vrtx_shdr, 0);
	vkDestroyShaderModule(gfx->devc, pipe->frag_shdr, 0);
	free(pipe);
}

void gfx_cmd_free(gfx_t* gfx, gfx_cmd_t* cmd) {
	vkFreeCommandBuffers(gfx->devc, cmd->pool, 1, &(cmd->draw));
	vkDestroyCommandPool(gfx->devc, cmd->pool, 0);
	free(cmd);
}

void gfx_win_free(gfx_t* gfx, gfx_win_t* win) {
	for (uint32_t i = 0; i < win->img_n; i++) {
		vkDestroyFramebuffer(gfx->devc, win->frme[i], 0);
	}
	free(win->frme);
	
	vkDestroyImageView(gfx->devc, win->dpth.v, 0);
	vkDestroyImage(gfx->devc, win->dpth.img, 0);
	vkFreeMemory(gfx->devc, win->dpth.mem, 0);
	
	for (uint32_t i = 0; i < win->img_n; i++) {
		vkDestroyImageView(gfx->devc, win->swap_img_v[i], 0);
	}
	free(win->swap_img);
	free(win->swap_img_v);
	vkDestroySwapchainKHR(gfx->devc, win->swap, 0);
	
	vkDestroyRenderPass(gfx->devc, win->rndr, 0);
	
	vkDestroySurfaceKHR(gfx->inst, win->srfc, 0);
	free(win);
}

void gfx_free(gfx_t* gfx) {
	vkDestroySemaphore(gfx->devc, gfx->smph_img, 0);
	vkDestroySemaphore(gfx->devc, gfx->smph_drw, 0);
	vkDestroyFence(gfx->devc, gfx->fnc, 0);
	
	vkDestroyDevice(gfx->devc, 0);
	vkDestroyInstance(gfx->inst, 0);
	free(gfx);
}
