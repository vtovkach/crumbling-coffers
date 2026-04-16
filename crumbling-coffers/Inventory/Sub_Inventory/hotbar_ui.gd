extends Control

@onready var hotbar: Hotbar = preload("res://Inventory/Sub_Inventory/playerHotbar.tres")
@onready var slots: Array = $HBoxContainer.get_children()
# Signal to send out that the active item is highlighting a new item.
signal active_item_updated
# variable that will keep track of what index the selection is on.
var active_item_slot = 0



func _ready():
	hotbar.update.connect(self.update_hotbar_slots)
	# connect the active_item_updated signal 
	active_item_updated.connect(self.update_active_item_visual)
	# call the updates to have scene be up to date when first ran.
	update_hotbar_slots()
	update_active_item_visual()

# the update function for the hotbar will be the same functionality as the inventory's.
func update_hotbar_slots():
	for i in range(min(hotbar.hotbar_slots.size(), slots.size())):
		slots[i].update(hotbar.hotbar_slots[i])
		# calling to ensure that the texture is swapped to the highlighted slot.
	update_active_item_visual()

func update_active_item_visual():
	for i in range(slots.size()):
		slots[i].set_active_slot(i == active_item_slot)
		# function "set_active()" will update the visuals. 
		# Set function in hotbar_slot_ui.gd script.


func active_item_scroll_up():
	# Using modulation so the active item slot does not go out of bounds.
	active_item_slot = (active_item_slot + 1) % hotbar.hotbar_slots.size()
	active_item_updated.emit()

func active_item_scroll_down():
	if active_item_slot == 0:
		active_item_slot = hotbar.hotbar_slots.size() - 1
	else:
		active_item_slot -= 1
	
	active_item_updated.emit()

# Functionality for using numbers to select hotbar slot.
func set_active_slot(index: int):
	# Return, and change/do nothing, if index is out of bounds.
	if (index < 0) || (index >= hotbar.hotbar_slots.size()):
		return
	# Assign the index number to active_item_slot variable, then update.
	active_item_slot = index
	active_item_updated.emit()


# ADDING MOUSE HOVERING DETECTION.
func hovering_started(slots) -> void:
	pass

func hovering_ended() -> void:
	pass

# Will check if the button (right mouse click) is pressed to use the highlighted item.
# Will only remove the selected item from the array for now.
func _input(event):
	# Adding in the ability to use an item -- will only currently remove it from hotbar.
	if event.is_action_pressed("item_used"): # "item_used" is right mouse click.
		use_active_item()

func use_active_item():
	hotbar.use_item(active_item_slot, get_tree())
