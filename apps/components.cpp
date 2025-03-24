
#include "components.h"



Position::operator glm::vec3& () {
	return position;
}
	
Position::Position(size_t id)
	: Component(id)
	, position(0.f,0.f,0.f)
{}

Position::Position(size_t id, glm::vec2 position)
	: Component(id)
	, position(position,0.f)
{}

Position::Position(size_t id, glm::vec3 position)
	: Component(id)
	, position(position)
{}


CollisionMap::CollisionMap()
	: scale(1.f)
	, dims(0,0)
	, offset(0,0)
	, buckets()
{}

void CollisionMap::clear() {
	for (auto& bucket : buckets) {
		bucket.resize(0);
	}
}

std::vector<Physics*>* CollisionMap::bucket_at(glm::ivec2 position) {
	int y_index = position.y - offset.y;
	if ((y_index < 0) || (y_index >= dims.y)) {
		return nullptr;
	}
	int x_index = position.x - offset.x;
	if ((x_index < 0) || (x_index >= dims.x)) {
		return nullptr;
	}
	return &buckets[y_index * dims.x + x_index];
}

void CollisionMap::mark(glm::ivec2 minima, glm::ivec2 maxima, Physics* marker) {
	for (int y = minima.y; y <= maxima.y; y++) {
		for (int x = minima.x; x <= maxima.x; x++) {
			std::vector<Physics*> *bucket = bucket_at({ x,y });
			if (bucket == nullptr) {
				continue;
			}
			bucket->push_back(marker);
		}
	}
}


void CollisionMap::configure(float scale, glm::ivec2 dims, glm::ivec2 offset) {
	this->scale = scale;
	this->offset = offset;
	this->dims = dims;
	buckets.resize(dims.x * dims.y);
	clear();
}





Physics::Physics(size_t id)
	: Component(id)
	, velocity(0.f,0.f)
	, bbox_dims(0.f,0.f)
	, fixed(false)
	, solid(true)
	, has_drag(true)
{}


bool Physics::collides(Physics& other, glm::vec2& normal) {

	// Get relative offset
	glm::vec3& pos = ecs::get<Position>(id);
	glm::vec3& other_pos = ecs::get<Position>(other.id);
	glm::vec2 offset = pos - other_pos;
	glm::vec2 abs_offset = glm::abs(offset);

	// Determine if a collision has occurred
	glm::bvec2 collide = glm::lessThanEqual(abs_offset, (other.bbox_dims + bbox_dims));
	if (!glm::all(collide)) {
		return false;
	}

	normal = glm::vec2(0, 0);
	if (abs(offset.y) < glm::max(bbox_dims.y,other.bbox_dims.y)) {
		normal.x = (offset.x > 0) ? 1.f : -1.f;
	}
	if (abs(offset.x) < glm::max(bbox_dims.x,other.bbox_dims.x)) {
		normal.y = (offset.y > 0) ? 1.f : -1.f;
	}
	return true;
}

void Physics::resolve_collision(Physics& other, glm::vec2 normal) {
	glm::vec3& pos = ecs::get<Position>(id);
	glm::vec3& other_pos = ecs::get<Position>(other.id);
	glm::vec2 normal_mask = glm::abs(normal);
	glm::vec2 tangent_mask = glm::vec2(1, 1) - normal_mask;
	glm::vec2 relative_velocity = velocity - other.velocity;
	glm::vec2 position_restitution = ((bbox_dims+other.bbox_dims) - glm::vec2(glm::abs(pos - other_pos))) * normal;
	glm::vec2 velocity_restitution = (glm::abs(relative_velocity) * normal) * 2.f;
	glm::vec2 friction_force = -relative_velocity * tangent_mask * 0.1f;
	if (glm::dot(relative_velocity, normal) > 0 ) {
		velocity_restitution = glm::vec2(0,0);
	}
	float weight       = 0.5;
	float other_weight = 0.5;
	if (other.fixed) {
		other_weight = 0;
		weight = 1;
	}
	else if (fixed) {
		other_weight = 1;
		weight = 0;
	}
	velocity += (velocity_restitution+friction_force) * weight;
	pos      += glm::vec3(position_restitution * weight,0.f);
	other.velocity -= (velocity_restitution+friction_force) * other_weight;
	other_pos      -= glm::vec3(position_restitution * other_weight,0.f);
}

void Physics::delta_update(float delta) {

	if (!fixed) {
		velocity.y -= gravity;
		
		// update position based upon velocity
		if (has_drag) {
			velocity *= pow(drag,delta);
		}
		glm::vec3& pos = ecs::get<Position>(id);
		pos += glm::vec3(velocity * delta,0.f);
	}

	std::vector<Physics*> collision_record;
	collision_record.reserve(100);

	glm::vec3& pos = ecs::get<Position>(id);
	glm::ivec2 minima  = glm::floor( (glm::vec2(pos) - bbox_dims) / collision_map.scale);
	glm::ivec2 maxima = glm::ceil( (glm::vec2(pos) + bbox_dims) / collision_map.scale);
	for (int y = minima.y; y <= maxima.y; y++) {
		for (int x = minima.x; x <= maxima.x; x++) {
			glm::ivec2 const pos(x, y);
			std::vector<Physics*>* bucket_ptr = collision_map.bucket_at(pos);
			if (bucket_ptr == nullptr) {
				continue;
			}
			for (Physics *other_ptr : *bucket_ptr) {
				Physics& other = *other_ptr;
				glm::vec2 normal;
				bool visited = false;
				if (other.id <= id) {
					continue;
				}
				for (auto visit_id : collision_record) {
					if (visit_id == &other) {
						visited = true;
					}
				}
				if (visited) {
					continue;
				}
				else {
					collision_record.push_back(&other);
				}
				if (collides(other, normal)) {
					for (auto& fn : on_collide) {
						fn(other.id);
					}
					for (auto& fn : other.on_collide) {
						fn(id);
					}
					if (fixed && other.fixed) {
						continue;
					}
					if ((!solid) || (!other.solid)) {
						continue;
					}
					resolve_collision(other, normal);
				}
			}
		}
	}

}

void Physics::update_collision_map() {
	collision_map.clear();
	ecs::ComponentSet<Physics>::for_each(
		[](Physics& phys) {
			if ( (!phys.solid) && (phys.on_collide.empty()) ){
				return;
			}
			glm::vec3& pos = ecs::get<Position>(phys.id);
			glm::ivec2 minima = glm::floor( (glm::vec2(pos) - phys.bbox_dims) / collision_map.scale );
			glm::ivec2 maxima = glm::ceil( (glm::vec2(pos) + phys.bbox_dims) / collision_map.scale );
			collision_map.mark(minima, maxima, &phys);
		}
	);
	//for (auto pair : collision_map) {
	//	printf("(%d,%d) %d", pair.first.x, pair.first.y, pair.second.size());
	//}
}

void Physics::set_drag(float new_drag) {
	drag = new_drag;
}

void Physics::set_gravity(float new_gravity) {
	gravity = new_gravity;
}



float Physics::drag    = 1;
float Physics::gravity = 0;
CollisionMap Physics::collision_map = CollisionMap();



void Sprite::setup() {
	if (is_setup) {
		return;
	}
	vao     = new VAO;
	pos     = new Buffer<glm::vec3>;
	uv      = new Buffer<glm::vec2>;

	std::vector<glm::vec3> const quad_pos_cpu = {
		{-1.0,-1.0,0.0}, {1.0,1.0,0.0}, {1.0,-1.0,0.0},
		{-1.0,-1.0,0.0}, {-1.0,1.0,0.0}, {1.0,1.0,0.0},
	};

	// The texture positions of the quad's vertexes
	std::vector<glm::vec2> const quad_uv_cpu = {
		{0.0,1.0}, {1.0,0.0}, {1.0,1.0},
		{0.0,1.0}, {0.0,0.0}, {1.0,0.0},
	};

	VAO::BindGuard guard(*vao);

	pos->set_data(quad_pos_cpu,GL_STATIC_DRAW);
	uv ->set_data(quad_uv_cpu,GL_STATIC_DRAW);

	(*vao)[0].enable();
	(*vao)[0] = *pos;
	(*vao)[1].enable();
	(*vao)[1] = *uv;

}



Sprite::Sprite(size_t id,std::shared_ptr<Texture> tex, glm::vec2 scale)
	: Component(id)
	, tex(tex)
	, scale(scale)
	, program(ProgramCache::load("shaders/sprite.vert","shaders/sprite.frag"))
	, screenlock(false)
{
	setup();
}

Sprite::Sprite(size_t id,std::shared_ptr<Texture> tex, glm::vec2 scale, std::shared_ptr<GPUProgram> program)
	: Component(id)
	, tex(tex)
	, scale(scale)
	, program(program)
{
	setup();
}

Sprite::Sprite(
	size_t id,
	std::shared_ptr<Texture> tex,
	glm::vec2 scale,
	std::shared_ptr<GPUProgram> program,
	std::function<void(std::shared_ptr<GPUProgram>)> uniform_callback
)
	: Component(id)
	, tex(tex)
	, scale(scale)
	, program(program)
	, uniform_callback(uniform_callback)
{
	setup();
}

void Sprite::fixed_update() {
	glm::vec2 pos = ecs::get<Position>(id).position;
	glBindTexture(GL_TEXTURE_2D, *tex);
	glUseProgram(*program);
	(*program)["offset"] = pos;
	(*program)["depth"]  = depth;
	(*program)["scale"]  = scale;
	(*program)["tex"]    = 0;
	if (screenlock) {
		(*program)["cam_pos"]= glm::vec2(0,0);
	}
	else {
		(*program)["cam_pos"]= cam_pos;
	}
	if (uniform_callback) {
		uniform_callback(program);
	}
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

GPUProgram &Sprite::get_program() {
	return *program;
}

VAO &Sprite::get_vao() {
	return *vao;
}

bool               Sprite::is_setup = false;
VAO               *Sprite::vao      = nullptr;
Buffer<glm::vec3> *Sprite::pos      = nullptr;
Buffer<glm::vec2> *Sprite::uv       = nullptr;
glm::vec2          Sprite::cam_pos  = glm::vec2(0, 0);


AI::AI (size_t id, void(*d_update)(size_t,float,void*),void* data)
	: Component(id)
	, d_update(d_update)
	, data(data)
{}

void AI::delta_update(float delta) {
	d_update(id,delta,data);
}



Status::Status(int id, int health, int armor)
	: Component(id)
	, health(health)
	, armor(armor)
	, alignment(NEUTRAL)
{}


// Make the sets for our component types exist	
template<> std::vector<ecs::ComponentSet<Position>::Entry> ecs::ComponentSet<Position> ::data = std::vector<ecs::ComponentSet<Position>::Entry>();
template<> std::vector<ecs::ComponentSet<Physics>::Entry>  ecs::ComponentSet<Physics> ::data = std::vector<ecs::ComponentSet<Physics>::Entry>();
template<> std::vector<ecs::ComponentSet<Sprite>::Entry>   ecs::ComponentSet<Sprite>  ::data = std::vector<ecs::ComponentSet<Sprite>::Entry>();
template<> std::vector<ecs::ComponentSet<AI>::Entry>       ecs::ComponentSet<AI>      ::data = std::vector<ecs::ComponentSet<AI>::Entry>();
template<> std::vector<ecs::ComponentSet<Status>::Entry>   ecs::ComponentSet<Status>  ::data = std::vector<ecs::ComponentSet<Status>::Entry>();


