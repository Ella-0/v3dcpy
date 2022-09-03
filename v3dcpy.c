#include <string.h>
#include <stdio.h>
#include <vulkan/vulkan.h>

#define VK(f) \
do { \
    VkResult res = vk##f; \
    if (res < VK_SUCCESS) \
		fprintf(stderr, "%d: %s\n", res, "vk" #f); \
} while (0)

int main() {
	struct vk {
		VkInstance instance;
		VkPhysicalDevice pdev;
		VkDevice dev;
		VkQueue queue;
		VkCommandPool cmd_pool;
		VkCommandBuffer cmd_buffer;

		VkImage src;
		VkDeviceMemory src_mem;
		VkImage dst;
		VkDeviceMemory dst_mem;
	} vk;

	VK(CreateInstance(
		&(VkInstanceCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.enabledLayerCount = 1,
			.ppEnabledLayerNames = (char const *[]) {"VK_LAYER_KHRONOS_validation"}
		},
		NULL,
		&vk.instance
	));

	VK(EnumeratePhysicalDevices(vk.instance, (uint32_t[]) {1}, &vk.pdev));

	VK(CreateDevice(
		vk.pdev,
		&(VkDeviceCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = &(VkDeviceQueueCreateInfo) {
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = 0,
				.queueCount = 1,
				.pQueuePriorities = (float[]) {1.0f}
			}
		},
		NULL,
		&vk.dev
	));

	vkGetDeviceQueue(vk.dev, 0, 0, &vk.queue);

	VK(CreateCommandPool(
		vk.dev,
		&(VkCommandPoolCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO
		},
		NULL,
		&vk.cmd_pool
	));

	VK(AllocateCommandBuffers(
		vk.dev,
		&(VkCommandBufferAllocateInfo) {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = vk.cmd_pool,
			.commandBufferCount = 1
		},
		&vk.cmd_buffer
	));


	VK(CreateImage(
		vk.dev,
		&(VkImageCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = VK_FORMAT_R8G8B8A8_UNORM,
			.extent = {
				.width = 64,
				.height = 64,
				.depth = 1
			},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_LINEAR,
			.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 1,
			.pQueueFamilyIndices = (uint32_t[]) {0},
			.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED
		},
		NULL,
		&vk.src
	));

	VK(CreateImage(
		vk.dev,
		&(VkImageCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = VK_FORMAT_R8G8B8A8_UNORM,
			.extent = {
				.width = 64,
				.height = 64,
				.depth = 1
			},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 1,
			.pQueueFamilyIndices = (uint32_t[]) {0},
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		},
		NULL,
		&vk.dst
	));

	VkMemoryRequirements src_reqs;

	vkGetImageMemoryRequirements(vk.dev, vk.src, &src_reqs);

	VK(AllocateMemory(
		vk.dev,
		&(VkMemoryAllocateInfo) {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = src_reqs.size
		},
		NULL,
		&vk.src_mem
	));

	VkMemoryRequirements dst_reqs;

	vkGetImageMemoryRequirements(vk.dev, vk.dst, &dst_reqs);

	VK(AllocateMemory(
		vk.dev,
		&(VkMemoryAllocateInfo) {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = dst_reqs.size
		},
		NULL,
		&vk.dst_mem
	));

	uint32_t pixels[64 * 64];
	for (size_t i = 0; i < 64 * 64; i++)
		pixels[i] = i;
	void *data;
	VK(MapMemory(vk.dev, vk.src_mem, 0, src_reqs.size, 0, &data));
		memcpy(data, pixels, 64 * 64 * 4);
	vkUnmapMemory(vk.dev, vk.src_mem);

	VK(BindImageMemory(vk.dev, vk.src, vk.src_mem, 0));
	VK(BindImageMemory(vk.dev, vk.dst, vk.dst_mem, 0));

	VK(BeginCommandBuffer(
		vk.cmd_buffer,
		&(VkCommandBufferBeginInfo) {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		}
	));

	vkCmdCopyImage(
		vk.cmd_buffer,
		vk.src,
		VK_IMAGE_LAYOUT_GENERAL,
		vk.dst,
		VK_IMAGE_LAYOUT_GENERAL,
		1,
		&(VkImageCopy) {
			.srcSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.layerCount = 1
			},
			.dstSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.layerCount = 1
			},
			.extent = {
				.width = 64,
				.height = 64,
				.depth = 1
			},
		}
	);

	VK(EndCommandBuffer(vk.cmd_buffer));

	vkFreeCommandBuffers(vk.dev, vk.cmd_pool, 1, &vk.cmd_buffer);

	vkDestroyCommandPool(vk.dev, vk.cmd_pool, NULL);

	vkFreeMemory(vk.dev, vk.src_mem, NULL);
	vkFreeMemory(vk.dev, vk.dst_mem, NULL);

	vkDestroyImage(vk.dev, vk.src, NULL);
	vkDestroyImage(vk.dev, vk.dst, NULL);

	vkQueueWaitIdle(vk.queue);

	vkDeviceWaitIdle(vk.dev);
	vkDestroyDevice(vk.dev, NULL);

	vkDestroyInstance(vk.instance, NULL);
}
