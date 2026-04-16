# There could be generalized indicators / inheritance, 
# but typing concerns and me not yet willing to add "Indicatable" interface to everything that exists
# makes me less willing to add the generalization at this moment.

# So for now, this 2-point item indicator will just extend Node2D.
# even though 1-point indicators, 2-point non-item indicators, etc [may] be desired

extends Node2D
class_name TwoPointItemIndicator

@export var source: UserPlayer
@export var target: PickupBase

var dir: Vector2
var bounds: Vector2
var margin: float = 24

var fade_distance = 50 #px
var indicator_speed = 384	# This value is sort of placeholder. Decreasing makes indicator change positions slower

@onready var sprite: Sprite2D = $Sprite2D

func _ready() -> void:
	z_index = -1
	bounds = get_viewport_rect().size

func _process(delta: float) -> void:
	
	if not target or not source:	# safety
		return
		
	bounds = get_viewport_rect().size
	dir = target.global_position - source.global_position

	_update_rotation()
	_update_position(delta)

func init(player: Player, item: PickupBase) -> void:
	source = player
	target = item
	_quick_set_position()
	set_initial_color(item.color)

func _update_rotation() -> void:
	# Rotate to face target
	rotation = atan2(dir.y, dir.x)
	
func _update_position(delta: float) -> void:
	var new_position: Vector2 = _calculate_screen_position()
	
	# Below code will make the movement of indicator slightly more clunky. Which I think is good. Clunkiness can be adjusted with new speed variable
	position.x = move_toward(position.x, new_position.x, indicator_speed * delta)
	position.y = move_toward(position.y, new_position.y, indicator_speed * delta)

func _quick_set_position() -> void:
	dir = target.global_position - source.global_position
	position = _calculate_screen_position()

func _calculate_screen_position() -> Vector2:
	# Place at screen edge between source and target

	# Imagine a "ray" from the center of screen to the item. Place item along the "ray" at the border of screen (with margin included)
	var center = bounds * 0.5
	var dir_normalized = dir.normalized()
	var ray_scale: Vector2
	if dir.x != 0 and dir.y != 0:
		ray_scale.x = (center.x - margin) / abs(dir_normalized.x) if dir.x != 0 else (center.y - margin)/ abs(dir_normalized.y)
		ray_scale.y = (center.y - margin)/ abs(dir_normalized.y) if dir.y != 0 else ray_scale.x
	else:
		ray_scale = Vector2(0, 0) # realistically this should never occur 

	# this is calculated position as screen coordinates. Buuut the point of reference isn't the screen, this should be fixed later. 
	var new_position: Vector2 = center + dir.normalized() * min(ray_scale.x, ray_scale.y) 
	
	return new_position

func set_initial_color(color: Color):
	recolor(color)
	sprite.modulate.a = 0

func recolor(color: Color):
	sprite.modulate = color

# Fade in and out when near inner and outer margin
func apply_fade(inner_rect: Rect2, outer_rect: Rect2) -> void:
	sprite.modulate.a = _calculate_fade(inner_rect, outer_rect)
	
func _calculate_fade(inner_rect: Rect2, outer_rect: Rect2) -> float:
	var screen: Vector2 = get_viewport().get_canvas_transform() * target.global_position
	
	var inner_dist = _dist_in_rect(screen, inner_rect)
	var outer_dist = _dist_in_rect(screen, outer_rect)

	# produce a nice and fast fade. ai helped with the math
	var inner_alpha = clampf(-inner_dist / fade_distance, 0.0, 1.0)
	var outer_alpha = clampf(outer_dist / fade_distance, 0.0, 1.0)
	return inner_alpha * outer_alpha
	
func _dist_in_rect(point: Vector2, rect: Rect2) -> float:
	return minf(
		minf(point.x - rect.position.x, rect.end.x - point.x),
		minf(point.y - rect.position.y, rect.end.y - point.y)
	)
