#version 450
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUV;

struct Vertex {
	vec3 position;
	float uvX;
	vec3 normal;
	float uvY;
	vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer{
	Vertex vertices[];
};

layout(push_constant) uniform constants{
	mat4 renderMatrix;
	VertexBuffer vertexBuffer;
} PushConstants;


void main() 
{
	//const array of positions for the triangle
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

	//output the position of each vertex
	gl_Position = PushConstants.renderMatrix * vec4(v.position, 1.0f);
	outColor = v.color.xyz;
	outUV = vec2(v.uvX, v.uvY);
}
