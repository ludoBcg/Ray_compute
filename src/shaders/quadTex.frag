// Fragment shader
#version 430

// image2D input
// declared as GL_RGBA8 (UNSIGNED_BYTE) in c++ code -> rgba8 in compute shader 
// https://www.khronos.org/opengl/wiki/Image_Load_Store#Format_qualifiers
uniform layout( rgba8, binding=0) image2D u_screenTex;

	
// INPUT	
in vec3 vert_uv;

// OUTPUT
out vec4 frag_color;


// MAIN
void main()
{
		// final color
		vec4 color = vec4(1.0f);
		
		// fetch image dimensions
		ivec2 dims = imageSize(u_screenTex); 
		//read color from image
		color.rgb = imageLoad(u_screenTex, ivec2(vert_uv.x * dims.x, vert_uv.y * dims.y) ).rgb; // for image2D input
		
		frag_color = color;
}

