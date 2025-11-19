#version 450

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outColor;

layout(binding = 0) uniform Uniforms {
    vec2 resolution;
    float time;
    float sunEnabled;
    vec3 cameraPos;
    float padding2;
    vec3 cameraDir;
    float padding3;
} ubo;

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct HitInfo {
    bool hit;
    float dist;
    vec3 point;
    vec3 normal;
    vec3 matColor;
    float reflectivity;
};

struct Sphere {
    vec3 center;
    float radius;
    vec3 color;
    float roughness;
};

struct PointLight {
    vec3 position;
    float intensity;
    vec3 color;
    float padding;
};

struct SpotLight {
    vec3 position;
    float intensity;
    vec3 direction;
    float cutOff;
    vec3 color;
    float outerCutOff;
};

layout(std140, binding = 1) readonly buffer SceneBuffer {
    Sphere spheres[100];
    PointLight pointLight;
    SpotLight spotLight;
    int sphereCount;
    vec3 sunDirection;
} scene;

HitInfo traceScene(Ray ray) {
    HitInfo closestHit;
    closestHit.hit = false;
    closestHit.dist = 1e30;
    closestHit.reflectivity = 0.0;

    // Check Spheres
    for (int i = 0; i < scene.sphereCount; i++) {
        Sphere s = scene.spheres[i];
        vec3 oc = ray.origin - s.center;
        float b = dot(oc, ray.direction);
        float c = dot(oc, oc) - s.radius * s.radius;
        float h = b * b - c;

        if (h > 0.0) {
            float t = -b - sqrt(h);
            if (t > 0.001 && t < closestHit.dist) {
                closestHit.hit = true;
                closestHit.dist = t;
                closestHit.point = ray.origin + ray.direction * t;
                closestHit.normal = normalize(closestHit.point - s.center);
                closestHit.matColor = s.color;
                closestHit.reflectivity = 1.0 - s.roughness;
            }
        }
    }

    // Check Point Light (Visual Representation)
    {
        vec3 oc = ray.origin - scene.pointLight.position;
        float b = dot(oc, ray.direction);
        float c = dot(oc, oc) - 0.1 * 0.1; // Small radius 0.1
        float h = b * b - c;
        if (h > 0.0) {
            float t = -b - sqrt(h);
            if (t > 0.001 && t < closestHit.dist) {
                closestHit.hit = true;
                closestHit.dist = t;
                closestHit.point = ray.origin + ray.direction * t;
                closestHit.normal = normalize(closestHit.point - scene.pointLight.position);
                closestHit.matColor = scene.pointLight.color * 10.0; // Emissive
                closestHit.reflectivity = 0.0;
            }
        }
    }

    // Check Spot Light (Visual Representation)
    {
        vec3 oc = ray.origin - scene.spotLight.position;
        float b = dot(oc, ray.direction);
        float c = dot(oc, oc) - 0.1 * 0.1; // Small radius 0.1
        float h = b * b - c;
        if (h > 0.0) {
            float t = -b - sqrt(h);
            if (t > 0.001 && t < closestHit.dist) {
                closestHit.hit = true;
                closestHit.dist = t;
                closestHit.point = ray.origin + ray.direction * t;
                closestHit.normal = normalize(closestHit.point - scene.spotLight.position);
                closestHit.matColor = scene.spotLight.color * 10.0; // Emissive
                closestHit.reflectivity = 0.0;
            }
        }
    }

    return closestHit;
}

void main() {
    // Correct aspect ratio
    vec2 uv = inUV * 2.0 - 1.0;
    float aspect = ubo.resolution.x / ubo.resolution.y;
    vec2 screenCoord = vec2(uv.x * aspect, -uv.y);

    // Camera Setup
    vec3 camPos = ubo.cameraPos;
    vec3 forward = normalize(ubo.cameraDir);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), forward));
    vec3 up = cross(forward, right);

    vec3 rayDir = normalize(forward * 1.5 + right * screenCoord.x + up * screenCoord.y);

    Ray ray = Ray(camPos, rayDir);
    vec3 finalColor = vec3(0.0);
    vec3 throughput = vec3(1.0);

    vec3 lightDir = normalize(scene.sunDirection);

    // Ray Bounce Loop
    for (int bounce = 0; bounce < 3; bounce++) {
        HitInfo hit = traceScene(ray);

        if (hit.hit) {
            // If it's an emissive object (light source representation), just return color
            if (length(hit.matColor) > 2.0) {
                finalColor += hit.matColor * throughput;
                break;
            }

            vec3 totalLight = vec3(0.0);
            float ambient = 0.1;

            // 1. Directional Light (Sun)
            if (ubo.sunEnabled > 0.5) {
                float diff = max(dot(hit.normal, lightDir), 0.0);
                Ray shadowRay = Ray(hit.point + hit.normal * 0.001, lightDir);
                float shadow = 1.0;
                HitInfo shadowHit = traceScene(shadowRay);
                if (shadowHit.hit && length(shadowHit.matColor) <= 2.0) { shadow = 0.1; } // Don't shadow if hit light source
                totalLight += hit.matColor * (diff * shadow);
            }

            // 2. Point Light
            {
                vec3 L = normalize(scene.pointLight.position - hit.point);
                float dist = length(scene.pointLight.position - hit.point);
                float attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
                float diff = max(dot(hit.normal, L), 0.0);
                
                Ray shadowRay = Ray(hit.point + hit.normal * 0.001, L);
                float shadow = 1.0;
                HitInfo shadowHit = traceScene(shadowRay);
                if (shadowHit.hit && shadowHit.dist < dist && length(shadowHit.matColor) <= 2.0) { shadow = 0.1; }
                
                totalLight += hit.matColor * scene.pointLight.color * scene.pointLight.intensity * diff * attenuation * shadow;
            }

            // 3. Spot Light
            {
                vec3 L = normalize(scene.spotLight.position - hit.point);
                float dist = length(scene.spotLight.position - hit.point);
                float attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
                
                float theta = dot(L, normalize(-scene.spotLight.direction));
                float epsilon = scene.spotLight.cutOff - scene.spotLight.outerCutOff;
                float intensity = clamp((theta - scene.spotLight.outerCutOff) / epsilon, 0.0, 1.0);

                if (intensity > 0.0) {
                    float diff = max(dot(hit.normal, L), 0.0);
                    
                    Ray shadowRay = Ray(hit.point + hit.normal * 0.001, L);
                    float shadow = 1.0;
                    HitInfo shadowHit = traceScene(shadowRay);
                    if (shadowHit.hit && shadowHit.dist < dist && length(shadowHit.matColor) <= 2.0) { shadow = 0.1; }
                    
                    totalLight += hit.matColor * scene.spotLight.color * scene.spotLight.intensity * diff * attenuation * intensity * shadow;
                }
            }

            // Add Ambient
            totalLight += hit.matColor * ambient;

            finalColor += totalLight * throughput * (1.0 - hit.reflectivity);
            throughput *= hit.reflectivity;

            // Prepare next ray (Reflection)
            ray.origin = hit.point + hit.normal * 0.001;
            ray.direction = reflect(ray.direction, hit.normal);
        } else {
            // Sky Color
            float t = 0.5 * (ray.direction.y + 1.0);
            vec3 sky = mix(vec3(0.5, 0.7, 1.0), vec3(0.1, 0.1, 0.2), t);
            finalColor += sky * throughput;
            break;
        }
    }

    // Gamma Correction
    finalColor = pow(finalColor, vec3(1.0/2.2));

    outColor = vec4(finalColor, 1.0);
}
