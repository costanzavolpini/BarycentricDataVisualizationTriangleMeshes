#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec4 color[3]; // an array of 3 vectors of size 4 (since it is a triangle)
out vec3 coords;
out vec4 wedge_color[3]; // an array of 3 vectors of size 4 (since it is a triangle)

void main()
{
	wedge_color[0] = color[0];
	wedge_color[1] = color[1];
	wedge_color[2] = color[2];

	coords = vec3(1.0, 0.0, 0.0);
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

	coords = vec3(0.0, 1.0, 0.0);
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

	coords = vec3(0.0, 0.0, 1.0);
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();

    EndPrimitive();
}