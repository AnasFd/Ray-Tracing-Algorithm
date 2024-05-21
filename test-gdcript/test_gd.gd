extends Node


# Called when the node enters the scene tree for the first time.
func _ready():
	print("enter: step == 0")
	var input_1 = PackedInt32Array()
	for i in range(WSX):
		for j in range(WSY):
			input_1.append(randi())
	data_0 = input_1.to_byte_array()

	var input_2 = PackedInt32Array()
	for i in range(WSX):
		for j in range(WSY):
			input_2.append(randi())
	data_1 = input_2.to_byte_array()

var WSX:int = 128
var WSY:int = 128
var step:int = 0
var data_0:PackedByteArray=[]
var data_1:PackedByteArray=[]

# data_0 is for the "retina". data_1 is for the matrix.
func _process(delta):
	print("step == " + str(step))
	for x in range(128):
		for y in range(128):
			var p:int = x + y * WSX;

			#Init
					
			if (step == 0):
				if (((x / 16 + y / 16)) % 2 == 0) :
					data_1[p] = 0xFF000000 + (int(x) + 128*int(y))
				else:
					data_1[p] = 0xFF000000 + 0x00FF0000

			# RayTracing
			# 3D Matrix variables ********************/
			# Position
			# 100.0 * sin(step / 100.0)
			# -100 + 100.0 * sin(step / 50.0)
			var pos_mat_3D:Vector3 = Vector3(0, 0, 0)

			# Size
			var size_mat_3D:Vector3 = Vector3(128.0, 128.0, 1.0)

  			#******************** Retina variables ********************/
			# Position
			var pos_ret:Vector3 = Vector3(64.0, 64.0, 100.0 * absf(sin(step / 10.0)))
			# Size
			var size_ret:float = 1.0

			#******************** Eye variables ********************/
			var pos_eye:Vector3 = Vector3(pos_ret.x + (size_ret / 2), pos_ret.y + (size_ret / 2), pos_ret.z + (size_ret / 2) )

			#******************** Ray variables ********************/
			# Position
			var pos_ray:Vector3 = Vector3(pos_ret.x + x * size_ret / size_mat_3D.x, pos_ret.y + y * size_ret / size_mat_3D.y, pos_ret.z)
			# Direction
			var dir_ray:Vector3 = Vector3(pos_ray.x - pos_eye.x,  pos_ray.y - pos_eye.y, pos_ray.z - pos_eye.z)

			# Normalization of Direction
			# First, we calculate the length of the ray vector using the Euclidean
			# distance formula
			var L:float = sqrt((dir_ray.x * dir_ray.x) + (dir_ray.y * dir_ray.y) + (dir_ray.z * dir_ray.z))
			# Normalizing the vector
			dir_ray.x = dir_ray.x / L
			dir_ray.y = dir_ray.y / L
			dir_ray.z = dir_ray.z / L

			#* This next code segment traces a ray through the 3D matrix,
			#* checking for intersections with the defined regions.
			#* If an intersection is found, it retrieves the corresponding color value
			#* from the 3D matrix and assigns it to the output array data_0 (the Retina).
			#* The ray's length is limited to MAX_RAY_LENGTH iterations to prevent infinite loops.
			#* The loop iterates through the ray's path, updating its position at each
			#* step based on its direction vector. If the ray is inside the 3D matrix, the
			#* loop terminates early to optimize computation.

			var color:int = 0 # color init
			# Calculating the diagonal distance across the 3D matrix
			var diagonal_distance:float = sqrt(pow(size_mat_3D.x, 2) + pow(size_ret, 2))

			# Safety margin (10%)
			var safety_margin:float = 0.1 * diagonal_distance

			# Calculate the maximum ray length
			var MAX_RAY_LENGTH:int = int(diagonal_distance + safety_margin)

			if (step > 0):
				for i in range(MAX_RAY_LENGTH):
					# Is in 3D Matrix
					if (pos_ray.x >= pos_mat_3D.x && \
						pos_ray.y >= pos_mat_3D.y && \
						pos_ray.z >= pos_mat_3D.z && \
						pos_ray.x < (pos_mat_3D.x + size_mat_3D.x) && \
						pos_ray.y < (pos_mat_3D.y + size_mat_3D.y) && \
						pos_ray.z < (pos_mat_3D.z + size_mat_3D.z)):
						var p_ray:int = int(pos_ray.x) + int(pos_ray.y) * int(size_mat_3D.y)
						color = (data_1[p_ray] << 24) + (data_1[p_ray+1] << 16) + (data_1[p_ray+2] << 8) + (data_1[p_ray+3])
						break # If we've reached this far it means that the ray traveled through of the 3d matrix so we can move on to the next position
					pos_ray = pos_ray + dir_ray
				data_0[p*4] = (color & 0xFF000000) >> 24
				data_0[p*4 + 1] = (color & 0x00FF0000) >> 16
				data_0[p*4 + 2] = (color & 0x0000FF00) >> 8
				data_0[p*4 + 3] = (color & 0x000000FF)
	afficher_retine()
	step = step + 1

func afficher_retine():
	var disp : Node = $"../Retina"
	var values : PackedByteArray = data_0
		
	var img : Image = Image.create_from_data(WSX, WSY, false, Image.FORMAT_RGBA8, values)
	var tex : Texture2D = ImageTexture.create_from_image(img)
	
	if disp is Sprite2D :
		#var old_width  : float = disp.texture.get_width()
		#var old_height : float = disp.texture.get_height()
		disp.set_texture(tex)
		#disp.scale *= Vector2(old_width/WSX, old_height/WSY)
	else :
		disp.set_texture(tex)
