
struct Matrix {
    vec3 position;
    vec3 size;
};

struct Retina {
    vec3 position;
    float width;
    float extent;
};

struct Eye {
    vec3 position;
};

struct Ray {
    vec3 origin;
    vec3 direction;
};

int MAX_RAY_LENGTH;
const float retina_matrix_distance = 170.0;
const float retina_size_factor = 0.0001;
const float eye_retina_distance = 10.0;
Matrix matrix;
Retina retina;
Eye eye;
int color = 0;

// Helper functions:

int calculate_max_ray_length() {
    float diagonal_distance = sqrt(
        pow(matrix.size.x, 2) + 
        pow(matrix.size.y, 2) + 
        pow(retina.position.z, 2)
    );
    float safety_margin = 0.1 * diagonal_distance;
    
    return int(diagonal_distance + safety_margin);
}

bool is_in_matrix(vec3 ray_position, float MATRIX_DEPTH) {
    return (
        ray_position.x >= matrix.position.x && 
        ray_position.y >= matrix.position.y && 
        ray_position.z >= matrix.position.z - MATRIX_DEPTH && 
        ray_position.x < (matrix.position.x + matrix.size.x) && 
        ray_position.y < (matrix.position.y + matrix.size.y) && 
        ray_position.z < (matrix.position.z - MATRIX_DEPTH + matrix.size.z)
    );
}

Ray create_ray(uint x, uint y) {
    Ray ray;
    ray.origin = vec3(
        retina.position.x + x * retina.width / matrix.size.x,
        retina.position.y + y * retina.extent / matrix.size.y,
        retina.position.z
    );
    ray.direction = vec3(
        ray.origin.x - eye.position.x,
        ray.origin.y - eye.position.y,
        ray.origin.z - eye.position.z
    );
    ray.direction = normalize(ray.direction);

    return ray;
}

int encodeColor(float r, float g, float b, float a) {
    return (int(a * 255) << 24) | (int(r * 255) << 16) | (int(g * 255) << 8) | int(b * 255);
}

vec4 decodeColor(uint color) {
    float a = float((color >> 24) & 0xFF) / 255.0;
    float r = float((color >> 16) & 0xFF) / 255.0;
    float g = float((color >> 8) & 0xFF) / 255.0;
    float b = float(color & 0xFF) / 255.0;
    return vec4(r, g, b, a);
}

vec3 rotate_around_axis(vec3 v, vec3 axis, float angle) {
    axis = normalize(axis);
    float cos_angle = cos(angle);
    float sin_angle = sin(angle);
    return v * cos_angle + cross(axis, v) * sin_angle + axis * dot(axis, v) * (1.0 - cos_angle);
}

vec3 rotate_around_axisdd(vec3 original, vec3 axis, float angle) {
    angle = radians(angle);
    float cos_angle = cos(angle);
    float sin_angle = sin(angle);
    float one_minus_cos = 1.0 - cos_angle;

    // Normalize the axis
    axis = normalize(axis);

    // Compute the rotation matrix components
    float xx = axis.x * axis.x * one_minus_cos + cos_angle;
    float xy = axis.x * axis.y * one_minus_cos - axis.z * sin_angle;
    float xz = axis.x * axis.z * one_minus_cos + axis.y * sin_angle;

    float yx = axis.y * axis.x * one_minus_cos + axis.z * sin_angle;
    float yy = axis.y * axis.y * one_minus_cos + cos_angle;
    float yz = axis.y * axis.z * one_minus_cos - axis.x * sin_angle;

    float zx = axis.z * axis.x * one_minus_cos - axis.y * sin_angle;
    float zy = axis.z * axis.y * one_minus_cos + axis.x * sin_angle;
    float zz = axis.z * axis.z * one_minus_cos + cos_angle;

    // Apply the rotation matrix
    return vec3(
        original.x * xx + original.y * xy + original.z * xz,
        original.x * yx + original.y * yy + original.z * yz,
        original.x * zx + original.y * zy + original.z * zz
    );
}

Ray create_ray_from_rotated_positions(vec3 retina_pos, vec3 eye_pos, vec3 matrix_center) {
    Ray ray;
    ray.origin = retina_pos;
    ray.direction = normalize(matrix_center - eye_pos);
    return ray;
}

// Initialization function
void ready(uint x, uint y) {
    // Initialize matrix
    matrix.position = vec3(0.0, 0.0, 0.0);
    matrix.size = vec3(128.0, 128.0, 2.0);

    // Initialize retina
    retina.width = matrix.size.x * retina_size_factor;
    retina.extent = matrix.size.y * retina_size_factor;
    retina.position = vec3(
        matrix.position.x + matrix.size.x / 2.0 - (retina.width / 2.0),
        matrix.position.y + matrix.size.y / 2.0 - (retina.extent / 2.0),
        matrix.position.z + retina_matrix_distance
    );

    // Initialize eye
    eye.position = vec3(
        retina.position.x + (retina.width / 2.0),
        retina.position.y + (retina.extent / 2.0),
        retina.position.z + eye_retina_distance
    );

    // Calculate max ray length
    MAX_RAY_LENGTH = calculate_max_ray_length();

    // Initialize matrix colors
    for (uint z = 0; z < 2; z++) {
        uint p = x + y * WSX + z * x * y;
        if (z == 0) {
            if (x <= 64)
                if (y <= 64) 
                    data_1[p] = encodeColor(0.0, 1.0, 0.0, 1.0);
                else
                    data_1[p] = encodeColor(1.0, 1.0, 1.0, 1.0);
            else
                if (y <= 64) 
                    data_1[p] = encodeColor(0.0, 0.0, 1.0, 1.0);
                else
                    data_1[p] = encodeColor(1.0, 0.0, 0.0, 1.0);
        } else {
            if (x <= 64)
                if (y <= 64) 
                    data_2[p] = encodeColor(1.0, 1.0, 0.0, 1.0);
                else
                    data_2[p] = encodeColor(0.66, 0.66, 0.66, 1.0);
            else
                if (y <= 64) 
                    data_2[p] = encodeColor(1.0, 1.0, 0.5, 1.0);
                else
                    data_2[p] = encodeColor(1.0, 0.3, 0.77, 1.0);
        }
    }
}

void main() {
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    uint p = x + y * WSX;

    // Initialization step
    if (step == 0) {
        ready(x, y);
    }

    // Ray tracing step
    if (step > 0) {
        float angle = 10. * abs(sin(step / 1000.));
        vec3 axis = vec3(1.0, 1.0, 0.0);

        // Rotate the matrix around the specified axis
       // vec3 matrix_center = matrix.position + matrix.size / 2.0;
       // vec3 translated_matrix_pos = matrix.position - matrix_center;
       // vec3 rotated_matrix_pos = rotate_around_axis(translated_matrix_pos, axis, angle);
       // matrix.position = rotated_matrix_pos + matrix_center;

        // Rotate the retina position
//        vec3 translated_retina_pos = retina.position - matrix_center;
//        vec3 rotated_retina_pos = rotate_around_axis(translated_retina_pos, axis, angle);
//        retina.position = rotated_retina_pos + matrix_center;
//
//        // Rotate the eye position
//        vec3 translated_eye_pos = eye.position - matrix_center;
//        vec3 rotated_eye_pos = rotate_around_axis(translated_eye_pos, axis, angle);
//        eye.position = rotated_eye_pos + matrix_center;
//
//
//        // Create and rotate the ray
        Ray ray = create_ray(x, y);
//        ray.origin = vec3(
//        retina.position.x + x * retina.width / matrix.size.x,
//        retina.position.y + y * retina.extent / matrix.size.y,
//        retina.position.z
//    );
        //ray.direction = normalize(matrix_center - eye.position);
        //ray.origin = rotate_around_axis(ray.origin - matrix_center, axis, angle) + matrix_center;
        //ray.direction = rotate_around_axis(ray.direction, axis, angle);
        //ray.direction = normalize(ray.direction);

        // Calculate max ray length, normally calculated once if we're dealing with a static setup
        // We're working with a dynamic setup (changing the matrix's rotation) so it should be
        // recalculated every step
        MAX_RAY_LENGTH = calculate_max_ray_length();

// Trace the ray through the matrix
        bool intersected = false;
        for (int i = 0; i < MAX_RAY_LENGTH; i++) {
            for (int z = 0; z < matrix.size.z; z++) {
                if (is_in_matrix(ray.origin, float(z))) {
                    int pos_ray = int(ray.origin.x) + int(ray.origin.y) * int(matrix.size.x) + int(ray.origin.z) * int(matrix.size.x) * int(matrix.size.y);
                    if (z == 0) {
                        color = data_1[pos_ray];
                    } else {
                        color = data_2[pos_ray];
                    }
                    intersected = true;
                    break;
                }
            }
            if (intersected) break;
            ray.origin += ray.direction;
        }
        data_0[p] = color;
    }
}
