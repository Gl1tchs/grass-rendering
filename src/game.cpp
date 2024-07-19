#include "game.h"

#include <core/event/input.h>
#include <renderer/renderer.h>
#include <scene/components.h>
#include <scene/scene.h>
#include <scene/view.h>

Game::Game(const ApplicationCreateInfo& p_info) : Application(p_info) {}

void Game::_on_start() {
	// assign the scene to the renderer
	get_renderer()->set_scene(&scene);

	camera_entity = scene.create();
	{
		auto [cc, transform] =
				scene.assign<CameraComponent, Transform>(camera_entity);

		scene.assign<TagComponent>(camera_entity, "Camera");

		cc->is_primary = true;

		camera_controller.set_camera(&cc->camera, transform);
	}

	floor = Model::load_gltf("assets/floor.glb");
	if (floor) {
		Entity floor_entity = scene.create();

		auto [tag, transform, mesh] =
				scene.assign<TagComponent, Transform, MeshRendererComponent>(
						floor_entity);

		tag->name = "Floor";

		transform->local_scale = { 25, 1, 25 };

		mesh->model = floor;
	}

	grass_renderer = create_ref<GrassRenderer>(&scene, get_renderer(),
			Vec3f{ -25.0f, 0, -25.0f }, Vec3f{ 25.0f, 0, 25.0f }, 0.15f);
}

void Game::_on_update(float p_dt) {
	if (scene.has<CameraComponent>(camera_entity)) {
		scene.get<CameraComponent>(camera_entity)->camera.aspect_ratio =
				get_window()->get_aspect_ratio();
	}

	static bool mouse_disabled = false;
	if (Input::is_mouse_pressed(MOUSE_BUTTON_RIGHT)) {
		if (!mouse_disabled) {
			get_window()->set_cursor_mode(WINDOW_CURSOR_MODE_DISABLED);
			mouse_disabled = true;
		}

		camera_controller.update(p_dt);
	} else {
		camera_controller.last_mouse_pos.x = Input::get_mouse_position().x;
		camera_controller.last_mouse_pos.y = Input::get_mouse_position().y;
	}

	if (Input::is_mouse_released(MOUSE_BUTTON_RIGHT)) {
		if (mouse_disabled) {
			get_window()->set_cursor_mode(WINDOW_CURSOR_MODE_NORMAL);
			mouse_disabled = false;
		}
	}

	grass_renderer->render(p_dt);
}

void Game::_on_destroy() {
	get_renderer()->wait_for_device();

	if (floor) {
		Model::destroy(floor);
	}
}
