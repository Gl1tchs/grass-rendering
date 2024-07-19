#include "grass.h"

#include <core/transform.h>
#include <renderer/camera.h>
#include <renderer/types.h>
#include <scene/components.h>
#include <tuple>

static std::vector<uint32_t> _get_spirv_data(const fs::path& p_filepath) {
	size_t file_size = fs::file_size(p_filepath);

	std::ifstream file(p_filepath, std::ios::in | std::ios::binary);

	if (!file.is_open()) {
		return {};
	}

	std::vector<uint32_t> buffer(file_size);
	file.read(reinterpret_cast<char*>(buffer.data()), file_size);

	return buffer;
}

GrassRenderer::GrassRenderer(Scene* p_scene, Ref<Renderer> p_renderer,
		Vec3f p_min_pos, Vec3f p_max_pos, float p_distance_between) :
		scene(p_scene),
		renderer(p_renderer),
		backend(p_renderer->get_backend()),
		min_pos(p_min_pos),
		max_pos(p_max_pos),
		distance_between(p_distance_between) {
	// create pipeline for drawing
	SpirvData vertex_data = {};
	vertex_data.stage = SHADER_STAGE_VERTEX_BIT;
	vertex_data.byte_code = _get_spirv_data("assets/shaders/grass.vert.spv");

	SpirvData fragment_data = {};
	fragment_data.stage = SHADER_STAGE_FRAGMENT_BIT;
	fragment_data.byte_code = _get_spirv_data("assets/shaders/grass.frag.spv");

	std::vector<SpirvData> shader_stages = {
		vertex_data,
		fragment_data,
	};

	grass_shader = backend->shader_create_from_bytecode(shader_stages);

	PipelineDepthStencilState depth_stencil_state = {};
	depth_stencil_state.depth_compare_operator = COMPARE_OP_LESS;
	depth_stencil_state.enable_depth_test = true;
	depth_stencil_state.enable_depth_write = true;
	depth_stencil_state.enable_depth_range = true;

	PipelineColorBlendState color_blend_state =
			PipelineColorBlendState::create_blend();

	RenderingState rendering_state = {};
	rendering_state.color_attachments.push_back(
			Renderer::get_draw_image_format());
	rendering_state.depth_attachment = Renderer::get_depth_image_format();

	grass_pipeline = backend->render_pipeline_create(grass_shader,
			RENDER_PRIMITIVE_TRIANGLES, {}, {}, depth_stencil_state,
			color_blend_state, 0, rendering_state);

	// load grass models
	grass_model = Model::load_gltf("assets/grass_LOD.glb");

	GL_ASSERT(grass_model);

	// grass field transform
	{
		const Vec3f position = (max_pos + min_pos) / 2.0f;

		Transform field_transform;
		memcpy(&field_transform.local_position, &position, sizeof(Vec3f));

		transform = field_transform.get_transform_matrix();
	}

	const uint32_t grass_count_x = (max_pos.x - min_pos.x) / distance_between;
	const uint32_t grass_count_z = (max_pos.z - min_pos.z) / distance_between;

	grass_count = grass_count_x * grass_count_z;

	const size_t data_size = grass_count * sizeof(Grass);

	// buffer to store our instances
	grass_instance_buffer = backend->buffer_create(data_size,
			BUFFER_USAGE_STORAGE_BUFFER_BIT | BUFFER_USAGE_TRANSFER_DST_BIT,
			MEMORY_ALLOCATION_TYPE_GPU);

	// populate buffer using compute shader
	{
		SpirvData compute_data = {};
		compute_data.stage = SHADER_STAGE_COMPUTE_BIT;
		compute_data.byte_code =
				_get_spirv_data("assets/shaders/"
								"populate_grass_buffer.comp.spv");

		std::vector<SpirvData> shaders = { compute_data };

		grass_compute_shader = backend->shader_create_from_bytecode(shaders);
		grass_compute_pipeline =
				backend->compute_pipeline_create(grass_compute_shader);

		std::vector<ShaderUniform> uniforms(1);
		uniforms[0].type = UNIFORM_TYPE_STORAGE_BUFFER;
		uniforms[0].binding = 0;
		uniforms[0].data.push_back(grass_instance_buffer);

		compute_instance_set =
				backend->uniform_set_create(uniforms, grass_compute_shader, 0);
	}

	{
		std::vector<ShaderUniform> uniforms(1);
		uniforms[0].type = UNIFORM_TYPE_STORAGE_BUFFER;
		uniforms[0].binding = 0;
		uniforms[0].data.push_back(grass_instance_buffer);

		grass_instance_set =
				backend->uniform_set_create(uniforms, grass_shader, 1);
	}
}

GrassRenderer::~GrassRenderer() {
	Model::destroy(grass_model);
	// Model::destroy(grass_model_lod);

	backend->shader_free(grass_compute_shader);
	backend->pipeline_free(grass_compute_pipeline);
	backend->uniform_set_free(compute_instance_set);

	backend->uniform_set_free(grass_instance_set);
	backend->buffer_free(grass_instance_buffer);

	backend->pipeline_free(grass_pipeline);
	backend->shader_free(grass_shader);
}

void GrassRenderer::render(float p_dt) {
	dt = p_dt;

	_repopulate_grass_buffer();

	renderer->submit(GL_BIND_FUNC(_render_func));
}

void GrassRenderer::_render_func(
		CommandBuffer p_cmd, DeletionQueue& p_frame_deletion) {
	backend->command_bind_graphics_pipeline(p_cmd, grass_pipeline);

	UniformSet camera_set = _create_camera_set(&p_frame_deletion);

	std::vector<UniformSet> uniform_sets = {
		camera_set,
		grass_instance_set,
	};
	backend->command_bind_uniform_sets(p_cmd, grass_shader, 0, uniform_sets);

	const Ref<Mesh>& grass_mesh = grass_model->meshes.front();
	// const Ref<Mesh>& grass_mesh_lod = grass_model_lod->meshes.front();

	GrassPushConstants push_constants = {};
	push_constants.transform = transform;
	push_constants.grass_vertex_buffer = grass_mesh->vertex_buffer_address;
	push_constants.time = timer.get_elapsed_seconds();

	backend->command_push_constants(p_cmd, grass_shader, 0,
			sizeof(GrassPushConstants), &push_constants);

	backend->command_bind_index_buffer(
			p_cmd, grass_mesh->index_buffer, 0, grass_mesh->index_type);

	backend->command_draw_indexed(p_cmd, grass_mesh->index_count, grass_count);
}

void GrassRenderer::_repopulate_grass_buffer() {
	backend->command_immediate_submit([&](CommandBuffer p_cmd) {
		auto [_, camera_transform] = _get_main_camera();
		ComputePushConstant compute_constants = {};
		compute_constants.min_pos = min_pos;
		compute_constants.distance_between = distance_between;
		compute_constants.max_pos = max_pos;
		compute_constants.camera_pos = camera_transform.get_position();
		compute_constants.camera_front = camera_transform.get_forward();

		// execute shader
		backend->command_bind_compute_pipeline(p_cmd, grass_compute_pipeline);

		backend->command_bind_uniform_sets(p_cmd, grass_compute_shader, 0,
				compute_instance_set, PIPELINE_TYPE_COMPUTE);

		backend->command_push_constants(p_cmd, grass_compute_shader, 0,
				sizeof(ComputePushConstant), &compute_constants);

		backend->command_dispatch(p_cmd, grass_count / 16, 1, 1);
	});
}

std::tuple<Camera*, Transform> GrassRenderer::_get_main_camera() {
	for (const Entity entity : scene->view<CameraComponent, Transform>()) {
		auto [cc, transform] = scene->get<CameraComponent, Transform>(entity);
		if (cc->is_primary) {
			return std::make_tuple(&cc->camera, *transform);
		}
	}
	return std::make_tuple(nullptr, DEFAULT_TRANSFORM);
}

UniformSet GrassRenderer::_create_camera_set(DeletionQueue* p_frame_deletion) {
	Buffer camera_uniform_buffer = backend->buffer_create(
			sizeof(GrassCameraUniform),
			BUFFER_USAGE_UNIFORM_BUFFER_BIT | BUFFER_USAGE_TRANSFER_SRC_BIT,
			MEMORY_ALLOCATION_TYPE_CPU);

	p_frame_deletion->push_function(
			[=, this]() { backend->buffer_free(camera_uniform_buffer); });

	GrassCameraUniform* camera_uniform_data =
			(GrassCameraUniform*)backend->buffer_map(camera_uniform_buffer);

	auto [camera, transform] = _get_main_camera();
	if (camera) {
		camera_uniform_data->view = camera->get_view_matrix(transform);
		camera_uniform_data->proj = camera->get_projection_matrix();
		camera_uniform_data->pos = transform.get_position();
	}
	backend->buffer_unmap(camera_uniform_buffer);

	ShaderUniform uniform;
	uniform.type = UNIFORM_TYPE_UNIFORM_BUFFER;
	uniform.binding = 0;
	uniform.data.push_back(camera_uniform_buffer);

	UniformSet camera_uniform =
			backend->uniform_set_create(uniform, grass_shader, 0);
	p_frame_deletion->push_function(
			[=, this]() { backend->uniform_set_free(camera_uniform); });

	return camera_uniform;
}
