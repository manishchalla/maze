
#include "quad.h"
#include <glm/gtc/type_ptr.hpp>

Wall::Wall(glm::vec2 pos, std::shared_ptr<Texture> tex, glm::vec2 scale)
	: CollisionQuad(pos, tex, scale)
{}

Wall::Wall(glm::vec2 pos, std::shared_ptr<Texture> tex, glm::vec2 scale, std::shared_ptr<GPUProgram> program)
	: CollisionQuad(pos, tex, scale)
{
	Sprite& sprite = ecs::get<Sprite>(id);
	sprite.program = program;
}


template<>
Wall *deserialize(std::string text) {
	std::stringstream stream(text);
	std::string token;
	if ((stream >> token) && (token == "wall")) {
		glm::vec2 pos;
		glm::vec2 scale;
		if (!(stream >> pos.x >> pos.y >> scale.x >> scale.y >> token)) {
			return nullptr;
		}
		std::string tex_path = "assets/" + token;
		std::string vertex, fragment;
		if (stream >> vertex >> fragment) {
			std::shared_ptr<GPUProgram> program = ProgramCache::load(vertex, fragment);
			return new Wall(pos, TextureCache::load(tex_path,false), scale, program);
		}
		else {
			return new Wall(pos, TextureCache::load(tex_path,false), scale);
		}
	}
	return nullptr;
}




Background::Background(glm::vec2 pos, std::shared_ptr<Texture> tex, glm::vec2 scale)
	: Quad(pos, tex, scale)
{}

Background::Background(glm::vec2 pos, std::shared_ptr<Texture> tex, glm::vec2 scale, std::shared_ptr<GPUProgram> program)
	: Quad(pos, tex, scale)
{
	Sprite& sprite = ecs::get<Sprite>(id);
	sprite.program = program;
}

template<>
Background* deserialize(std::string text) {
	std::stringstream stream(text);
	std::string token;
	if ((stream >> token) && (token == "back")) {
		glm::vec2 pos;
		glm::vec2 scale;
		if(!(stream >> pos.x >> pos.y >> scale.x >> scale.y >> token)) {
			return nullptr;
		}
		std::string tex_path = "assets/" + token;
		std::string vertex, fragment;
		if (stream >> vertex >> fragment) {
			std::shared_ptr<GPUProgram> program = ProgramCache::load(vertex, fragment);
			return new Background(pos, TextureCache::load(tex_path,false), scale, program);
		}
		else {
			return new Background(pos, TextureCache::load(tex_path,false), scale);
		}
	}
	return nullptr;
}

void TextBox::set_letter(int col, int row, int codepoint) {
	data[10 + row * dims.x + col] = codepoint;
}
	
GLint TextBox::get_letter(int col, int row) {
	return data[10 + row * dims.x + col];
}


GLint TextBox::convert_symbol(char const*& iter) {
	int tail_count = 0;
	int result = 0;
	if ( (*iter & 0x80) == 0 ) {
		result = *iter;
	}
	else if ( (*iter & 0xE0) == 0xC0) {
		tail_count = 1;
		result = *iter & 0x1F;
	}
	else if ((*iter & 0xF0) == 0xE0) {
		tail_count = 2;
		result = *iter & 0x0F;
	}
	else if ((*iter & 0xF8) == 0xF0) {
		tail_count = 3;
		result = *iter & 0x07;
	}
	else {
		throw std::runtime_error("Unicode conversion failure.");
	}
	iter++;
	for (int i = 0; i < tail_count; i++) {
		if ((*iter & 0xC0) == 0x80) {
			result <<= 6;
			result |= *iter & 0x3F;
		}
		else {
			throw std::runtime_error("Unicode conversion failure.");
		}
		iter++;
	}
	return result;
}

void TextBox::handle_wrap(int &col, int&row) {
	if ((col != 0) || (row == 0)) {
		return;
	}
	if (row >= dims.y) {
		return;
	}
	int start_col = dims.x-1;
	int start_row = row-1;
	if (get_letter(start_col,start_row) == ' ') {
		return;
	}
	while ((start_col >= 0) && (get_letter(start_col,start_row) != ' ')) {
		start_col--;
	}
	if (start_col == -1) {
		return;
	}
	start_col++;
	int transfer_count = dims.x - start_col;
	for (int i = 0; i < transfer_count; i++) {
		set_letter(i,row,get_letter(start_col+i,start_row));
		set_letter(start_col + i, start_row, ' ');
	}
	col += transfer_count;
}

void TextBox::convert_text(bool wrap) {
	data.resize(10 + dims.x * dims.y,' ');
	memcpy(data.data(),   glm::value_ptr(forecolor), sizeof(GLfloat) * 4);
	memcpy(data.data()+4, glm::value_ptr(backcolor), sizeof(GLfloat) * 4);
	data[8] = dims.x;
	data[9] = dims.y;
	int row = 0;
	int col = 0;
	char const* iter = text.data();
	char const* end = text.data() + text.size();
	while (iter != end) {
		int codepoint = convert_symbol(iter);
		if (codepoint == '\n') {
			col+= dims.x;
		}
		else if (codepoint == '\t') {
			col += 4;
		}
		else {
			if (col >= dims.x) {
				col = 0;
				row++;
			}
			if (row >= dims.y) {
				break;
			}
			if (wrap) {
				handle_wrap(col, row);
			}
			if ((codepoint != ' ') || (col != 0)) {
				set_letter(col,row,codepoint);
				col++;
			}
		}
	}
}

void TextBox::setup() {
	if (is_setup) {
		return;
	}
	is_setup = true;
	std::shared_ptr<GPUProgram> program = ProgramCache::load("shaders/text.vert","shaders/text.frag");
	glUniformBlockBinding(*program,0,0);
}

TextBox::TextBox(glm::vec2 pos, glm::vec2 scale, glm::ivec2 dims, std::string text)
	: Background(pos, TextureCache::load("assets/unifont.png",false), scale)
	, text(text)
	, dims(dims)
	, modified(true)
	, forecolor(0,0,0,1)
	, backcolor(1,1,1,1)
{
	Sprite& sprite = ecs::get<Sprite>(id);
	setup();
	sprite.program = ProgramCache::load("shaders/text.vert","shaders/text.frag");
	convert_text(true);
	sprite.uniform_callback = [this](std::shared_ptr<GPUProgram> program) {
		safety::entry_guard("Textbox Uniform Callback");
		if (modified) {
			buff.set_data(data,GL_STATIC_DRAW);
			modified = false;
		}
		glBindBuffer(GL_UNIFORM_BUFFER, buff);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, buff);
		safety::exit_guard("Textbox Uniform Callback");
	};
}

void TextBox::set_text(std::string text, bool wrap) {
	safety::entry_guard("TextBox::set_text");
	this->text = text;
	convert_text(true);
	modified = true;
	safety::exit_guard("TextBox::set_text");
}

void TextBox::set_foreground(glm::vec4 color) {
	forecolor = color;
	modified = true;
}

void TextBox::set_background(glm::vec4 color) {
	backcolor = color;
	modified = true;
}


template<>
TextBox* deserialize(std::string text) {
	std::stringstream stream(text);
	std::string token;
	if ((stream >> token) && (token == "text")) {
		glm::vec2 pos;
		glm::vec2 scale;
		glm::ivec2 dims;
		std::string token;
		if(!(stream >> pos.x >> pos.y >> scale.x >> scale.y >> dims.x >> dims.y >> token)) {
			return nullptr;
		}
		std::string text;
		std::getline(stream, text);
		text = token + text;
		return new TextBox(pos, scale, dims, text);
	}
	return nullptr;
}

bool TextBox::is_setup = false;


