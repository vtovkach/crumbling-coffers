extends Control

@onready var hotbar: Hotbar = preload("res://Inventory/Sub_Inventory/playerHotbar.tres")
@onready var slots: Array = $HBoxContainer.get_children()

func _ready() -> void:
	hotbar.update.connect(self.update_hotbar_slots)
	update_hotbar_slots()

# the update function for the hotbar will be the same functionality as the inventory's.
func update_hotbar_slots():
	for i in range(min(hotbar.hotbar_slots.size(), slots.size())):
		slots[i].update(hotbar.hotbar_slots[i])
