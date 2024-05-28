extends Node

@onready var dispRetina:Node = $"../Retina"
@onready var dispMatrix:Node = $"../Matrice3D"
var WSX:int = 128
var WSY:int = 128
var step:int = 0
var data_0:PackedByteArray = PackedByteArray()
var data_1:PackedByteArray = PackedByteArray()

class Matrix:
	var Position:Vector3
	var Size:Vector3

class Retina:
	var Position: Vector3
	var Width: float
	var Lenght: float

class Eye:
	var Position:Vector3

class Ray:
	var Position:Vector3
	var Direction:Vector3

var matrix = Matrix.new()
var retina = Retina.new()
var eye = Eye.new()
var MAX_RAY_LENGTH:int
var color: int = 0  # color ini

# Called when the node enters the scene tree for the first time.
func _ready():
	print("enter: step == 0")
	
	# If stable version: Put all the variables here.
	# Matrix variables
	matrix.Position = Vector3(0, 0, 0)
	matrix.Size = Vector3(128.0, 128.0, 1.0)
	
	# Calculate once, MAX_RAY_LENGTH
	MAX_RAY_LENGTH = calculate_max_ray_length()
	
	# Create a checkerboard pattern for data_1
	for y in range(WSY):
		for x in range(WSX):
			@warning_ignore("integer_division")
			if (x / 16 + y / 16) % 2 == 0:
				# Bellow is equivalent to 0xFF0000FF (Red in RGBA format)
				data_1.append(0xFF)  # Red
				data_1.append(0x00)  # Green
				data_1.append(0x00)  # Blue
				data_1.append(0xFF)  # Alpha
			else:
				# Bellow is equivalent to 0x00FF00FF (Green in RGBA format)
				data_1.append(0x00)  # Red
				data_1.append(0xFF)  # Green
				data_1.append(0x00)  # Blue
				data_1.append(0xFF)  # Alpha
	
	# Init data_0 with zeros
	for i in range(WSX * WSY * 4):
		data_0.append(0)

	display(dispMatrix,data_1)

# data_0 is for the "retina". data_1 is for the matrix.
@warning_ignore("unused_parameter")
func _process(delta):
	print("step == " + str(step))
	
	var retina_distance: float = 170 # Distance above the matrix
	var retina_size_factor: float = 0.06 # Size of the retina as a fraction of the matrix size

	# Retina variables
	retina.Width = matrix.Size.x * retina_size_factor
	retina.Lenght = matrix.Size.y * retina_size_factor
	
	retina.Position = Vector3(matrix.Position.x + matrix.Size.x / 2 - (retina.Width / 2),
							  matrix.Position.y + matrix.Size.y / 2 - (retina.Lenght / 2),
							  matrix.Position.z + retina_distance)

	# Eye variables
	eye.Position = Vector3(retina.Position.x + (retina.Width / 2),
						   retina.Position.y + (retina.Lenght / 2),
						   retina.Position.z + 10)
	var ray = Ray.new()
	## ROTATION ##
	var angle = 5*step
	rotate_eye_and_retina(angle)
	#rotate_matrix(5*step)
	print("Matrix pos: " + str(matrix.Position))
	print("Retina pos: " + str(retina.Position))
	print("Eye pos: " + str(eye.Position))
	print("Current Angle: " + str(angle))
	
	# Ray Tracing
	for x in range(WSX):
		for y in range(WSY):
			var p: int = x + y * WSX
			ray.Position = Vector3(retina.Position.x + x * retina.Width / matrix.Size.x,
								   retina.Position.y + y * retina.Lenght / matrix.Size.y,
								   retina.Position.z)

			ray.Direction = Vector3(ray.Position.x - eye.Position.x,
									ray.Position.y - eye.Position.y,
									ray.Position.z - eye.Position.z).normalized()

			for i in range(MAX_RAY_LENGTH):
				if (is_in_matrix(ray)):
					var p_ray: int = int(ray.Position.x) + int(ray.Position.y) * int(matrix.Size.y)
					p_ray *= 4
					color = (data_1[p_ray] << 24) + (data_1[p_ray + 1] << 16) + (data_1[p_ray + 2] << 8) + (data_1[p_ray + 3])
					break
				else:
					color = 0
				ray.Position += ray.Direction

			data_0[p * 4] = color >> 24
			data_0[p * 4 + 1] = color >> 16
			data_0[p * 4 + 2] = color >> 8
			data_0[p * 4 + 3] = color & 0xFF
			
	display(dispRetina, data_0)
	step = step + 1

func rotate_eye_and_retina(angle_degrees: float):
	var angle: float = deg_to_rad(angle_degrees)
	var cos_angle: float = cos(angle)
	var sin_angle: float = sin(angle)

	# Rotate Eye Position
	var rotated_eye_pos: Vector3 = Vector3(
		eye.Position.x * cos_angle + eye.Position.z * sin_angle,
		eye.Position.y,
		-eye.Position.x * sin_angle + eye.Position.z * cos_angle
	)

	# Rotate Retina Position
	var rotated_retina_pos: Vector3 = Vector3(
		retina.Position.x * cos_angle + retina.Position.z * sin_angle,
		retina.Position.y,
		-retina.Position.x * sin_angle + retina.Position.z * cos_angle
	)

	# Update positions
	eye.Position = rotated_eye_pos
	retina.Position = rotated_retina_pos

func normalize(old_dir)->Vector3:
	# First, we calculate the length of the ray vector using the Euclidean distance formula
	var L:float = sqrt((pow(old_dir.x,2)) + (pow(old_dir.y,2)) + (pow(old_dir.z,2)))
	# Normalizing the vector
	var new_dir:Vector3 = Vector3((old_dir.x / L), (old_dir.y / L), (old_dir.z / L))
	return new_dir

func calculate_max_ray_length() -> int:
	# Calculating the diagonal distance across the 3D matrix and from the retina
	var diagonal_distance: float = sqrt(pow(matrix.Size.x, 2) + pow(matrix.Size.y, 2) + pow(retina.Position.z, 2))
	# Safety margin (10%)
	var safety_margin: float = 0.1 * diagonal_distance

	return int(diagonal_distance + safety_margin)

func is_in_matrix(curRay: Ray)->bool:
	return (curRay.Position.x >= matrix.Position.x and 
			curRay.Position.y >= matrix.Position.y and 
			curRay.Position.z >= matrix.Position.z and 
			curRay.Position.x < (matrix.Position.x + matrix.Size.x) and 
			curRay.Position.y < (matrix.Position.y + matrix.Size.y) and 
			curRay.Position.z < (matrix.Position.z + matrix.Size.z))

func display(disp:Node, values:PackedByteArray):
	var img : Image = Image.create_from_data(WSX, WSY, false, Image.FORMAT_RGBA8, values)
	var tex : Texture2D = ImageTexture.create_from_image(img)
	disp.set_texture(tex)

