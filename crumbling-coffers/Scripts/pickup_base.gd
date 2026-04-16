extends Area2D
class_name PickupBase

@export var points: int = 1
@export var float_amplitude: float = 10
@export var float_speed: float = 2.0
@export var color = Color("#FFFFFF")	# default

var initial_y: float
var random_offset: float

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
	on_collected(body)
	remove_from_group("pickups") # remove this item from that group of pickups. this is technically "on_collected" behavior but it may not be worth it to place this there
	queue_free()

func on_collected(body: Node) -> void:
	# meant to be overridden
	pass
