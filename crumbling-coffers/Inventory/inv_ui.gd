extends Control

@onready var inventory: Inventory = preload("res://Inventory/playerInventory.tres")
@onready var slots: Array = $NinePatchRect/GridContainer.get_children()
var is_open: bool = false

func _ready():
	update_slots()
#	close()

func update_slots():
	for i in range(min(inventory.items.size(), slots.size())):
		slots[i].update(inventory.items[i])

func open():
	self.visible = true
	is_open = true
	
func close():
	self.visible = false
	is_open = false
	
