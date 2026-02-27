extends PickupItems
class_name Common

var rarity_type = "common"
var item_type = "collectible"
var common_score = 1		# default score. Can be changed for each subclass.

func _ready() -> void:
	_set_rarity(rarity_type)
	_set_itemType(item_type)
	_set_score(common_score)
