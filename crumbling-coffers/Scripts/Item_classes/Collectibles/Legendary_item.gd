extends PickupItems
class_name Legendary

var rarity_type = "legendary"
var item_type = "collectible"
var legendary_score = 20		# default score. Can be changed for each subclass.

func _ready() -> void:
	_set_rarity(rarity_type)
	_set_itemType(item_type)
	_set_score(legendary_score)
