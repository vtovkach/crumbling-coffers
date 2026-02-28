extends Area2D

@export var points: int = 1
@export var float_amplitude: float = 10
@export var float_speed: float = 2.0

var initial_y: float
var random_offset: float

func _ready() -> void:
	initial_y = position.y
	random_offset = randf_range(0.0, PI * 2)
	body_entered.connect(_on_body_entered)

func _process(delta: float) -> void:
	var time = (Time.get_ticks_msec() / 1000.0) * float_speed
	var y_offset = sin(time + random_offset) * float_amplitude
	position.y = initial_y + y_offset

func _on_body_entered(body: Node) -> void:
	on_collected(body)
	queue_free()

func on_collected(body: Node) -> void:
	# meant to be overridden
	pass
