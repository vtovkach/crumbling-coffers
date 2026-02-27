extends Node2D
# Defining the "parent" item class that child item classes will inherit from.
class_name PickupItems	# Class name.

# #########################################
# ### Variable descriptions/explanations ### 
# ItemType: used to define if the item is a collectible or an ability/consumable.
# ability: for consumable items - set their ability to bool and specify directly in "subclass" what it does 
	# if set to TRUE.
		# !! (Up for improvements. This is a basic implementation for now.) !!
# rarity: for collectibles. Common, rare, legendary status is assigned here.
# score: how much value the item is worth.
		# !! Going forth with the idea that ability/consumable items will have a score assigned to them 
		# and will contribute to the final score if not used. 
			# Not finalized design; up to changes in the future. !!
# ##########################################

var itemType : String
@export var rarity : String	# rarity can be assigned outside of the script in "Inspector" by using @export.
var ability : bool
@export var score : int


func _ready() -> void:
	pass

func _set_itemType(label : String):
	itemType = label

func _set_rarity(new_rarity : String):
	rarity = new_rarity

func _set_score(new_score : int):
	score = new_score

func _on_item_collected():
	print("I've been picked up!")
# More sophisticated function will be implemented in future tasks. Placeholder in order to test functions work
	# and collision signals are being received.
