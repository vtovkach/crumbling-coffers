extends PickupItems
class_name Rare

var rarity_type = "rare"
var item_type = "collectible"
var rare_score = 5		# default score. Can be changed for each subclass.

func _ready() -> void:
	_set_rarity(rarity_type)
	_set_itemType(item_type)
	_set_score(rare_score)
