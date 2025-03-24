
#include "glazy_texture.h"

#define STB_IMAGE_IMPLEMENTATION 
#include <stb_image.h>




namespace glazy {


	Texture::Texture(std::string file_path, bool mipmap) {
		// Load image data, width, and height
		int width, height, n;
		unsigned char* data = stbi_load(file_path.c_str(), &width, &height, &n, 4);
		if (data == nullptr) {
			throw std::runtime_error("Failed to load texture!");
		}
		construct((Texture::RGBA8*)data, width, height, mipmap);
		delete[] data;
	}

	
	Texture::Texture(std::vector<std::string> file_paths, bool mipmap) {
		// Load image data, width, and height
		std::vector<Texture::RGBA8*> file_data;
		int width, height, n;
		unsigned char* data = stbi_load(file_paths[0].c_str(), &width, &height, &n, 4);
		if (data == nullptr) {
			delete[] data;
			throw std::runtime_error("Failed to load texture!");
		}
		file_data.push_back((Texture::RGBA8*)data);
		for (size_t i = 1; i < file_paths.size(); i++) {
			int other_width, other_height;
			unsigned char *other_data =  stbi_load(file_paths[i].c_str(), &other_width, &other_height, &n, 4);
			if (other_data == nullptr) {
				for (Texture::RGBA8* tex_data : file_data) {
					delete[] data;
					delete[] tex_data;
				}
				throw std::runtime_error("Failed to load texture!");
			}
			file_data.push_back((Texture::RGBA8*)data);
		}
		construct((Texture::RGBA8*)data, width, height, file_paths.size(), mipmap);
	}
	
	
	Texture::Texture(std::vector<Texture::RGBA8>& data, size_t width, size_t height, bool mipmap) {
		construct(data.data(), width, height, mipmap);
	}


	void Texture::construct(Texture::RGBA8* data, size_t width, size_t height, bool mipmap) {
		safety::entry_guard("Texture::Texture");
		id = 0;
		glGenTextures(1, &id);
		if (id == 0) {
			throw std::runtime_error("Failed to allocate texture id.");
		}
		glBindTexture(GL_TEXTURE_2D, id);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		if (mipmap) {
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
		glBindTexture(GL_TEXTURE_2D, 0);
		safety::exit_guard("Texture::Texture");
	}
	
	void Texture::construct(Texture::RGBA8* data, size_t width, size_t height, size_t count, size_t mip_count) {
		safety::entry_guard("Texture::Texture");

		Texture::RGBA8 *combined_data = new Texture::RGBA8[width * height * count];

		id = 0;
		glGenTextures(1, &id);
		if (id == 0) {
			throw std::runtime_error("Failed to allocate texture id.");
		}


		glBindTexture(GL_TEXTURE_2D_ARRAY, id);

		GLsizei mip_level_count = (mip_count == 0) ? 1 : mip_count;
		// Allocate the storage.
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, mip_level_count, GL_RGBA8, width, height, count);
		// Upload pixel data.
		// The first 0 refers to the mipmap level (level 0, since there's only 1)
		// The following 2 zeroes refers to the x and y offsets in case you only want to specify a subrectangle.
		// The final 0 refers to the layer index offset (we start from index 0 and have 2 levels).
		// Altogether you can specify a 3D box subset of the overall texture, but only one mip level at a time.
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, count, GL_RGBA, GL_UNSIGNED_BYTE, data);
		
		glTexImage2D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		if (mip_count != 0) {
			glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}
		else {
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		
		delete[] combined_data;
		safety::exit_guard("Texture::Texture");
	}

	Texture::~Texture() {
		glDeleteTextures(1, &id);
	}

	Texture::operator GLuint() {
		return id;
	}


	Sampler::Sampler() {
		glGenSamplers(1, &id);
	}

	Sampler::~Sampler() {
		glDeleteSamplers(1, &id);
	}

}
