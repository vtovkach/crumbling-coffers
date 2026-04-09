extends State
class_name PlayerState

@export var player: ControllablePlayer

func enter() -> void:
	player.setState(self)
