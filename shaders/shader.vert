#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inTexCoord;
// layout(location = 3) in uint inTexIndex;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragTexCoord;
// layout(location = 2) out flat uint texIndex;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

    fragColor = inColor;
    fragTexCoord = inTexCoord;
    // texIndex = inTexIndex;
}