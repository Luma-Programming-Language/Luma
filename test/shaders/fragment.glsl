#version 330 core
in vec3 vNormal;
uniform vec3 uTint;
out vec4 FragColor;

void main() {
    vec3 n = normalize(vNormal);
    vec3 lightDir = normalize(vec3(0.55, 1.0, 0.4));
    float diff = max(dot(n, lightDir), 0.0);
    vec3 color = uTint * (0.28 + 0.72 * diff);
    FragColor = vec4(color, 1.0);
}