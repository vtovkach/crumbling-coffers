extends "res://Scripts/pickup_base.gd"

#Adding item resource variable that will connect corresponding .tres resources to .tscn scenes.
@export var itemRes: InventoryItem

func _ready() -> void:
	super()
	points = 10

func on_collected(body: Node) -> void:
	if body.has_method("add_score"):
		body.add_score(points)
	# When player (assigned to body) walks over item, it will also call the collect item in player.gd,
	# which will insert the item into the inventory UI.
	if body.has_method("collect"):
		body.collect(itemRes)
