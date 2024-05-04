// data_0 is for the "retina". data_1 is for the matrix.
void main() {
  uint x = gl_GlobalInvocationID.x;
  uint y = gl_GlobalInvocationID.y;
  uint p = x + y * WSX;

  // Init
  if (step == 0) {
    if (((x / 16 + y / 16)) % 2 == 0)
      //data_1[p] = 0xFF000000 + (int(x) + 128*int(y)) ;
      data_1[p] = 0xFF000000;
  }

  // RayTracing
  /******************** 3D Matrix variables ********************/
  // Position
  //float pos_mat_3D.x = 0.0;
  //float pos_mat_3D.y = 0.0; // 100.0 * sin(step / 100.0);
  //float pos_mat_3D.z = 0.0; //-100 + 100.0 * sin(step / 50.0);
  vec3 pos_mat_3D = vec3(0.0, 0.0, 0.0);

  // Size
  //float size_mat_3D.x = 128.0;
  //float size_mat_3D.y = 128.0;
  //float size_mat_3D.z = 1.0;
  vec3 size_mat_3D = vec3(128.0, 128.0, 128.0);

  /******************** Retina variables ********************/
  // Position
  float pos_ret_x = 64.0;
  float pos_ret_y = 64.0;
  float pos_ret_z = 64.0;
  // Size
  float size_ret = 10.0;

  /******************** Eye variables ********************/
  float pos_eye_x = pos_ret_x + (size_ret / 2);
  float pos_eye_y = pos_ret_y + (size_ret / 2);
  float pos_eye_z = pos_ret_z + (size_ret / 2);

  /******************** Ray variables ********************/
  // Position
  float pos_ray_x = pos_ret_x + x * size_ret / size_mat_3D.x;
  float pos_ray_y = pos_ret_y + y * size_ret / size_mat_3D.y;
  float pos_ray_z = pos_ret_z;
  // Direction
  float dir_ray_x = pos_ray_x - pos_eye_x;
  float dir_ray_y = pos_ray_y - pos_eye_y;
  float dir_ray_z = pos_ray_z - pos_eye_z;

  // Normalization of Direction
  // First, we calculate the length of the ray vector using the Euclidean
  // distance formula
  float L = sqrt((dir_ray_x * dir_ray_x) + (dir_ray_y * dir_ray_y) +
                 (dir_ray_z * dir_ray_z));
  // Normalizing the vector
  dir_ray_x = dir_ray_x / L;
  dir_ray_y = dir_ray_y / L;
  dir_ray_z = dir_ray_z / L;

  /* This next code segment traces a ray through the 3D matrix,
   * checking for intersections with the defined regions.
   * If an intersection is found, it retrieves the corresponding color value
   * from the 3D matrix and assigns it to the output array data_0 (the Retina).
   * The ray's length is limited to MAX_RAY_LENGTH iterations to prevent infinite loops.
   * The loop iterates through the ray's path, updating its position at each
   * step based on its direction vector. If the ray is inside the 3D matrix, the
   * loop terminates early to optimize computation. */

  int color = 0; // color init
  // Calculating the diagonal distance across the 3D matrix
  float diagonal_distance = sqrt(pow(size_mat_3D.x, 2) + pow(size_ret, 2));

  // Safety margin (10%)
  float safety_margin = 0.1 * diagonal_distance;

  // Calculate the maximum ray length
  int MAX_RAY_LENGTH = int(diagonal_distance + safety_margin);

  if (step > 0) {
    for (int i = 0; i < MAX_RAY_LENGTH; i++) {
      // Is in 3D Matrix
      if (pos_ray_x >= pos_mat_3D.x && pos_ray_y >= pos_mat_3D.y &&
          pos_ray_z >= pos_mat_3D.z &&
          pos_ray_x < (pos_mat_3D.x + size_mat_3D.x) &&
          pos_ray_y < (pos_mat_3D.y + size_mat_3D.y) &&
          pos_ray_z < (pos_mat_3D.z + size_mat_3D.z)) {
        int pos_ray = int(pos_ray_x) + int(pos_ray_y) * int(size_mat_3D.x);
        color = data_1[pos_ray] == 0xFF000000 ? data_1[pos_ray] : 0x00FFFFFF;
        break; // If we've reached this far it means that the ray traveled
               // through of the 3d matrix so we can move on to the next position
      }
      pos_ray_x += dir_ray_x;
      pos_ray_y += dir_ray_y;
      pos_ray_z += dir_ray_z;
    }
    data_0[p] = color;
  }
}
