#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <iostream>
#include <algorithm>    // std::max
#include <stdlib.h>     // srand, rand

GLuint runner_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > runner_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("runner.pnct"));
	runner_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > runner_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("runner.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = runner_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = runner_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::PlayMode() : scene(*runner_scene) {
	// get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Car") car = &transform;
		// else if (transform.name == "PlayerObjects") player = &transform;
		else if (transform.name[0] == 'B') {
			block_base_rotation = transform.rotation;
			transform.position.y = 0.0f;
		} else if (transform.name[0] == 'R') {
			road_base_rotation = transform.rotation;
			std::cout << transform.position.y;
		}
		// else if (transform.name == "LowerLeg.FL") lower_leg = &transform;
	}
	if (car == nullptr) throw std::runtime_error("Car not found.");
	// if (player == nullptr) throw std::runtime_error("Player Objects not found.");
	// if (upper_leg == nullptr) throw std::runtime_error("Upper leg not found.");
	// if (lower_leg == nullptr) throw std::runtime_error("Lower leg not found.");

	// hip_base_rotation = hip->rotation;
	// upper_leg_base_rotation = upper_leg->rotation;
	// lower_leg_base_rotation = lower_leg->rotation;
	// player_base_rotation = player->rotation;
	car_base_rotation = car->rotation;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
	camera_base_rotation = camera->transform->rotation;
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			// glm::vec2 motion = glm::vec2(
			// 	evt.motion.xrel / float(window_size.y),
			// 	-evt.motion.yrel / float(window_size.y)
			// );
			// camera->transform->rotation = glm::normalize(
			// 	camera->transform->rotation
			// 	* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
			// 	* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			// );
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):
	wobble += elapsed / 10.0f;
	wobble -= std::floor(wobble);

	// hip->rotation = hip_base_rotation * glm::angleAxis(
	// 	glm::radians(5.0f * std::sin(wobble * 2.0f * float(M_PI))),
	// 	glm::vec3(0.0f, 1.0f, 0.0f)
	// );
	// upper_leg->rotation = upper_leg_base_rotation * glm::angleAxis(
	// 	glm::radians(7.0f * std::sin(wobble * 2.0f * 2.0f * float(M_PI))),
	// 	glm::vec3(0.0f, 0.0f, 1.0f)
	// );
	// lower_leg->rotation = lower_leg_base_rotation * glm::angleAxis(
	// 	glm::radians(10.0f * std::sin(wobble * 3.0f * 2.0f * float(M_PI))),
	// 	glm::vec3(0.0f, 0.0f, 1.0f)
	// );

	//move camera:
	{


		//combine inputs into a move:
		static float PlayerSpeed = 10.0f;
		if (lost)
			PlayerSpeed = 5.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =1.0f;
		if (!left.pressed && right.pressed) move.x = -1.0f;
		if (down.pressed && !up.pressed) move.y = 0.0f;
		if (!down.pressed && up.pressed) move.y = -1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		// glm::mat4x3 frame = car->make_local_to_parent();
		// glm::vec3 frame_right = frame[0];
		// glm::vec3 frame_up = frame[1];
		// glm::vec3 frame_forward = -frame[2];

		// car->position += move.x * frame_up + move.y * frame_forward;
		float new_pos = car->position.x;
		new_pos += move.x;
		new_pos = std::max(new_pos, -2.0f);
		new_pos = std::min(new_pos, 2.0f);
		car->position.x = new_pos;



		static float RoadSpeed = 50.0f;
		// RoadSpeed += 0.1;
		if (lost)
			RoadSpeed = 15.0f;
		constexpr float FlipDist = 4.2f;
		float CarFlipDist = FlipDist - 1.0f;
		srand (time(NULL));

		prevIsUp = isUp;

		if (up.pressed && !isUp) {
			car->position.z += CarFlipDist;
			car->rotation = car_base_rotation * glm::angleAxis(
				glm::radians(float(180)),
				glm::vec3(1.0f, 0.0f, 0.0f)
			);
		} else if (!up.pressed && isUp){
			car->position.z -= CarFlipDist;
			car->rotation = car_base_rotation;
		}
		for (auto &transform : scene.transforms) {
			if (transform.name[0] == 'R') {

				transform.position.y += RoadSpeed * elapsed;
				if (transform.position.y > 19.0f)
					transform.position.y = -(61.6127);
				if (up.pressed && !isUp) {
					transform.position.z += FlipDist;
				} else if (!up.pressed && isUp){
					transform.position.z -= FlipDist;
				}
			}
			if (transform.name[0] == 'B') {
				transform.position.y += RoadSpeed * elapsed;

				auto hit = [=](glm::vec3 a, glm::vec3 b) {
		                float dist_x = abs(b.x - a.x);
		                float dist_y = abs(b.y - a.y);
						float dist_z = abs(b.z - a.z);

		                return dist_z < 1.0f && dist_y < 1.0f && dist_x < 2.0f;
		        };

				if (hit(transform.position, car->position)) {
					transform.position.y = -1.0f;
					lost = true;
				}

				if (transform.position.y > -2.0f) {
					if (!lost)
						score += 1;
					transform.position.y = -((rand() % 1000) + 10.0f);
				}
				// if (isUp != prevIsUp) {
				// 	if (isUp) {
				// 		if (transform.name[6] == 'L')
				// 			transform.position.z += FlipDist;
				// 		else
				// 			transform.position.z -= FlipDist;
				// 	} else {
				// 		if (transform.name[6] == 'L')
				// 			transform.position.z -= FlipDist;
				// 		else
				// 			transform.position.z += FlipDist;
				// 	}
				// }
			}
		}

		if (up.pressed && !isUp) {
			isUp = true;
		} else if (!up.pressed && isUp){
			isUp = false;
		}

	}



	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.50f;
		constexpr float hH = 0.1f;
		lines.draw_text("Press A and D to move; Hold W to flip gravity; Don't crash",
			glm::vec3(-aspect + 0.1f * hH, -1.0 + 0.1f * hH, 0.0),
			glm::vec3(hH, 0.0f, 0.0f), glm::vec3(0.0f, hH, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Press A and D to move; Hold W to flip gravity; Don't crash",
			glm::vec3(-aspect + 0.1f * hH + ofs, -1.0 + + 0.1f * hH + ofs, 0.0),
			glm::vec3(hH, 0.0f, 0.0f), glm::vec3(0.0f, hH, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		float y = -0.2;
		float x = -aspect;
		lines.draw_text(std::to_string(score),
			glm::vec3(x + 0.1f * H, y + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		if (lost) {
			lines.draw_text("WASTED",
				glm::vec3(x/2 + 0.2f + 0.1f * H, -0.2 + 0.1f * H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}
	}
}
