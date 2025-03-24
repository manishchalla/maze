
#include "level.h"
#include "noise.h"


Quad::Quad(glm::vec2 position, std::shared_ptr<Texture> tex, glm::vec2 dimensions)
	: ecs::ComponentHandle<Position>(position)
	, ecs::ComponentHandle<Sprite>(tex,dimensions)
{}

CollisionQuad::CollisionQuad(glm::vec2 position, std::shared_ptr<Texture> tex, glm::vec2 dimensions)
	: Quad(position,tex,dimensions)
{
	Physics& phys = ecs::get<Physics>(id);
	phys.fixed = true;
	phys.solid = true;
	phys.bbox_dims = glm::vec2(dimensions);
}




void Creature::logic(float delta) {}

void Creature::on_death() {}

void Creature::logic_trampoline(size_t id, float delta, void* self_ptr) {
	Creature* self = (Creature*)self_ptr;
	self->logic(delta);
}

Creature::Creature()
	: ecs::ComponentHandle<Physics>()
	, ecs::ComponentHandle<Sprite>(TextureCache::load("assets/placeholder.png",false),glm::vec2(0.1f,0.1f))
	, ecs::ComponentHandle<AI>(logic_trampoline,this)
	, ecs::ComponentHandle<Status>(0,0)
{}

void Creature::track_life(std::shared_ptr<Creature> creature) {
	alive.push_back(creature);
}

void Creature::cleanup() {
	std::vector<std::shared_ptr<Creature>> old_alive;
	old_alive.reserve(alive.size());
	std::swap(alive,old_alive);
	for (auto creature : old_alive) {
		Status& status = ecs::get<Status>(*creature);
		if (status.health > 0) {
			alive.push_back(creature);
		}
		else {
			creature->on_death();
		}
	}
}

Creature::~Creature() {}


std::vector<std::shared_ptr<Creature>> Creature::alive = std::vector<std::shared_ptr<Creature>>();


Level::Level(std::string file_path) {
	std::ifstream level_file(file_path);
	if (!level_file) {
		std::string message = "Failed to load level at '";
		message += file_path + "'.";
		throw std::runtime_error(message);
	}
	std::string line;
	glm::ivec2 quad_pos(0, 0);
	while (std::getline(level_file,line)) {
		if (line.empty()) {
			break;
		}
		Quad* quad = nullptr;
		for (auto deser : quad_deserializers) {
			quad = deser(line);
			if (quad != nullptr) {
				break;
			}
		}
		if (quad == nullptr) {
			std::string message = "Failed to deserialize quad from string '";
			message = message + line + "'.";
			throw std::runtime_error(message);
		}
	}

	while (std::getline(level_file, line)) {
		if (line.empty()) {
			break;
		}	
		Creature* creature = nullptr;
		for (auto deser : creature_deserializers) {
			creature = deser(line);
			if (creature != nullptr) {
				break;
			}
		}
		if (creature == nullptr) {
			std::string message = "Failed to deserialize creature string '";
			message = message + line + "'.";
			throw std::runtime_error(message);
		}
		Creature::track_life(std::shared_ptr<Creature>(creature));
	}
}




Level::~Level() {
	for (auto row : grid) {
		for (auto quad : row) {
			delete quad;
		}
	}
}

std::vector < std::function<Quad* (std::string)> > Level::quad_deserializers = std::vector < std::function<Quad * (std::string)> >();
std::vector < std::function<Creature* (std::string)> > Level::creature_deserializers = std::vector < std::function<Creature * (std::string)> >();




void HealthBar::logic(size_t id, float delta, void* data) {
	HealthBar* self = (HealthBar*)data;
	if (! ecs::has<Status>(self->subject_id)) {
		return;
	}
	Physics& self_phys     = ecs::get<Physics>(*self);
	Sprite&  self_sprite   = ecs::get<Sprite>(*self);
	Physics& subject_phys  = ecs::get<Physics>(self->subject_id);
	Status&  subject_stat  = ecs::get<Status>(self->subject_id);
	glm::vec3& self_pos    = ecs::get<Position>(id);
	glm::vec3& subject_pos = ecs::get<Position>(self->subject_id);
	self_pos = subject_pos + glm::vec3(0.0f,0.1f, -0.15f);
	glm::vec2 new_dims(subject_stat.health * 0.001, 0.01);
	self_sprite.scale = new_dims;
	self_sprite.depth = -0.15f;
}

HealthBar::HealthBar(size_t subject_id)
	: ecs::ComponentHandle<Physics>()
	, ecs::ComponentHandle<Sprite>(TextureCache::load("assets/red.png",false),glm::vec2(0,0))
	, ecs::ComponentHandle<AI>(logic,this)
	, subject_id(subject_id)
{
	Physics& self_phys = ecs::get<Physics>(id);
	self_phys.solid = false;
}


void Enemy::logic(float delta) {
	Physics& my_phys = ecs::get<Physics>(id);
	glm::vec3& my_pos = ecs::get<Position>(id);
	Physics* target_phys = nullptr;
	float target_dist = std::numeric_limits<float>::infinity();
	for (auto creature : Creature::alive) {
		glm::vec3& other_pos = ecs::get<Position>(*creature);
		Status& status = ecs::get<Status>(*creature);
		bool valid_target = (status.alignment == Status::GOOD);
		Physics& other_phys = ecs::get<Physics>(*creature);
		float dist = glm::distance(my_pos, other_pos);
		bool target_closer = (target_phys == nullptr) || (dist < target_dist);
		if (valid_target && target_closer) {
			target_phys = &other_phys;
			target_dist = dist;
		}
	}
	if (target_phys != nullptr) {
		glm::vec3& target_pos = ecs::get<Position>(target_phys->get_id());
		glm::vec2 offset = target_pos - my_pos;
		my_phys.velocity += glm::vec2(glm::normalize(target_pos - my_pos) * delta);
	}
}

Enemy::Enemy(glm::vec2 position)
	: Creature()
	, health_bar(*this)
{
	glm::vec3& pos  = ecs::get<Position>(*this);
	Physics& phys   = ecs::get<Physics>(*this);
	Sprite & sprite = ecs::get<Sprite>(*this);
	Status & status = ecs::get<Status>(*this);
	phys.bbox_dims = glm::vec2(0.024f,0.024f);
	pos = glm::vec3(position,0.f);
	sprite.scale = glm::vec2(0.030f, 0.030f);
	sprite.tex = TextureCache::load("assets/enemy.png",false);
	sprite.depth = -0.1f;
	status.health = 100;
	status.alignment = Status::EVIL;
	phys.on_collide.push_back([](size_t id) {
		if (!ecs::has<Status>(id)) {
			return;
		}
		Status & status = ecs::get<Status>(id);
		if (status.alignment == Status::GOOD) {
			status.health-=10;
		}
	});
}


void Bullet::logic(float delta) {
	Status & status = ecs::get<Status>(*this);
	status.health--;
}

Bullet::Bullet(glm::vec2 position, glm::vec2 velocity)
	: Creature()
{
	glm::vec3& pos  = ecs::get<Position>(*this);
	Physics& phys   = ecs::get<Physics>(*this);
	Sprite & sprite = ecs::get<Sprite>(*this);
	Status & status = ecs::get<Status>(*this);
	phys.bbox_dims = glm::vec2(0.05f,0.05f);
	pos = glm::vec3(position,0.0f);
	phys.velocity = velocity;
	phys.has_drag = false;
	sprite.scale = glm::vec2(0.02f, 0.02f);
	sprite.tex = TextureCache::load("assets/ally.png",false);
	sprite.depth = -0.15f;
	status.health = 10;
	status.alignment = Status::NEUTRAL;
	phys.on_collide.push_back([](size_t id) {
		if (! ecs::has<Status>(id)) {
			return;
		}
		Status & status = ecs::get<Status>(id);
		if (status.alignment == Status::EVIL) {
			status.health -= 10;
		}
	});
}





void PopUp::logic(float delta) {

}

void PopUp::on_death() {
	mouseclick_callbacks.erase(id);
}

PopUp::PopUp(std::shared_ptr<Texture> tex, glm::vec2 position, glm::vec2 dimensions)
{
	glm::vec3& pos  = ecs::get<Position>(*this);
	Sprite& sprite = ecs::get<Sprite>(id);
	sprite.depth = -0.2f;
	sprite.tex = tex;
	sprite.scale = dimensions;
	Physics& phys = ecs::get<Physics>(id);
	phys.solid = false;
	pos = glm::vec3(position,0.f);
	ecs::get<Status>(id).health = 1;
	mouseclick_callbacks[id] = [this](glm::vec2 pos) {
		ecs::get<Status>(id).health = 0;
	};
}



void Bird::logic(float delta) {

	x_ease *= 0.9f;
	y_ease *= 0.9f;

	if ( (w_down || s_down) && (y_ease < 1.0f)) {
		y_ease += 0.01f;
	}

	if ( (d_down || a_down) && (x_ease < 1.0f) ) {
		x_ease += 0.01f;
	}

	float up_force = (w_down - s_down) * y_ease;
	float right_force = (d_down - a_down) * x_ease;


	Sprite& sprite = ecs::get<Sprite>(id);

	if (shoot_cooldown <= 0) {
		int frame_total = loop.tex_list.size() * loop.frames_per_tex;
		loop.tex_iterator = (loop.tex_iterator + 1) % frame_total;
		int tex_index = loop.tex_iterator / loop.frames_per_tex;
		sprite.tex = loop.tex_list[tex_index];
	}
	else {
		sprite.tex = TextureCache::load("assets/shoot_texture.png",false);
	}


	Physics& phys = ecs::get<Physics>(id);
	phys.velocity += glm::vec2(right_force, up_force);

	glm::vec3& pos  = ecs::get<Position>(*this);
	Sprite::cam_pos = pos;

	shoot_cooldown -= delta;

}


void Bird::on_death () {
	Creature::track_life(std::shared_ptr<Creature>(new PopUp(
		TextureCache::load("assets/game_over.png",false),
		Sprite::cam_pos,
		glm::vec2(0.5,0.5)
	)));
}

Bird::Bird(glm::vec2 position)
	: health_bar(*this)
{
	glm::vec3& pos  = ecs::get<Position>(*this);
	Physics& phys   = ecs::get<Physics>(*this);
	Sprite & sprite = ecs::get<Sprite>(*this);
	Status & status = ecs::get<Status>(*this);
	phys.bbox_dims = glm::vec2(0.08f,0.08f);
	pos = glm::vec3(position,0.f);
	sprite.scale = glm::vec2(0.1f, 0.1f);
	sprite.tex = TextureCache::load("assets/bird1.png",false);
	sprite.depth = -0.1f;
	status.health = 100;
	status.alignment = Status::GOOD;

}


float Bird::x_ease = 0.f;
float Bird::y_ease = 0.f;
bool  Bird::w_down = false;
bool  Bird::a_down = false;
bool  Bird::s_down = false;
bool  Bird::d_down = false;
float Bird::shoot_cooldown = 0.1f;
AnimationLoop Bird::loop = AnimationLoop{};




float random_norm() {
	return (rand() % 10000) / 10000.0f;
}




template<>
Bird* deserialize(std::string line) {
	std::stringstream stream(line);
	std::string creature_type;
	stream >> creature_type;
	if (creature_type != "bird") {
		return nullptr;
	}
	glm::vec2 position;
	if (!(stream >> position.x)) {
		return nullptr;
	}
	if (!(stream >> position.y)) {
		return nullptr;
	}
	return new Bird(position);
}

template<>
Enemy* deserialize(std::string line) {
	std::stringstream stream(line);
	std::string creature_type;
	stream >> creature_type;
	if (creature_type != "enemy") {
		return nullptr;
	}
	glm::vec2 position;
	if (!(stream >> position.x)) {
		return nullptr;
	}
	if (!(stream >> position.y)) {
		return nullptr;
	}
	return new Enemy(position);
}




