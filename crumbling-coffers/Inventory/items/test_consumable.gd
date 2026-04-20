extends "res://Scripts/pickup_base.gd"

@export var hotbar_itemRes: HotbarItem

func _ready() -> void:
	super()
	points = 0

func on_collected(body: Node) -> bool:
	if not body.is_in_group("player"):
		return false

	if hotbar_itemRes == null:
		return false

	if body.has_method("consumable_collect"):
		body.consumable_collect(hotbar_itemRes)
		return true

	return false
