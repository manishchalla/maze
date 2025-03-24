#version 410

in  vec2 vuv;

out vec4 pColor;

uniform sampler2D tex;

// Data layout is weird in OpenGL. The best we
// have (that is version 410 compatible) aligns
// all fields to 16 byte boundaries regardless of
// their size. This stinks, but we can keep our
// memory usage efficient by representing integer
// data as ivec4s, which take exactly 16 bytes.
layout (binding = 0, std140) uniform Text {
	vec4 forecolor;
	vec4 backcolor;
	ivec4 data[1024];
} text;


// Extracts the dimensions from the uniform block
ivec2 get_text_dims() {
	return ivec2(text.data[0].x,text.data[0].y);
}

// Extracts the codepoint at the corresponding
// index within the block
int get_letter(int index) {
	index += 2;
	switch(index%4) {
		case  0: return text.data[index/4].x;
		case  1: return text.data[index/4].y;
		case  2: return text.data[index/4].z;
		default: return text.data[index/4].w;
	}
}

void main() {

	// The size of each tile, in pixel coordinates
	ivec2 tile_dims = ivec2(16,16);
	// Dimensions of the texture atlas, in tiles
	ivec2 grid_dims = ivec2(256,256*2);
	// Dimensions of the texture atlas, in pixel coordinates
	ivec2 unifont_dims = grid_dims * tile_dims;
	// Dimensions of text box, as column and row counts
	ivec2 text_dims = get_text_dims();
	// The column and row the fragment falls into
	ivec2 tile_pos = ivec2(text_dims * vuv);	
	// The letter to be used for the fragment's column and row
	int letter = get_letter(text_dims.x*tile_pos.y+tile_pos.x);
	// The minimal x and y coordinates of the sub-texture to be
	// used, in pixel coordinates
	vec2 top_left  = vec2((letter%grid_dims.x),letter/grid_dims.x)*tile_dims;

	// The position of the fragment within its tile, in pixel coordinates
	vec2 pix_local  = (vuv-(vec2(tile_pos)/text_dims)) * text_dims * tile_dims;
	// The position of the fragment in the atlas, in pixel coordinates
	vec2 pix_coords = top_left + pix_local;
	// The position of the fragment in the atlas, in texture coordinates
	vec2 tex_coords = pix_coords / unifont_dims;

	// Fetching the corresponding part of the atlas
	vec4 color = texture(tex,tex_coords).rgba;
	// Switch between foreground and background based upon the brightness
	// of the retrieved sample.
	if(color.r > 0.5) {
		color = text.backcolor;
	} else {
		color = text.forecolor;
	}
	// If the color is really transparent, just discard the fragment
	if(color.a < 0.99){
		discard;
	}
	// Write out the output color
	pColor = color;
}

