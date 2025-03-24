
#ifndef QUAD
#define QUAD

#include "level.h"


struct Wall
	: CollisionQuad
{
	Wall(glm::vec2 pos, std::shared_ptr<Texture> tex, glm::vec2 scale);
	Wall(glm::vec2 pos, std::shared_ptr<Texture> tex, glm::vec2 scale, std::shared_ptr<GPUProgram> program);
};

template<>
Wall* deserialize(std::string text);

struct Background
	: Quad
{
	Background(glm::vec2 pos, std::shared_ptr<Texture> tex, glm::vec2 scale);
	Background(glm::vec2 pos, std::shared_ptr<Texture> tex, glm::vec2 scale, std::shared_ptr<GPUProgram> program);
};

template<>
Background* deserialize(std::string text);


class TextBox
	: public Background
{

	static bool is_setup;
	bool modified;
	glm::ivec2  dims;
	std::string text;
	std::vector<GLint> data;
	Buffer<GLint> buff;
	glm::vec4 forecolor;
	glm::vec4 backcolor;

	void set_letter(int col, int row, GLint codepoint);
	GLint get_letter(int col, int row);
	GLint convert_symbol(char const*& iter);
	void handle_wrap(int& row, int& col);
	void convert_text(bool wrap);
	static void setup();

public:

	TextBox(glm::vec2 pos, glm::vec2 scale, glm::ivec2 dims, std::string text);
	void set_text(std::string text, bool wrap);
	void set_foreground(glm::vec4 color);
	void set_background(glm::vec4 color);
};


template<>
TextBox* deserialize(std::string text);

#endif
