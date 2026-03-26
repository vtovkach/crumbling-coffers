# There could be generalized indicators / inheritance, 
# but typing concerns and me not yet willing to add "Indicatable" interface to everything that exists
# makes me less willing to add the generalization at this moment.

# So for now, this 2-point item indicator will just extend Node2D.
# even though 1-point indicators, 2-point non-item indicators, etc [may] be desired

extends Node2D
class_name TwoPointItemIndicator

var source: Player
var target: PickupBase
# var bounds: This thing needs to be aware of its bounds

func spawn(player: Player, item: PickupBase) -> void:
	source = player
	target = item
	# Add this to a group of indicators

func destroy() -> void:
	source = null
	target = null
	# Remove this from the group of indicators
	
func update() -> void:
	# Set angle from source to target
	# Set position between source and target but within bounds
	
	
	pass


	
