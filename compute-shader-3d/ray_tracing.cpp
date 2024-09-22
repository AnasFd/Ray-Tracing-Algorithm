// data_0: Retina
// data_1: Matrix

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
const float retina_matrix_distance = 400.0;
const float retina_size_factor = 1.;
const float eye_retina_distance = 1000.0;
Matrix matrix;
Retina retina;
Eye eye;
int color;

// Helper functions:

int encodeColor(float r, float g, float b, float a) {
    int ri = int(clamp(r, 0.0, 1.0) * 255.0);
    int gi = int(clamp(g, 0.0, 1.0) * 255.0);
    int bi = int(clamp(b, 0.0, 1.0) * 255.0);
    int ai = int(clamp(a, 0.0, 1.0) * 255.0);

    // Swap red and blue channels, Because encodeColor(1,0,0,1) (red) gives blue...
    return (ai << 24) | (bi << 16) | (gi << 8) | ri;
}

// Initialization function
void ready() {
    // Initialize matrix
    matrix.position = vec3(0.0, 0.0, 0.0);
    matrix.size = vec3(128.0, 128.0, 128.0);
    matrix.center = matrix.position + (matrix.size / 2.0);

    // Initialize retina
    retina.width = WSX * retina_size_factor;
    retina.extent = WSY * retina_size_factor * (WSY / WSX);
    retina.position = vec3(
        matrix.center.x - (retina.width / 2.0),
        matrix.center.y - (retina.extent / 2.0),
        matrix.center.z + retina_matrix_distance
    );

    // Initialize eye
    eye.position = vec3(
        retina.position.x + retina.width / 2.0,
        retina.position.y + retina.extent / 2.0,
        retina.position.z + eye_retina_distance
    );

    // Define colors
    int red = encodeColor(1.0, 0.0, 0.0, 1.0);
    int green = encodeColor(0.0, 1.0, 0.0, 1.0);
    int blue = encodeColor(0.0, 0.0, 1.0, 1.0);
    int yellow = encodeColor(1.0, 1.0, 0.0, 1.0);
    int magenta = encodeColor(1.0, 0.0, 1.0, 1.0);
    int cyan = encodeColor(0.0, 1.0, 1.0, 1.0);
    int black = encodeColor(0.0, 0.0, 0.0, 1.0);
    int white = encodeColor(1.0, 1.0, 1.0, 1.0); 

    // Initialize matrix colors
    for (uint z = 0; z < matrix.size.z; z++) {
        for (uint x = 0; x < matrix.size.x; x++) {
            for (uint y = 0; y < matrix.size.y; y++) {
                uint p = x + y * uint(matrix.size.x) + z * uint(matrix.size.x) * uint(matrix.size.y);

                // Set face colors with chess pattern
                if (x == 0) {
                    if ((y / 16 + z / 16) % 2 == 0) {
                        data_1[p] = black;
                    } else {
                        data_1[p] = red;
                    }
                } else if (x == matrix.size.x - 1) {
                    if ((y / 16 + z / 16) % 2 == 0) {
                        data_1[p] = black;
                    } else {
                        data_1[p] = green;
                    }
                } else if (y == 0) {
                    if ((x / 16 + z / 16) % 2 == 0) {
                        data_1[p] = black;
                    } else {
                        data_1[p] = blue;
                    }
                } else if (y == matrix.size.y - 1) {
                    if ((x / 16 + z / 16) % 2 == 0) {
                        data_1[p] = black;
                    } else {
                        data_1[p] = yellow;
                    }
                } else if (z == 0) { // Top face
                    if ((x / 16 + y / 16) % 2 == 0) {
                        data_1[p] = black;
                    } else {
                        data_1[p] = magenta;
                    }
                } else if (z == matrix.size.z - 1) {
                    if ((x / 16 + y / 16) % 2 == 0) {
                        data_1[p] = black;
                    } else {
                        data_1[p] = cyan;
                    }
                } else { // inside of the matrix
                    data_1[p] = white;
                }
            }
        }
    }
}

int calculate_max_ray_length(float step_size) {
    float matrix_diagonal_distance = sqrt(
        pow(matrix.size.x, 2) + 
        pow(matrix.size.y, 2) + 
        pow(matrix.size.z, 2)
    );
    float max_distance = matrix_diagonal_distance + retina_matrix_distance + eye_retina_distance;
    float safety_margin = 0.1 * max_distance;

    // Adjust MAX_RAY_LENGTH based on the step size
    return int((max_distance + safety_margin) / step_size);
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
        retina.position.x + (x / float(WSX)) * retina.width,
        retina.position.y + (y / float(WSY)) * retina.extent,
        retina.position.z
    );

    //ray.direction = normalize(ray.position - eye.position); // Not always facing the matrix, but gives a full view of the scene
    //ray.direction = normalize(matrix.center - ray.position); // Always facing the matrix but too zoomed in, good for camera rotation tests
    
    // As a solution to the above problem,
    // we mix the direction from eye to pixel and eye to matrix center
    vec3 direction_to_pixel = normalize(ray.position - eye.position);
    vec3 direction_to_center = normalize(matrix.center - ray.position);
    
    // Blend both directions: 0.8 -> bias towards the scene, 0.2 -> always face matrix
    ray.direction = normalize(mix(direction_to_pixel, direction_to_center, 0.0)); 
    return ray;
}

vec3 rotate_around_axis(vec3 v, vec3 axis, float angle) {
    axis = normalize(axis);
    float cos_angle = cos(angle);
    float sin_angle = sin(angle);
    return v * cos_angle + cross(axis, v) * sin_angle + axis * dot(axis, v) * (1.0 - cos_angle);
}

void main() {
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    uint p = x + y * WSX; // Used for the 2D Retina

    // Initialization step
    if (step == 0) {
        ready();
    }

    // Ray marching
    if (step > 0) {
        // Rotation axis and angle
        vec3 axis = vec3(1.0, 1.0, 0.0); // (x,y,z)
        //float angle = 10 * sin(step * 0.0001); // Use step for dynamic angle adjusments
        float angle = radians(-45.);

        // Camera rotatin (the camera is composed of the eye and the retina)
        retina.position = rotate_around_axis(retina.position, axis, angle);
        eye.position = rotate_around_axis(eye.position, axis, angle);

        // Create the ray
        Ray ray = create_ray(x,y);

        float step_size = 0.1; // In my experience, 0.1 is the most optimal value for maximum matrix clarity, an other factor is the eye_retina_distance var (FOV)
        MAX_RAY_LENGTH = calculate_max_ray_length(step_size); // In a dynamic setup, calculate MAX_RAY_LENGTH every step. Otherwise this can go on the ready() func

        // Ray launching
        for (int i = 0; i < MAX_RAY_LENGTH; i++) {
            color = encodeColor(1.0, 1.0, 1.0, 0.5);
            if (is_in_matrix(ray.position)) {
                // Calculate the ray's position in 3D space
                int pos_ray = int(ray.position.x) + int(ray.position.y) * int(matrix.size.x) + int(ray.position.z) * int(matrix.size.x) * int(matrix.size.y);
                color = data_1[pos_ray]; // Retrieve the color where the intersection happened
                break;
            }
            ray.position += ray.direction * step_size;  // Keep small step size for clearer view
        }
        data_0[p] = color; // Assign to the retina the color of the matrix where the intersection happened
    }
}
