// data_0 = retina. 
// data_1 = matrix.

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
    vec3 position;
    vec3 direction;
};

int MAX_RAY_LENGTH;
const float retina_matrix_distance = 170.0;
const float retina_size_factor = 0.06;
const float eye_retina_distance = 10.0;
Matrix matrix;
Retina retina;
Eye eye;
int color = 0;

// Helper functions:
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

int calculate_max_ray_length() {
    // Calculating the diagonal distance across the matrix and from the retina
    float diagonal_distance = sqrt(
        pow(matrix.size.x, 2) + 
        pow(matrix.size.y, 2) + 
        pow(retina.position.z, 2)
    );
    // Adding a safety margin of 10%
    float safety_margin = 0.1 * diagonal_distance;
    
    return int(diagonal_distance + safety_margin);
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

// The 2 functions bellow aren't needed because we can give the colors in glsl
// in the 0xAARRGGBB Format. So here I'm manualy encoding the color to be less confusing.
int encodeColor(float r, float g, float b, float a) {
    return (int(a * 255) << 24) | (int(r * 255) << 16) | (int(g * 255) << 8) | int(b * 255);
}

// This function decodedes a color to give us access to it values.
// In our case, it is not needed.
vec4 decodeColor(uint color) {
    float a = float((color >> 24) & 0xFF) / 255.0;
    float r = float((color >> 16) & 0xFF) / 255.0;
    float g = float((color >> 8) & 0xFF) / 255.0;
    float b = float(color & 0xFF) / 255.0;
    return vec4(r, g, b, a);
}

// End Helper functions
void main() {
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    uint p = x + y * WSX;

    // Init
    if (step == 0) {
        // Defining the Matrix variables
        matrix.position = vec3(0.0, 0.0, 0.0);
        matrix.size = vec3(128.0, 128.0, 1.0);
        
        // Defining the Retina variables
        retina.width = matrix.size.x * retina_size_factor;
        retina.extent = matrix.size.y * retina_size_factor; // Used extent instead of length because length is a builtin glsl function
        retina.position = vec3(
            matrix.position.x + matrix.size.x / 2.0 - (retina.width / 2.0),
            matrix.position.y + matrix.size.y / 2.0 - (retina.extent / 2.0),
            matrix.position.z + retina_matrix_distance
        );

        // Defining the Eye variables
        eye.position = vec3(
            retina.position.x + (retina.width / 2.0),
            retina.position.y + (retina.extent / 2.0),
            retina.position.z + eye_retina_distance
        );

        // Calculating the ray's max length
        MAX_RAY_LENGTH = calculate_max_ray_length();

        // Defining the Matrix's initial colors (chess like pattern)
        if (((x / 4 + y / 4)) % 2 == 0)
            data_1[p] = encodeColor(1.0, 0.0, 0.0, 1.0); // Red
        else
            data_1[p] = encodeColor(0.0, 0.0, 1.0, 1.0); // Blue
    }

    // Ray tracing
    if (step > 0) {
        // Creating a new ray
        Ray ray = create_ray(x, y);
        for (int i = 0; i < MAX_RAY_LENGTH; i++) {
            // Is in 3D Matrix ?
            if (is_in_matrix(ray.position)) {
                int pos_ray = int(ray.position.x) + int(ray.position.y) * int(matrix.size.y);
                color = data_1[pos_ray];
                break; 
            }
            ray.position += ray.direction;
        }
        data_0[p] = color;
    }
}
