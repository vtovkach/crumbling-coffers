extends Node
class_name ItemIndicatorManager

@export var indicator_scene: HUD
var player: Player

func _ready() -> void:
	pass
	

func setPlayer(p: Player) -> void:
	player = p
