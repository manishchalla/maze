#ifndef COMMON
#define COMMON

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glad.h>
#include <glfw3.h>

#include <cmath>

#include "glazy.h"
#include "shape.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <algorithm>
#include <thread>
#include <atomic>
#include <limits>
#include <sstream>



using namespace glazy;


template<typename T>
std::string serialize(T obj);

template<typename T>
T* deserialize(std::string text) {
	return nullptr;
}

extern std::unordered_map<size_t, std::function<void(glm::vec2)>> mouseclick_callbacks;

class TextureCache {
	static std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
public:
	static std::shared_ptr<Texture> load(std::string file_path, bool mipmap);
};



class ProgramCache {
	static std::unordered_map<std::string, std::shared_ptr<GPUProgram>> programs;
public:

	static std::shared_ptr<GPUProgram> load(std::string vertex, std::string fragment);
};


extern bool      mouse_on_screen;
extern glm::vec2 mouse_position;

#endif
