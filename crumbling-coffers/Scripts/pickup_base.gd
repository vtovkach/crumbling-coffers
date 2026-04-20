extends Area2D
class_name PickupBase

@export var points: int = 1
@export var float_amplitude: float = 10
@export var float_speed: float = 2.0
@export var color = Color("#FFFFFF")	# default

var initial_y: float
var random_offset: float
var is_collection_pending: bool = false

func _ready() -> void:
	initial_y = position.y
	random_offset = randf_range(0.0, PI * 2)
	body_entered.connect(_on_body_entered)
	add_to_group("pickups")	# THIS IS ONLY IN PICKUP_BASE BECAUSE all pickups should be in a group including those NOT spawned by item_spawner_manager
							# i.e. item spawner manager's internal list doesn't store items that it doesn't spawn (aka items existing originally)

func _process(delta: float) -> void:
	var time = (Time.get_ticks_msec() / 1000.0) * float_speed
	var y_offset = sin(time + random_offset) * float_amplitude
	position.y = initial_y + y_offset

func _on_body_entered(body: Node) -> void:
	if is_collection_pending:
		return
		
	if not can_be_collected_by(body):
		return
	
	is_collection_pending = true
	
	if try_collect(body):
		finalize_collection()
	else:
		is_collection_pending = false
	
	
func can_be_collected_by(body: Node) -> bool:
	return body.is_in_group("player") and body.has_method("receive_pickup")
	
func finalize_collection() -> void:
	remove_from_group("pickups")
	set_deferred("monitoring", false)
	set_deferred("monitorable", false)
	set_deferred("collision_layer", 0)
	set_deferred("collision_mask", 0)
	queue_free()
	
func try_collect(body: Node) -> bool:
	return on_collected(body)

func on_collected(body: Node) -> bool:
	# meant to be overridden
	return false
