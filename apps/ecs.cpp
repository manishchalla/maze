// By Braxton Cuneo, Published under Creative Commons CC-BY

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glad.h>
#include <glfw3.h>

#include "glazy.h"
#include "shape.h"


// Window dimensions
glm::ivec2 const window_dims = { 1000,1000 };
glm::ivec2 const window_pos = { 100, 100 };



#include "components.h"
#include "level.h"
#include "noise.h"
#include "quad.h"



void key_handler(GLFWwindow* window, int key, int scancode, int action, int mods);
void entry_exit_handler(GLFWwindow* window, int entered);
void cursor_position(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

int main() {

	std::vector<glazy::context::WindowHint> hints = {};
	GLFWwindow* window = setup(window_pos, window_dims, "ECS Platform", hints);
	glfwSetKeyCallback(window, key_handler);
	glfwSetCursorEnterCallback(window, entry_exit_handler);
	glfwSetCursorPosCallback(window, cursor_position);
	glfwSetMouseButtonCallback(window, mouse_button_callback);


	std::shared_ptr<Texture>  bird_tex1(new Texture("assets/bird1.png", true));
	std::shared_ptr<Texture>  bird_tex2(new Texture("assets/bird2.png", true));
	std::shared_ptr<Texture>  ally_tex(new Texture("assets/ally.png", true));
	std::vector<std::shared_ptr<Bird>> birds;
		
	Bird::loop = {
		{ bird_tex1,bird_tex2 },
		20,
		0
	};

	Physics::collision_map.configure(0.1f, { 200, 200 }, { -100, -100 });

	Level::register_quad_class<Wall>();
	Level::register_quad_class<Background>();
	Level::register_quad_class<TextBox>();

	Level::register_creature_class<Bird>();
	Level::register_creature_class<Enemy>();

	Level level("assets/level_0.txt");

	// Make sure to use the u8 prefix and not to use multi-codepoint symbols
	// (unless you want to handle that logic yourself, of course).
	std::string unicode = u8"Example Unicode:\n😀🌳💤🤙👀🐁🐢☢\n古池や\n蛙飛び込む\n水の音";
	TextBox unicode_textbox(glm::vec2(-2,0),glm::vec2(0.4,0.4),glm::ivec2(8,8),unicode);
	
	TextBox counting_textbox(glm::vec2(0,0.6),glm::vec2(0.8,0.1),glm::ivec2(8,1),unicode);
	
	TextBox fps_textbox(glm::vec2(0.7,-0.95),glm::vec2(0.48,0.04),glm::ivec2(12,1),unicode);
	Sprite& fps_sprite = ecs::get<Sprite>(fps_textbox);
	fps_sprite.depth = -0.5f;
	fps_sprite.screenlock = true;
	fps_textbox.set_background(glm::vec4(0,0,0,0));
	fps_textbox.set_foreground(glm::vec4(1,0,0,1));

	srand((unsigned int) time(nullptr));
	
	
	mouseclick_callbacks[-1] = [](glm::vec2 mouse_position) {
		if (Bird::shoot_cooldown > 0) {
			return;
		}
		glm::vec2 correction(Sprite::cam_pos.x, Sprite::cam_pos.y);
		glm::vec2 offset = correction - mouse_position;
		glm::vec2 direction = glm::normalize(-mouse_position);
		glm::vec2 velocity = direction * 5.f;
		glm::vec2 position = Sprite::cam_pos + direction*0.1f;
		Bird::shoot_cooldown = 0.2f;
		Creature::track_life(std::shared_ptr<Creature>(new Bullet(position, velocity)));
	};

	Physics::set_drag(0.1f);

	glEnable(GL_DEPTH_TEST);

	int frame_count = 0;

	GLfloat first_time = (float)glfwGetTime();
	GLfloat last_time = (float)glfwGetTime();
	while (!glfwWindowShouldClose(window)) {

		Creature::cleanup();
		ecs::cleanup<AI>();
		ecs::cleanup<Physics>();
		ecs::cleanup<Sprite>();
		ecs::cleanup<Status>();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		GLfloat time = (float)glfwGetTime();	
		counting_textbox.set_text(std::to_string(time),true);

		GLfloat fps = 1.f / (time - last_time);
		std::string fps_string = "fps=";
		fps_string += std::to_string(fps);
		fps_textbox.set_text(fps_string, true);

		ecs::ComponentSet<AI>::delta_update(time-last_time);
		Physics::update_collision_map();
		ecs::ComponentSet<Physics>::delta_update(time-last_time);

		VAO::BindGuard guard(Sprite::get_vao());
		glActiveTexture(GL_TEXTURE0);
		ecs::ComponentSet<Sprite>::fixed_update();
		last_time = time;
		// Add this to simulate random lag
		// std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 80));
		glFlush();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}


void key_handler(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_G:
			if (Physics::gravity < 0.001) {
				Physics::gravity = 0.1f;
			}
			else {
				Physics::gravity = 0.f;
			}
			break;
		}
	}

	switch (key) {
	case GLFW_KEY_W: Bird::w_down = (action != GLFW_RELEASE); break;
	case GLFW_KEY_A: Bird::a_down = (action != GLFW_RELEASE); break;
	case GLFW_KEY_S: Bird::s_down = (action != GLFW_RELEASE); break;
	case GLFW_KEY_D: Bird::d_down = (action != GLFW_RELEASE); break;
	}

}

void entry_exit_handler(GLFWwindow* window, int entered) {
	mouse_on_screen = entered;
	if (entered) {
		std::cout << "Mouse entered!" << std::endl;
	}
	else {
		std::cout << "Mouse left!" << std::endl;
	}
}

void cursor_position(GLFWwindow* window, double xpos, double ypos) {
	xpos = -((xpos / 500.f) - 1.0f);
	ypos = (ypos / 500.f) - 1.0f;
	mouse_position = glm::vec2((float)xpos, (float)ypos);
}


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if ((button == GLFW_MOUSE_BUTTON_1) && (action == GLFW_PRESS)) {
		for (const auto& pair : mouseclick_callbacks) {
			pair.second(mouse_position);
		}
	}
}




