extends Node

@onready var dispRetina:Node = $"../Retina"
@onready var dispMatrix:Node = $"../Matrice3D"
var WSX:int = 128
var WSY:int = 128
var step:int = 0
var data_0:PackedByteArray=[]
var data_1:PackedByteArray=[]

class Matrix:
	var Position:Vector3
	var Size:Vector3
	
class Retina:
	var Position:Vector3
	var Size:float

class Eye:
	var Position:Vector3

class Ray:
	var Position:Vector3
	var Direction:Vector3

var matrix = Matrix.new()
var retina = Retina.new()
var eye = Eye.new()
var ray = Ray.new()
var MAX_RAY_LENGTH:int

# Called when the node enters the scene tree for the first time.
func _ready():
	print("enter: step == 0")
	
	# Matrix variables
	matrix.Position = Vector3(0, 0, 0)
	matrix.Size = Vector3(128.0, 128.0, 1.0)

	# Calculate once, MAX_RAY_LENGTH
	MAX_RAY_LENGTH = calculate_max_ray_length()
	
	data_0 = PackedByteArray()
	data_1 = PackedByteArray()
	
# Create a checkerboard pattern for data_1
	for y in range(WSY):
		for x in range(WSX):
			if (x / 16 + y / 16) % 2 == 0:
				# Bellow is equivalent to 0xFF0000FF (Green in RGBA format)
				data_1.append(0x00)  # Red
				data_1.append(0x00)  # Green
				data_1.append(0xFF)  # Blue
				data_1.append(0xFF)  # Alpha
			else:
				# Bellow is equivalent to 0x00FF00FF (Green in RGBA format)
				data_1.append(0x00)  # Red
				data_1.append(0xFF)  # Green
				data_1.append(0x00)  # Blue
				data_1.append(0xFF)  # Alpha
	
	# Initialize data_0 with zeros
	for i in range(WSX * WSY * 4):
		data_0.append(0)

	display(dispMatrix,data_1)

# data_0 is for the "retina". data_1 is for the matrix.
func _process(delta):
	print("step == " + str(step))
	for x in range(WSX):
		for y in range(WSY):
			var p:int = x + y * WSX;

			# Retina variables (Put them in _process for variation purposes
			#retina.Position = Vector3(64.0, 64.0, 100.0 * absf(sin(step / 10.0)))
			retina.Position = Vector3(64.0, 64.0, 64) 
				
			retina.Size = 10.0
			
			# Eye variables
			eye.Position = Vector3(retina.Position.x + (retina.Size / 2),
									retina.Position.y + (retina.Size / 2),
									retina.Position.z + (retina.Size / 2))
			
			# RayTracing
			ray.Position = Vector3(retina.Position.x + x * retina.Size / matrix.Size.x,
									retina.Position.y + y * retina.Size / matrix.Size.y,
									retina.Position.z)
			
			ray.Direction = Vector3(ray.Position.x - eye.Position.x, \
									ray.Position.y - eye.Position.y, \
									ray.Position.z - eye.Position.z)
			
			# Normalization of Direction
			ray.Direction = normalize(ray.Direction) # Equivalent to the godot builtin 'normilized()' function

			#* This next code segment traces a ray through the 3D matrix,
			#* checking for intersections with the defined regions.
			#* If an intersection is found, it retrieves the corresponding color value
			#* from the 3D matrix and assigns it to the output array data_0 (the Retina).
			#* The ray's length is limited to MAX_RAY_LENGTH iterations to prevent infinite loops.
			#* The loop iterates through the ray's path, updating its position at each
			#* step based on its direction vector. If the ray is inside the 3D matrix, the
			#* loop terminates early to optimize computation.
			var color:int = 0 # color init

			if (step > 0):
				for i in range(MAX_RAY_LENGTH):
					# Is in 3D Matrix
					if (is_in_matrix(ray)):
						var p_ray:int = int(ray.Position.x) + int(ray.Position.y) * int(matrix.Size.y)
						p_ray*=4
						# Add bounds checking to ensure p_ray and subsequent accesses are within bounds
						if p_ray + 3 < data_1.size():
							color = (data_1[p_ray] << 24) + (data_1[p_ray + 1] << 16) + (data_1[p_ray + 2] << 8) + (data_1[p_ray + 3])
							#print("Color Retrieved: ", color)
						else:
							print("Out of bounds access: ", p_ray)
							
						break # If we've reached this far it means that the ray traveled through of the 3d matrix so we can move on to the next position
					ray.Position += ray.Direction
				
				data_0[p*4] = color >> 24
				data_0[p*4 + 1] = color >> 16
				data_0[p*4 + 2] = color >> 8
				data_0[p*4 + 3] = color
				
	display(dispRetina,data_0)
	step = step + 1

func normalize(old_dir)->Vector3:
	# First, we calculate the length of the ray vector using the Euclidean distance formula
	var L:float = sqrt((pow(old_dir.x,2)) + (pow(old_dir.y,2)) + (pow(old_dir.z,2)))
	# Normalizing the vector
	var new_dir:Vector3 = Vector3((old_dir.x / L), (old_dir.y / L), (old_dir.z / L))
	return new_dir

func calculate_max_ray_length()->int:
	# Calculating the diagonal distance across the 3D matrix
	var diagonal_distance:float = sqrt(pow(matrix.Size.x, 2) + pow(retina.Size, 2))
	# Safety margin (10%)
	var safety_margin:float = 0.1 * diagonal_distance
	
	return int(diagonal_distance + safety_margin)

func is_in_matrix(ray: Ray)->bool:
	return (ray.Position.x >= matrix.Position.x and 
			ray.Position.y >= matrix.Position.y and 
			ray.Position.z >= matrix.Position.z and 
			ray.Position.x < (matrix.Position.x + matrix.Size.x) and 
			ray.Position.y < (matrix.Position.y + matrix.Size.y) and 
			ray.Position.z < (matrix.Position.z + matrix.Size.z))

func display(disp:Node, values:PackedByteArray):
	var img : Image = Image.create_from_data(WSX, WSY, false, Image.FORMAT_RGBA8, values)
	var tex : Texture2D = ImageTexture.create_from_image(img)
	disp.set_texture(tex)
