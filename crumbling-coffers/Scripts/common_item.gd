<<<<<<< HEAD
extends "res://Scripts/pickup_base.gd"

func _ready() -> void:
	super()
	points = 5

func on_collected(body: Node) -> void:
	if body.has_method("add_score"):
		body.add_score(points)
=======
extends Area2D


# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	print("This is a basic item.")


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass
>>>>>>> origin/PROJ-21-design-ability-item-system-spec
