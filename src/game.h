#pragma once

#include <core/application.h>
#include <renderer/camera.h>
#include <renderer/model.h>
#include <scene/scene.h>

#include "camera_controller.h"
#include "grass.h"

class Game : public Application {
public:
	Game(const ApplicationCreateInfo& info);
	virtual ~Game() = default;

protected:
	void _on_start() override;

	void _on_update(float p_dt) override;

	void _on_destroy() override;

private:
	Scene scene;
	Entity camera_entity;

	Ref<Model> floor;

	CameraController camera_controller;
	PerspectiveCamera camera;

	Ref<GrassRenderer> grass_renderer;
};
