
#ifndef COMPONENTS
#define COMPONENTS

#include "common.h"

// Forward declare our component types
struct Position;
struct Physics;
struct Sprite;
struct AI;



struct CollisionMap {
	float scale;
	glm::ivec2 dims;
	glm::ivec2 offset;
	std::vector<std::vector<Physics*>> buckets;

	CollisionMap();
	void clear();
	std::vector<Physics*>* bucket_at(glm::ivec2 position);
	void mark(glm::ivec2 minima, glm::ivec2 maxima, Physics* marker);
	void configure(float scale, glm::ivec2 dims, glm::ivec2 offset);
};


struct Position : public ecs::Component {
	glm::vec3     position;
	operator glm::vec3& ();
	Position(size_t id);
	Position(size_t id, glm::vec2 position);
	Position(size_t id, glm::vec3 position);
};



// All entities with velocity update their position component over time
struct Physics : public ecs::Component {
	
	static float  drag;
	static float  gravity;

	static CollisionMap collision_map;

	glm::vec2     velocity;
	bool fixed;
	bool solid;
	bool has_drag;

	std::vector<std::function<void(size_t)>> on_collide;

	glm::vec2     bbox_dims;

	Physics(size_t id);
	bool collides(Physics& other, glm::vec2& normal);
	void resolve_collision(Physics& other, glm::vec2 normal);
	void delta_update(float delta);
	static void update_collision_map();
	static void set_drag(float new_drag);
	static void set_gravity(float new_gravity);
	Physics(Physics const&) = default;
};



// The sprite component renders a sprite to the screen with a given
// image and dimensions, with the screen position informed by the 
// position component
struct Sprite : public ecs::Component {


	// All sprites use the same unit-wide quad
	static bool is_setup;
	static VAO               *vao;
	static Buffer<glm::vec3> *pos;
	static Buffer<glm::vec2> *uv;
	static glm::vec2          cam_pos;

	

	// Sprites can have different textures and dimensions
	std::function<void(std::shared_ptr<GPUProgram>)> uniform_callback;
	std::shared_ptr<GPUProgram> program;
	std::shared_ptr<Texture> tex;
	glm::vec2 scale;
	float depth;
	bool screenlock;

	static void setup();
	Sprite(size_t id, std::shared_ptr<Texture> tex, glm::vec2 scale);
	Sprite(size_t id, std::shared_ptr<Texture> tex, glm::vec2 scale, std::shared_ptr<GPUProgram> program);
	Sprite(size_t id, std::shared_ptr<Texture> tex, glm::vec2 scale, std::shared_ptr<GPUProgram> program, std::function<void(std::shared_ptr<GPUProgram>)> uniform_callback);
	void fixed_update();
	GPUProgram& get_program();
	static VAO& get_vao();
	Sprite(Sprite const&) = default;

};


struct AI : ecs::Component {

	void (*d_update)(size_t,float,void*);
	void* data;

	AI(size_t id, void(*d_update)(size_t, float, void*), void* data);
	void delta_update(float delta);
	AI(AI const&) = default;

};


struct Status : ecs::Component {

	int health;
	int armor;

	enum Alignment {
		EVIL    = -1,
		NEUTRAL =  0,
		GOOD    =  1,
	};

	Alignment alignment;
	Status(int id, int health, int armor);
	Status(Status const&) = default;

};


#endif
