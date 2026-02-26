extends Area2D

@export var points: int = 1

func _ready() -> void:
	body_entered.connect(_on_body_entered)

func _on_body_entered(body: Node) -> void:
	on_collected(body)
	queue_free()

func on_collected(body: Node) -> void:
	# meant to be overridden
	pass
