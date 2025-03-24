

#include "common.h"
#include "glazy_program.h"

std::shared_ptr<Texture> TextureCache::load(std::string file_path, bool mipmap) {
	if (textures.find(file_path) == textures.end()) {
		textures[file_path] = std::shared_ptr<Texture>(new Texture(file_path, mipmap));
	}
	return textures[file_path];
}

std::unordered_map<std::string, std::shared_ptr<Texture>> TextureCache::textures = std::unordered_map<std::string, std::shared_ptr<Texture>>();

bool      mouse_on_screen;
glm::vec2 mouse_position;
std::unordered_map<size_t, std::function<void(glm::vec2)>> mouseclick_callbacks;


std::shared_ptr<GPUProgram> ProgramCache::load(std::string vertex, std::string fragment) {
	std::string key = vertex + '\n' + fragment;
	if (programs.count(key) == 0) {
		auto vertex_shader   = Shader<GL_VERTEX_SHADER>::from_file(vertex);
		auto fragment_shader = Shader<GL_FRAGMENT_SHADER>::from_file(fragment);
		programs[key] = std::shared_ptr<GPUProgram>(new GPUProgram(vertex_shader, {}, {}, {}, fragment_shader));
	}
	return programs[key];
}

std::unordered_map<std::string, std::shared_ptr<GPUProgram>> ProgramCache::programs = std::unordered_map<std::string, std::shared_ptr<GPUProgram>>();



