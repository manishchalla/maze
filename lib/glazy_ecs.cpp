#include "glazy_ecs.h"


namespace glazy {

	namespace ecs {

		Component::Component(size_t id) : id(id) {}

		void Component::delta_update(float delta) {}

		void Component::fixed_update() {}

		size_t Component::get_id() const {
			return id;
		}

		std::atomic<size_t> Entity::id_counter(0);
	}
}



