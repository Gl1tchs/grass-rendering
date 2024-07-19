#pragma once

#include <core/timer.h>
#include <renderer/camera.h>
#include <renderer/model.h>
#include <renderer/render_backend.h>
#include <renderer/renderer.h>

struct Grass {
	Vec3f position;
	float tilt;
	Vec2f facing; // x, z
	float size;
	float bend;
};

class GrassRenderer {
public:
	GrassRenderer(Scene* scene, Ref<Renderer> p_renderer, Vec3f p_min_pos,
			Vec3f p_max_pos, float p_distance_between);
	~GrassRenderer();

	void render(float p_dt);

private:
	void _render_func(CommandBuffer p_cmd, DeletionQueue& p_frame_deletion);

	void _repopulate_grass_buffer();

	std::tuple<Camera*, Transform> _get_main_camera();

	UniformSet _create_camera_set(DeletionQueue* p_frame_deletion);

private:
	Scene* scene;

	Ref<Renderer> renderer;
	Ref<RenderBackend> backend;

	Vec3f min_pos;
	Vec3f max_pos;
	float distance_between;

	glm::mat4 transform;

	Shader grass_shader;
	Pipeline grass_pipeline;

	Buffer grass_instance_buffer;
	UniformSet grass_instance_set;
	uint32_t grass_count;

	Shader grass_compute_shader;
	Pipeline grass_compute_pipeline;
	UniformSet compute_instance_set;

	Ref<Model> grass_model;
	// Ref<Model> grass_model_lod;

	float dt;
	Timer timer;

	struct GrassCameraUniform {
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec3 pos;
	};

	struct GrassPushConstants {
		glm::mat4 transform;
		BufferDeviceAddress grass_vertex_buffer;
		float time;
		float __padding;
	};

	struct ComputePushConstant {
		glm::mat4 transform;
		Vec3f min_pos;
		float distance_between;
		Vec3f max_pos;
		float __padding;
		glm::vec3 camera_pos;
		float __padding2;
		glm::vec3 camera_front;
		float __padding3;
	};
};
