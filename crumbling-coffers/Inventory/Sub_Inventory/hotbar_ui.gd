extends Control

@onready var hotbar: Hotbar = preload("res://Inventory/Sub_Inventory/playerHotbar.tres")
@onready var slots: Array = $HBoxContainer.get_children()

func _ready() -> void:
	pass

# the update function for the hotbar will be the same functionality as the inventory's.
func update_hotbar_slots():
	for i in range(min(hotbar.slots.size(), slots.size())):
		slots[i].update(hotbar.slots[i])
