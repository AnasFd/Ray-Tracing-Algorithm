// data_0 is for the "retina". data_1 is for the matrix.
void main() {
	// TODO: Look up what do these stand for
	uint x = gl_GlobalInvocationID.x; 
	uint y = gl_GlobalInvocationID.y;
	uint p = x + y * WSX;

	// Init
	if (step == 0)  {
		if ( ( (x/16+y/16) ) % 2 == 0)
        data_1[p] = 0xFF000000 + (int(x) + 128*int(y)) ;
	}

	// RayTracing
	float pos_mat_3D_x = 0.0f ;
	float pos_mat_3D_y = 0; //100.0 * sin(step / 100.0);
	float pos_mat_3D_z = -100 + 100.0 * sin(step / 50.0);

	float pos_mat_ret_x = 64;
	float pos_mat_ret_y = 64;
	float pos_mat_ret_z = 64;

	float size_mat_ret = 8.0;

	float pos_eye_x = 64.0 + 20.0*sin(step / 100.0);
	float pos_eye_y = 64.0;
	float pos_eye_z = 70.0;

	float pos_ray_x = pos_mat_ret_x + x * size_mat_ret / 128.0 ; 
	float pos_ray_y = pos_mat_ret_y + y * size_mat_ret / 128.0;
	float pos_ray_z = pos_mat_ret_z;

	float dir_ray_x = ( pos_ray_x - pos_eye_x ) ;
	float dir_ray_y = ( pos_ray_y - pos_eye_y ) ;
	float dir_ray_z = ( pos_ray_z - pos_eye_z ) ;

	int color = 0;

	// L for Length, Here it's the length of the ray vector
	float L = sqrt( (dir_ray_x*dir_ray_x) + (dir_ray_y*dir_ray_y) + (dir_ray_z*dir_ray_z)); 
	L = L*2.0;
	dir_ray_x = dir_ray_x / L ;
	dir_ray_y = dir_ray_y / L ;
	dir_ray_z = dir_ray_z / L ;

	if(step > 0) {
		for (int i = 0; i < 600; i++) {
			// Is in 3D Matrix ? Color it white
		 	if (pos_ray_x >= pos_mat_3D_x && pos_ray_y >= pos_mat_3D_y && pos_ray_x < (pos_mat_3D_x + 128) && pos_ray_y < (pos_mat_3D_y + 128) ) {
                 if ( pos_ray_z >= pos_mat_3D_z && pos_ray_z < pos_mat_3D_z+1 ) {
                    int pos_ray = int(pos_ray_x) + int(pos_ray_y) * 128;
                    color = data_1[pos_ray];
                    i = 600;
                 }
			}
			pos_ray_x += dir_ray_x;
			pos_ray_y += dir_ray_y;
			pos_ray_z += dir_ray_z;
		}
		data_0[p] = color;

	}
}
