#pragma once

#include "core/color.h"

#include "renderer/types.h"

std::vector<uint32_t> get_bundled_spirv_data(const char* file_path);

struct MaterialInstance {
	Pipeline pipeline;
	Shader shader;
	UniformSet uniform_set;
};

struct MaterialConstants {
	Color diffuse_factor = COLOR_WHITE;
	// This will effect the specular component.
	// The higher it is shinier the material
	float metallic_factor = 0.0f;
	float shininess_factor = 1.0f;

private:
	float __padding[18];
};

struct MaterialResources {
	MaterialConstants constants;
	Image diffuse_image = GL_NULL_HANDLE;
	Image specular_image = GL_NULL_HANDLE;
	Sampler sampler = GL_NULL_HANDLE;
};

struct GL_API Material {
	Pipeline pipeline;
	Shader shader;

	static Ref<Material> create();

	static void destroy(Ref<Material> p_material);

	Ref<MaterialInstance> create_instance(const MaterialResources& resources);

private:
	std::vector<Buffer> allocated_buffers;
};
