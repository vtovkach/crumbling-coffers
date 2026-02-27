extends Node2D

@onready var player = $Player
@onready var hud = $HUD

func _ready() -> void:
	hud.bind_to_player(player)
