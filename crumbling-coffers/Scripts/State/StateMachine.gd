extends Node

var active_state : State
var states : Dictionary = {}

func _ready() -> void:
	for child in get_children():
		if child is State:
			states[child.name] = child

func _process(delta: float) -> void:
	if active_state:
		active_state.update(delta)

func _physics_process(delta: float) -> void:
	if active_state:
		active_state.update_physics(delta)
