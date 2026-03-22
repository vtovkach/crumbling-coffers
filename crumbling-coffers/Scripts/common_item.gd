extends "res://Scripts/pickup_base.gd"

@export var item: InventoryItem
var player = null

func _ready() -> void:
	super()
	points = 5

func on_collected(body: Node) -> void:
	if body.has_method("add_score"):
		body.add_score(points)
	
	if body.has_method("player"):
		body.collect(item)
