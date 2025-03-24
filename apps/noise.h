
#ifndef NOISE
#define NOISE


#include <random>
#include "glazy.h"
#include <glm/gtc/type_ptr.hpp>




template<glm::length_t L>
size_t coords_to_index(glm::vec<L, int> const& coord, glm::vec<L, int> const& dims) {
	size_t mult = 1;
	size_t result = 0;
	int const * coord_ptr = glm::value_ptr(coord);
	int const * dim_ptr = glm::value_ptr(dims);
	for (int i = 0; i < L; i++) {
		result += coord_ptr[i] * mult;
		mult *= dim_ptr[i];
	}
	return result;
}


template<glm::length_t L>
struct VectorGrid {
	
	glm::vec<L, int> dims;
	std::vector<glm::vec<L, float>> grid;

	VectorGrid(glm::vec<L, int> dims)
		: dims(dims)
	{
		grid.resize(coords_to_index(dims-1, dims)+1);
	}

	glm::vec<L, float>& operator[] (glm::vec<L,int> coordinates) {
		glm::vec<L, int> coords = ((coordinates % dims) + dims) % dims;
		size_t index = coords_to_index(coords,dims);
		return grid[index];
	}
};

template<glm::length_t L>
glm::vec<L, float> random_vec(std::mt19937 & gen) {
	glm::vec<L, float> result;
	for (glm::length_t i = 0; i < L; i++) {
		glm::value_ptr(result)[i] = (float) (int) (gen() - 0x80000000);
	}
	return result;
}

template<glm::length_t L>
glm::vec<L, float> random_unit(std::mt19937 & gen) {
	return random_vec<L>(gen) / (float) 0x80000000;
}

template<glm::length_t L>
glm::vec<L, float> random_norm(std::mt19937 & gen) {
	glm::vec<L,float> result = glm::normalize(random_vec<L>(gen));
	return result;
}


template<glm::length_t L>
struct PerlinMap {
	VectorGrid<L> grid;

	PerlinMap(glm::vec<L,int> dims, size_t seed)
		: grid(dims)
	{
		std::mt19937 gen(seed);
		for (auto& vec : grid.grid) {
			vec = random_norm<L>(gen);
		}
	}

	float sample(glm::vec<L, float> coords) {
		coords *= grid.dims;
		int limit = 1;
		for (int i = 0; i < L; i++) {
			limit *= 2;
		}
		glm::vec<L, int> offset;
		int* offset_ptr = glm::value_ptr(offset);
		float acc[L+1];
		float result = 0;
		float total = 0;
		for (int i = 0; i < limit; i++) {
			int residue = i;
			for (int j = 0; j < L; j++) {
				offset_ptr[j] = residue % 2;
				residue /= 2;
			}
			glm::vec<L, int> samp_coord = glm::vec<L, int>(coords) + offset;
			glm::vec<L, float> point_offset = coords - glm::vec<L,float>(samp_coord);
			float val = glm::dot(grid[samp_coord],point_offset);
			for (int j = 0; j < (L+1); j++) {
				if ((i >> j) & 1) {
					val = acc[j] + glm::smoothstep(0.f,1.f, glm::fract(glm::value_ptr(coords)[j])) * (val-acc[j]);
				}
				else {
					acc[j] = val;
					break;
				}
			}
		}
		return acc[L];
	}

};


template<glm::length_t L>
struct WorleyMap {
	VectorGrid<L> grid;

	WorleyMap(glm::vec<L,int> dims,size_t seed)
		: grid(dims)
	{
		std::mt19937 gen(seed);
		for (auto& vec : grid.grid) {
			vec = random_unit<L>(gen);
		}
	}

	float sample(glm::vec<L,float> coords) {
		coords *= grid.dims;
		int limit = 1;
		for (int i = 0; i < L; i++) {
			limit *= 5;
		}
		glm::vec<L, int> offset;
		int* offset_ptr = glm::value_ptr(offset);
		float best = std::numeric_limits<float>::infinity();
		for (int i = 0; i < limit; i++) {
			int residue = i;
			for (int j = 0; j < L; j++) {
				offset_ptr[j] = residue % 5 - 3;
				residue /= 5;
			}
			glm::vec<L, int> samp_coord = glm::vec<L, int>(coords) + offset;
			glm::vec<L,float> pos = glm::vec<L,float>(samp_coord) + ((grid[samp_coord]*0.5f) + 1.f);
			float dist = glm::distance(coords, pos);
			if (dist < best) {
				best = dist;
			}
		}
		return best;
	}

};


#endif NOISE


