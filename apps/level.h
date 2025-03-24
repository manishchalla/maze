
#ifndef LEVEL
#define LEVEL

#include "components.h"





struct Quad
	: ecs::ComponentHandle<Position>
	, ecs::ComponentHandle<Sprite>
{
	glm::vec2 const tile_scale = glm::vec2(0.1f, 0.1f);
	Quad(glm::vec2 position, std::shared_ptr<Texture> tex, glm::vec2 dimensions);
};

struct CollisionQuad
	: Quad
	, ecs::ComponentHandle<Physics>
{
	CollisionQuad(glm::vec2 position, std::shared_ptr<Texture> tex, glm::vec2 dimensions);
};

struct Creature
	: ecs::ComponentHandle<Position>
	, ecs::ComponentHandle<Physics>
	, ecs::ComponentHandle<Sprite>
	, ecs::ComponentHandle<AI>
	, ecs::ComponentHandle<Status>
{

	static std::vector<std::shared_ptr<Creature>> alive;
	virtual void logic(float delta);
	virtual void on_death();
	static void logic_trampoline(size_t id, float delta, void* self_ptr);
	Creature();
	static void track_life(std::shared_ptr<Creature> creature);
	static void cleanup();
	virtual ~Creature();

};


struct Level {

	static std::vector < std::function<Quad*(std::string)> > quad_deserializers;
	static std::vector < std::function<Creature*(std::string)> > creature_deserializers;

	std::vector<std::vector<Quad*>> grid;

	Level(std::string file_path);
	~Level();

	template<typename T>
	static void register_quad_class() {
		quad_deserializers.push_back(deserialize<T>);
	}

	template<typename T>
	static void register_creature_class() {
		creature_deserializers.push_back(deserialize<T>);
	}
	
};

struct HealthBar
	: ecs::ComponentHandle<Position>
	, ecs::ComponentHandle<Physics>
	, ecs::ComponentHandle<Sprite>
	, ecs::ComponentHandle<AI>
{

	size_t subject_id;

	static void logic(size_t id, float delta, void* data);
	HealthBar(size_t subject_id);

};




struct Enemy : public Creature {

	HealthBar health_bar;

	void logic(float delta);
	Enemy(glm::vec2 position);

};



struct Bullet : public Creature {

	void logic(float delta);
	Bullet(glm::vec2 position, glm::vec2 velocity);

};




struct PopUp : Creature {

	void logic(float delta);
	void on_death();
	PopUp(std::shared_ptr<Texture> tex, glm::vec2 position, glm::vec2 dimensions);

};



struct AnimationLoop {
	
	std::vector<std::shared_ptr<Texture>> tex_list;
	int frames_per_tex;
	int tex_iterator;

};




// A ball has position, velocity, gravity, and a sprite
class Bird : public Creature {

	void logic(float delta);
	void on_death();

public:


	static float x_ease;
	static float y_ease;
	static bool w_down;
	static bool a_down;
	static bool s_down;
	static bool d_down;

	static float shoot_cooldown;

	static AnimationLoop loop;

	HealthBar health_bar;

	Bird(glm::vec2 position);

};


template<>
Bird* deserialize(std::string line);

template<>
Enemy* deserialize(std::string line);




#endif 



