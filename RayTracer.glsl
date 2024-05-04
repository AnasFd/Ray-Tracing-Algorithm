#[compute]
#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(set = 0, binding = 0, std430) restrict buffer CameraData {
    mat4 CameraToWorld;
    float CameraFOV;
    float CameraFarPlane;
    float CameraNearPlane;
} 
camera_data;

layout(set = 0, binding = 1, std430) restrict buffer DirectionalLight {
    vec4 data;
} 
directional_light;

layout(rgba32f, binding = 2) uniform image2D rendered_image;

struct MatrixCell {
    int occupied;
    vec3 color;
};

struct Ray {
    vec3 origin;
    vec3 direction;
    vec3 energy;
};

struct RayHit {
    vec3 position;
    float distance;
    vec3 normal;
    vec3 color;
    vec3 specular;
};

const float INF = 99999.0;
const int MAX_REFLECTION_ITERATIONS = 7;
const vec3 sky_color = vec3(0.671, 0.851, 1.0);
const int MATRIX_SIZE = 3;
const float CELL_SIZE = 1;

mat4 BasicProjectionMatrix(float fov_deg, float far_plane, float near_plane)
{
	float S = 1.0 / tan(radians(fov_deg / 2.0));
	float mfbfmn = (-far_plane) / (far_plane - near_plane);
	float mfinbfmn = -(far_plane * near_plane) / (far_plane - near_plane);

	mat4 proj_mat = mat4(
		vec4(S, 0.0, 0.0, 0.0),
		vec4(0.0, S, 0.0, 0.0),
		vec4(0.0, 0.0, mfbfmn, -1.0),
		vec4(0.0, 0.0, mfinbfmn, 0.0)
	);

	return proj_mat;
}

 Ray CreateRay(vec3 origin, vec3 direction) {
    Ray ray;
    ray.origin = origin;
    ray.direction = direction;
    ray.energy = vec3(1.0);
    return ray;
}

Ray CreateCameraRay(vec2 uv) {

    mat4 _CameraToWorld = camera_data.CameraToWorld;
    mat4 _CameraInverseProjection = inverse(BasicProjectionMatrix(camera_data.CameraFOV, camera_data.CameraFarPlane, camera_data.CameraNearPlane));

    // Transform the camera origin to world space
    vec3 origin = _CameraToWorld[3].xyz;

    // Invert the perspective projection of the view-space position
    vec3 direction = (_CameraInverseProjection * vec4(uv, 0.0, 1.0)).xyz;
    // Transform the direction from camera to world space and normalize
    direction = (_CameraToWorld * vec4(direction, 0.0)).xyz;
    direction = normalize(direction);
    return CreateRay(origin, direction);
}

RayHit CreateRayHit() {
    RayHit hit;
    hit.position = vec3(0.0);
    hit.distance = INF;
    hit.normal = vec3(0.0);
    hit.color = vec3(0.0);
    hit.specular = vec3(0.0);
    return hit;
}  

void IntersectGroundPlane(Ray ray, inout RayHit bestHit) {
    // Calculate distance along the ray where the ground plane is intersected
    float t = -ray.origin.y / ray.direction.y;
    if (t > 0 && t < bestHit.distance)
    {
        bestHit.distance = t;
        bestHit.position = ray.origin + t * ray.direction;
        bestHit.normal = vec3(0.0, 1.0, 0.0);
        bestHit.color = vec3(0.8);
        bestHit.specular = vec3(0.5);
    }
}

void IntersectMatrix(Ray ray, inout RayHit bestHit, MatrixCell cell, vec3 cellPosition, float cellSize) {
    vec3 minCorner = cellPosition;
    vec3 maxCorner = cellPosition + vec3(cellSize);

    // Calculate the intersection of the ray with the bounding box of the cell
    vec3 tMin = (minCorner - ray.origin) / ray.direction;
    vec3 tMax = (maxCorner - ray.origin) / ray.direction;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);

    // Check if the ray intersects the cell's bounding box
    if (tNear <= tFar && tFar > 0.0 && tNear < bestHit.distance) {
        // Update the hit information if this intersection is closer than the current best hit
        bestHit.distance = tNear;
        bestHit.position = ray.origin + tNear * ray.direction;
        bestHit.normal = normalize(ray.direction); 
        bestHit.color = cell.color; // Use the color of the cell for shading
        bestHit.specular = vec3(0.6); // Set a default specular value
    }
}

MatrixCell[MATRIX_SIZE][MATRIX_SIZE][MATRIX_SIZE] InstanceMatrix() {
    MatrixCell matrix[MATRIX_SIZE][MATRIX_SIZE][MATRIX_SIZE];

    // Clear the matrix
    for (int x = 0; x < MATRIX_SIZE; x++) {
        for (int y = 0; y < MATRIX_SIZE; y++) {
            for (int z = 0; z < MATRIX_SIZE; z++) {
                matrix[x][y][z].occupied = 1;
                matrix[x][y][z].color = vec3(0.0, 0.0, 1.0); // Blue
            }
        }
    }

    return matrix;
}

RayHit Trace(Ray ray, MatrixCell[MATRIX_SIZE][MATRIX_SIZE][MATRIX_SIZE] matrix) {
    RayHit bestHit = CreateRayHit();

    for (int x = 0; x < MATRIX_SIZE; x++) {
        for (int y = 0; y < MATRIX_SIZE; y++) {
            for (int z = 0; z < MATRIX_SIZE; z++) {
                MatrixCell cell = matrix[x][y][z];
                vec3 cellPosition = vec3(x, y, z);
                IntersectMatrix(ray, bestHit, cell, cellPosition, CELL_SIZE); 
            }
        }
    }

    return bestHit;
}

vec3 Shade(inout Ray ray, RayHit hit, MatrixCell[MATRIX_SIZE][MATRIX_SIZE][MATRIX_SIZE] matrix) {
    if (hit.distance < INF) {
        // Reflect the ray and multiply energy with specular reflection
        ray.origin = hit.position + hit.normal * 0.001;
        ray.direction = reflect(ray.direction, hit.normal);
        ray.energy *= hit.specular;

        // Fix light direction
        vec3 light_direction = directional_light.data.xyz;
        light_direction.y *= -1.0;

        // Shadow test ray
        Ray shadowRay = CreateRay(hit.position + hit.normal * 0.001, -light_direction);
        RayHit shadowHit = Trace(shadowRay, matrix);
        if (shadowHit.distance != INF) {
            return vec3(0.0);
        }

        float NdotL = dot(hit.normal, light_direction);
        vec3 diffuse = hit.color * clamp(-NdotL, 0.0, 1.0);
        diffuse *= directional_light.data.w; // Multiply by light intensity

        vec3 view = normalize(ray.direction) * vec3(1.0, 1.0, -1.0);
        vec3 r = normalize(2.0 * NdotL * hit.normal - light_direction);
        float RdotV = dot(r, view);
        float shininess = 50.0;
        float _spec = max(pow(RdotV, shininess), 0.0);
        return diffuse + vec3(_spec);
    } else {
        ray.energy = vec3(0.0);
        return sky_color; // No intersection, return sky color
    }
}

void main() {
    // Couleur base du pixel pour l'image
    vec4 pixel = vec4(0.0, 0.0, 0.0, 1.0);

    ivec2 image_size = imageSize(rendered_image);
    // Coords in the range [-1,1]
    vec2 uv = vec2((gl_GlobalInvocationID.xy) / vec2(image_size) * 2.0 - 1.0);
    float aspect_ratio = float(image_size.x) / float(image_size.y);
    uv.x *= aspect_ratio;

    // Create the matrix of cells
    MatrixCell[MATRIX_SIZE][MATRIX_SIZE][MATRIX_SIZE] matrix = InstanceMatrix();

    // RayTracing
    Ray ray = CreateCameraRay(uv);
    vec3 result = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < MAX_REFLECTION_ITERATIONS + 1; i++) {
        RayHit hit = Trace(ray, matrix);
        result += ray.energy * Shade(ray, hit, matrix);
        if (!any(bvec3(ray.energy)))
            break;
    }
    pixel.xyz = result;

    // output to a specific pixel in the image buffer
    // Writes to texture
    imageStore(rendered_image, ivec2(gl_GlobalInvocationID.xy), pixel);
}
