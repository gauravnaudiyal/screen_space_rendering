#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform mat4      projection;
uniform bool      enableSSR;
uniform int       debugMode;   // 0=SSR, 1=position, 2=normal, 3=albedo

// ── McGuire & Mara 2014 Parameters ───────────────────────────────────────────
const int   MAX_STEPS        = 64;
const int   BINARY_STEPS     = 8;    // refinement steps after hit found
const float STEP_SIZE        = 0.2;
const float THICKNESS        = 0.5;
const float MAX_DISTANCE     = 30.0;
const float JITTER           = 0.25;

vec2 projectToUV(vec3 viewPos) {
    vec4 clip = projection * vec4(viewPos, 1.0);
    vec3 ndc  = clip.xyz / clip.w;
    return ndc.xy * 0.5 + 0.5;
}

bool isValidUV(vec2 uv) {
    return uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0;
}

// ── Binary search refinement — McGuire & Mara 2014 ───────────────────────────
// Once a coarse hit is found, step back and binary search for the exact
// intersection point, improving reflection accuracy significantly
vec2 binarySearch(vec3 rayPos, vec3 stepVec) {
    for (int i = 0; i < BINARY_STEPS; i++) {
        stepVec *= 0.5;
        vec2 uv      = projectToUV(rayPos);
        float sceneZ = texture(gPosition, uv).z;

        if (rayPos.z < sceneZ)
            rayPos -= stepVec;   // overshot — step back
        else
            rayPos += stepVec;   // undershot — step forward
    }
    return projectToUV(rayPos);
}

// ── Main ray march ────────────────────────────────────────────────────────────
bool traceRay(vec3 origin, vec3 dir, out vec2 hitUV) {
    vec3 step   = normalize(dir) * STEP_SIZE;
    vec3 rayPos = origin + step * JITTER;

    for (int i = 0; i < MAX_STEPS; i++) {
        rayPos += step;

        if (rayPos.z > 0.0) return false;
        if (length(rayPos - origin) > MAX_DISTANCE) return false;

        vec2 uv = projectToUV(rayPos);
        if (!isValidUV(uv)) return false;

        float sceneZ = texture(gPosition, uv).z;
        if (sceneZ == 0.0) continue;

        if (rayPos.z < sceneZ && rayPos.z > sceneZ - THICKNESS) {
            // Coarse hit found — refine with binary search
            hitUV = binarySearch(rayPos, step);
            return true;
        }
    }
    return false;
}

// ── Screen edge fade ──────────────────────────────────────────────────────────
float edgeFade(vec2 uv) {
    vec2 fade = smoothstep(0.0, 0.1, uv) * smoothstep(1.0, 0.9, uv);
    return fade.x * fade.y;
}

void main() {
    vec3  fragPos      = texture(gPosition,    TexCoords).xyz;
    vec3  normal       = normalize(texture(gNormal, TexCoords).xyz);
    vec4  albedoSpec   = texture(gAlbedoSpec,  TexCoords);
    vec3  albedo       = albedoSpec.rgb;
    float reflectivity = albedoSpec.a;

    // ── G-Buffer debug visualisation ──────────────────────────────────────────
    if (debugMode == 1) { FragColor = vec4(abs(fragPos) * 0.1, 1.0); return; }
    if (debugMode == 2) { FragColor = vec4(normal * 0.5 + 0.5,  1.0); return; }
    if (debugMode == 3) { FragColor = vec4(albedo, 1.0);               return; }

    // Simple diffuse lighting
    vec3 lightDir = normalize(vec3(1.0, 2.0, 1.0));
    float diff    = max(dot(normal, lightDir), 0.0);
    vec3 lighting = albedo * (0.3 + 0.7 * diff);

    if (!enableSSR || reflectivity < 0.01 || fragPos == vec3(0.0)) {
        FragColor = vec4(lighting, 1.0);
        return;
    }

    // Reflection ray
    vec3 viewDir    = normalize(fragPos);
    vec3 reflectDir = reflect(viewDir, normal);

    vec2 hitUV;
    vec3 reflColor = vec3(0.05);

    if (traceRay(fragPos, reflectDir, hitUV)) {
        vec3  hitAlbedo = texture(gAlbedoSpec, hitUV).rgb;
        vec3  hitNormal = normalize(texture(gNormal, hitUV).xyz);
        float hitDiff   = max(dot(hitNormal, lightDir), 0.0);
        reflColor       = hitAlbedo * (0.3 + 0.7 * hitDiff);
    }

    float fade      = edgeFade(TexCoords);
    vec3 finalColor = mix(lighting, reflColor, reflectivity * fade);
    FragColor       = vec4(finalColor, 1.0);
}