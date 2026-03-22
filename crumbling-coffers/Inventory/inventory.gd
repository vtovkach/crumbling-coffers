extends Resource

class_name Inventory

# Signal that will be sent out to tell inv_ui to update the changes that were made from func "insert".
signal update

@export var slots: Array[InventorySlot]

# Use of Godot's array filtering to parse through and check if a slot has a pre-existing item in it or not. 
# When checking, if the slot has a corresponding item slot, will fill it and assign amount to 1 (if not created yet)..
# If there is already an item that the player collected in the slot, then will add to that specific slot.
func insert(item: InventoryItem):
	var itemSlots = slots.filter(func(slot): return slot.item == item) 
	if !itemSlots.is_empty():
		itemSlots[0].amount += 1
	else:
		var emptySlots = slots.filter(func(slot): return slot.item == null)
		if !emptySlots.is_empty():
			emptySlots[0].item = item
			emptySlots[0].amount = 1
	
	update.emit()
