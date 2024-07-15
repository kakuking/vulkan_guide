#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

// layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 1) uniform sampler2D textures[];

void main() {
    // vec3 tempColor = vec3(fragTexCoord, 1.0);       // Multiples teh texture with the frag coordinates
    // outColor = vec4(tempColor * texture(texSampler, fragTexCoord).rgb, 1.0);
    uint index = int(fragTexCoord.z);
    vec4 texColor = texture(textures[nonuniformEXT(index)], fragTexCoord.xy);
    outColor = texColor;
}