extends "res://Scripts/pickup_base.gd"


@export var disorientation_duration: float = 5.0

@export var hotbar_itemRes: HotbarItem
func _ready() -> void:
	super()
	points = 0

func on_collected(body: Node) -> void:
	if body.has_method("add_score"):
		body.add_score(points)
	# When player (assigned to body) walks over item, it will also call the collect item in player.gd,
	# which will insert the item into the inventory UI.
	if body.has_method("consumable_collect"):
		body.consumable_collect(hotbar_itemRes)
