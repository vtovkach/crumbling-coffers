extends PickupBase

@export var itemRes: InventoryItem

func _ready() -> void:
	color = Color("#DA1")
	super()
	points = 35

func on_collected(body: Node) -> void:
	if body.has_method("add_score"):
		body.add_score(points)
	
	if body.has_method("collect"):
		body.collect(itemRes)
