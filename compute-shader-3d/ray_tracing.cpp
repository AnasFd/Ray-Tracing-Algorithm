struct Matrix {
    vec3 position;
    vec3 size;
    vec3 center;
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
    vec3 position;
    vec3 direction;
};

int MAX_RAY_LENGTH;
const float retina_matrix_distance = 200.0;
const float retina_size_factor = 2.0;
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
            pow(matrix.size.z, 2)
            );
    float safety_margin = 0.5 * diagonal_distance;

    return int(diagonal_distance + safety_margin);
}

bool is_in_matrix(vec3 ray_position) {
    return (
            ray_position.x >= matrix.position.x && 
            ray_position.y >= matrix.position.y && 
            ray_position.z >= matrix.position.z && 
            ray_position.x < (matrix.position.x + matrix.size.x) && 
            ray_position.y < (matrix.position.y + matrix.size.y) && 
            ray_position.z < (matrix.position.z + matrix.size.z)
           );
}

Ray create_ray(uint x, uint y) {
    Ray ray;
    ray.position = vec3(
            retina.position.x + x * retina.width / matrix.size.x,
            retina.position.y + y * retina.extent / matrix.size.y,
            retina.position.z
            );
    ray.direction = vec3(
            ray.position.x - eye.position.x,
            ray.position.y - eye.position.y,
            ray.position.z - eye.position.z
            );
    ray.direction = normalize(ray.direction);


    return ray;
}

//int encodeColor(float r, float g, float b, float a) {
//    return (int(a * 255) << 24) | (int(r * 255) << 16) | (int(g * 255) << 8) | int(b * 255);
//}

int encodeColor(float r, float g, float b, float a) {
    int ri = int(clamp(r, 0.0, 1.0) * 255.0);
    int gi = int(clamp(g, 0.0, 1.0) * 255.0);
    int bi = int(clamp(b, 0.0, 1.0) * 255.0);
    int ai = int(clamp(a, 0.0, 1.0) * 255.0);
    return (ai << 24) | (ri << 16) | (gi << 8) | bi;
}

vec3 rotate_around_axis(vec3 v, vec3 axis, float angle) {
    axis = normalize(axis);
    float cos_angle = cos(angle);
    float sin_angle = sin(angle);
    return v * cos_angle + cross(axis, v) * sin_angle + axis * dot(axis, v) * (1.0 - cos_angle);
}

vec3 to3d(int idx) {
    int z = idx / int(matrix.size.x * matrix.size.y);
    idx -= z * int(matrix.size.x * matrix.size.y);
    int y = idx / int(matrix.size.x);
    int x = idx % int(matrix.size.x);

    return vec3(float(x), float(y), float(z));
}

// Initialization function
void ready() {
    // Initialize matrix
    matrix.position = vec3(0.0, 0.0, 0.0);
    matrix.size = vec3(128.0, 128.0, 128.0);
    matrix.center = matrix.position + matrix.size / 2.0;

    // Initialize retina
    retina.width = matrix.size.x * retina_size_factor;
    retina.extent = matrix.size.y * retina_size_factor;
    //retina.position = vec3( // Perfectly in the middle above the matrix
    //    matrix.position.x + matrix.size.x / 2.0 - (retina.width / 2.0),
    //    matrix.position.y + matrix.size.y / 2.0 - (retina.extent / 2.0),
    //    matrix.position.z + retina_matrix_distance
    //);


    // Initialize eye
    // eye.position = vec3( // Perfectly in the middle above the retina
    //         retina.position.x + retina.width / 2.0,
    //         retina.position.y + retina.extent / 2.0 + 5,
    //         retina.position.z + eye_retina_distance
    //         );

    eye.position = vec3(256.0, 256.0, 256.0);


    // Calculate max ray length
    MAX_RAY_LENGTH = calculate_max_ray_length();

    // Define colors
    int colorFront = encodeColor(1.0, 0.0, 0.0, 1.0); // Red
    int colorBack = encodeColor(0.0, 1.0, 0.0, 1.0); // Green
    int colorLeft = encodeColor(0.0, 0.0, 1.0, 1.0); // Blue
    int colorRight = encodeColor(1.0, 1.0, 0.0, 1.0); // Yellow
    int colorTop = encodeColor(1.0, 0.0, 1.0, 1.0); // Magenta
    int colorBottom = encodeColor(0.0, 1.0, 1.0, 1.0); // Cyan
    int colorBlack = encodeColor(0.0, 0.0, 0.0, 1.0); // Black

    // Initialize matrix colors
    for (uint z = 0; z < matrix.size.z; z++) {
        for (uint x = 0; x < matrix.size.x; x++) {
            for (uint y = 0; y < matrix.size.y; y++) {
                uint p = x + y * uint(matrix.size.x) + z * uint(matrix.size.x) * uint(matrix.size.y);

                // Set face colors with chess pattern
                if (x == 0) { // Front face
                    if ((y / 16 + z / 16) % 2 == 0) {
                        data_1[p] = colorBlack;
                    } else {
                        data_1[p] = colorFront;
                    }
                } else if (x == matrix.size.x - 1) { // Back face
                    if ((y / 16 + z / 16) % 2 == 0) {
                        data_1[p] = colorBlack;
                    } else {
                        data_1[p] = colorBack;
                    }
                } else if (y == 0) { // Left face
                    if ((x / 16 + z / 16) % 2 == 0) {
                        data_1[p] = colorBlack;
                    } else {
                        data_1[p] = colorLeft;
                    }
                } else if (y == matrix.size.y - 1) { // Right face
                    if ((x / 16 + z / 16) % 2 == 0) {
                        data_1[p] = colorBlack;
                    } else {
                        data_1[p] = colorRight;
                    }
                } else if (z == 0) { // Top face
                    if ((x / 16 + y / 16) % 2 == 0) {
                        data_1[p] = colorBlack;
                    } else {
                        data_1[p] = colorTop;
                    }
                } else if (z == matrix.size.z - 1) { // Bottom face
                    if ((x / 16 + y / 16) % 2 == 0) {
                        data_1[p] = colorBlack;
                    } else {
                        data_1[p] = colorBottom;
                    }
                } else {
                    data_1[p] = encodeColor(1.0, 1.0, 1.0, 1.0); // White
                }
            }
        }
    }
}

void main() {
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    uint p = x + y * WSX; // Used for the 2D Retina

    // Initialization step
    if (step == 0) {
        ready();
    }

    // Ray tracing step
    if (step > 0) {
        float angle = abs(10.0 * sin(step / 1000.0));
        //float angle = 95;
        vec3 axis = vec3(1.0, 1.0, 0.0);


        // Position the retina in front of the eye in the direction of the view
        vec3 eye_to_matrix = normalize(matrix.position - eye.position);
        vec3 retina_offset = vec3(
                (x - retina.width / 2.0) * retina.width / matrix.size.x,
                (y - retina.width / 2.0) * retina.extent / matrix.size.y,
                0.0
                );
        retina.position = eye.position + eye_to_matrix * 200.0;
        retina.position = retina.position + retina_offset;
        // Interesting for testing...
        // Rotate the Matrix
        //vec3 translate_matrix_pos = matrix.position - matrix.center; // Alignment with the center to apply a rotaion around it
        //vec3 rotated_matrix_pos = rotate_around_axis(translate_matrix_pos, axis, angle); // Rotation
        //matrix.position = rotated_matrix_pos + matrix.center; // Back to it original position

        // Rotate the retina position (Same principal as the matrix)
        //vec3 translated_retina_pos = retina.position - matrix.center;
        //vec3 rotated_retina_pos = rotate_around_axis(translated_retina_pos, axis, angle);
        //retina.position = rotated_retina_pos + matrix.center;

        //// Rotate the eye position
        //vec3 translated_eye_pos = eye.position - matrix.center;
        //vec3 rotated_eye_pos = rotate_around_axis(translated_eye_pos, axis, angle);
        //eye.position = rotated_eye_pos + matrix.center;

        // Create the ray
        Ray ray = create_ray(x, y);

        // Calculate max ray length
        MAX_RAY_LENGTH = calculate_max_ray_length();

        // Trace the ray through the matrix
        for (int i = 0; i < MAX_RAY_LENGTH; i++) {
            if (is_in_matrix(ray.position)) {
                int pos_ray = int(ray.position.x) + int(ray.position.y) * int(matrix.size.x) + int(ray.position.z) * int(matrix.size.x) * int(matrix.size.y);
                // In the beggining I needed access to the z value of pos_ray because I hade each "slice" of the matrix declared in a Texture2D of its own
                // But Then I found a better way... Keeping this here because the function itself is intresting
                // Credits: https://stackoverflow.com/questions/7367770/how-to-flatten-or-index-3d-array-in-1d-array 
                // First comment, thanks.
                vec3 vec_pos_ray = to3d(pos_ray);

                color = data_1[pos_ray];
                break;
            }
            ray.position += ray.direction;
        }
        data_0[p] = color;
    }
}
