extends Control

@onready var inventory: Inventory = preload("res://Inventory/playerInventory.tres")
@onready var slots: Array = $NinePatchRect/GridContainer.get_children()
var is_open: bool = false

func _ready():
	inventory.update.connect(self.update_slots)
	update_slots()	# Called to have the inventory be empty at start.

func update_slots():
	for i in range(min(inventory.slots.size(), slots.size())):
		slots[i].update(inventory.slots[i])

func open():
	self.visible = true
	is_open = true
	
func close():
	self.visible = false
	is_open = false
	
