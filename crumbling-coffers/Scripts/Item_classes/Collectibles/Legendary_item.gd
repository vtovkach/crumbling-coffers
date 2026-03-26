extends PickupBase

@export var itemRes: InventoryItem

func _ready() -> void:
	super()
	points = 35

func on_collected(body: Node) -> void:
	if body.has_method("add_score"):
		body.add_score(points)
	
	if body.has_method("collect"):
		body.collect(itemRes)
