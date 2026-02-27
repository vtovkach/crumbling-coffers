extends PickupItems
class_name Debuffs

# This subclass will act as a parent class for "debuff" items that will be seen by other players and 
# placed into the environment. 
	# example(/potential) debuff items: slowness handheld, sludge, freeze.

var rarity_type = "null" # having "null" since not discusssed/declared if consumables will have rarity rankings.
var item_type = "powerup"
var score_val = 1
var ab = true	# shorterning ability to "ab"

func _ready() -> void:
	_set_rarity(rarity_type)
	_set_itemType(item_type)
	_set_score(score_val)
	_set_ability(ab)
