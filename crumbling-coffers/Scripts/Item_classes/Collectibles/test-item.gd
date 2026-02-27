extends Common

var ab = ability # set ability "ab" status to false. This is NOT an ability item.

func _on_body_entered(body: CharacterBody2D) -> void:
	_on_item_collected()
	abilityCheck(ab)
	
