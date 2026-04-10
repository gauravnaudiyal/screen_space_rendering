#version 330 core
/**
 * Screen-Space Ray Tracing — Fragment Shader
 *
 * Based on: McGuire, M., Mara, M. (2014).
 * "Efficient GPU Screen-Space Ray Tracing"
 * Journal of Computer Graphics Techniques (JCGT), 3(4), 73-85.
 * DOI: 10.2312/jcgt.20143124
 *
 * Algorithm summary:
 *  1. Reconstruct view-space position from G-Buffer
 *  2. Compute reflection direction using surface normal
 *  3. March ray in view space, project each step to screen UV
 *  4. Compare ray depth against G-Buffer depth for intersection
 *  5. On coarse hit: binary search refine for accurate position
 *  6. Sample colour at hit UV as reflection result
 *  7. Fade at screen edges to hide missing data artifacts
 */
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main() {
    TexCoords   = aTexCoords;
    gl_Position = vec4(aPos, 1.0);
}