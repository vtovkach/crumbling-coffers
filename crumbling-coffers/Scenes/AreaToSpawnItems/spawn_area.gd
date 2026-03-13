extends Area2D

@export var spawn_padding: float = 8.0


func _ready():
	add_to_group("spawn_areas")
	
	
func get_random_point() -> Vector2:
	var shape = $CollisionShape2D.shape
	
	if shape is RectangleShape2D:
		var extents = shape.size * 0.5
		
		var x = randf_range(-extents.x + spawn_padding, extents.x - spawn_padding)
		var y = randf_range(-extents.y + spawn_padding, extents.y - spawn_padding)
		
		return global_position + Vector2(x, y)
	
	return global_position
