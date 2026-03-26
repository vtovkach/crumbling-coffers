# There could be generalized indicators / inheritance, 
# but typing concerns and me not yet willing to add "Indicatable" interface to everything that exists
# makes me less willing to add the generalization at this moment.

# So for now, this 2-point item indicator will just extend Node2D.
# even though 1-point indicators, 2-point non-item indicators, etc [may] be desired

extends Node2D
class_name TwoPointItemIndicator

@export var source: Player
@export var target: PickupBase

var dir: Vector2

var camera_node: Camera2D
var bounds: Vector2
var margin: float = 4


func _ready() -> void:
	camera_node = get_viewport().get_camera_2d()
	bounds = get_viewport_rect().size

func _process(delta: float) -> void:
	if not target or not source:	# safety
		return

	dir = target.global_position - source.global_position
	_update_rotation()
	_update_position()


func init(player: Player, item: PickupBase) -> void:
	source = player
	target = item

func _update_rotation() -> void:
	# Rotate to face target
	rotation = atan2(dir.y, dir.x)
	
func _update_position() -> void:
	# Place at screen edge between source and target

	# Imagine a "ray" from the center of screen to the item. Place item along the "ray" at the border of screen (with margin included)
	var viewport_size = get_viewport_rect().size
	var center = viewport_size * 0.5
	var dir_normalized = dir.normalized()
	var ray_scale: Vector2
	if dir.x != 0 and dir.y != 0:
		ray_scale.x = (center.x - margin) / abs(dir_normalized.x) if dir.x != 0 else (center.y - margin)/ abs(dir_normalized.y)
		ray_scale.y = (center.y - margin)/ abs(dir_normalized.y) if dir.y != 0 else ray_scale.x
	else:
		ray_scale = Vector2(0, 0) # realistically this should never occur 

	# this is calculated position as screen coordinates. Buuut the point of reference isn't the screen, this should be fixed later. 
	position = center + dir.normalized() * min(ray_scale.x, ray_scale.y) 
