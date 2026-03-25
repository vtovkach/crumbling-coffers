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
	#connect the item_used signal
	hotbar.item_used.connect(hotbar._on_item_used)
	

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

# Will check if the button (right mouse click) is pressed to use the highlighted item.
# Will only remove the selected item from the array for now.
func _input(event):
	# Adding in the ability to use an item -- will only currently remove it from hotbar.
	if event.is_action_pressed("item_used"):
		use_active_item()


func use_active_item():
	var selected_item = hotbar.hotbar_slots[active_item_slot]
	
	if selected_item == null:
		return # return to ensure no game crash -- does nothing.
	
	# This function will be implemented to have the item affect the player/other players.
	# Not defined for now. Need to define in the future.
	# apply_item_effect(selected_item)
	
	# Send out signal that an item was used.
	hotbar.item_used.emit(active_item_slot)
