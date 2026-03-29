extends Panel

@onready var hotbarItemVisual: Sprite2D = $CenterContainer/Panel/item_display
@onready var hotbarAmtVisual: Label = $CenterContainer/Panel/amount
# Adding variable for the bg node in order to update slot.
@onready var slot_visual: Sprite2D = $bg
# initially set the slot that's being selected to be false.
var slot_active: bool = false
var current_slot: HotbarSlot

# Mouse detection signals.
#signal hovering_on(item_name)
#signal hovering_off


#func _ready():
#	mouse_entered.connect(_on_mouse_entered)
#	mouse_exited.connect(_on_mouse_exited)

# This update function is the same as the inventory's logic. Name conventions are changed to
# associate with hotbar instead of inventory.
# UPDATED "update" function to handle the updating more cleanly and safely.
func update(hotbar_slot: HotbarSlot):
	current_slot = hotbar_slot
	# Checking both conditions if there is no item or if hotbar_slot == null.
	if (!hotbar_slot.hotbar_item) || (hotbar_slot == null):
		hotbarItemVisual.visible = false
		hotbarAmtVisual.visible = false
		return

# Taken out "else" condition, so if the "if" condition is not met, automatically update visibility.
	hotbarItemVisual.visible = true
	hotbarItemVisual.texture = hotbar_slot.hotbar_item.texture
	
	# Also adding when amount is < 1, then the visibility of the amount will be disabled.
	if hotbar_slot.amount > 1: 
		hotbarAmtVisual.visible = true
		hotbarAmtVisual.text = str(hotbar_slot.amount)
	else: 
		hotbarAmtVisual.visible = false


# Update function that will change the visual from a normal slot png to the highlighted slot png..
func update_active_slot_visual():
	if slot_active: # if true
		slot_visual.texture = preload("res://Assets/Items/PNG/inventoryPNG/inventory-slot-highlighted.png")
	else: # if not true, then not active, so switch texture to non-highlighted.
		slot_visual.texture = preload("res://Assets/Items/PNG/inventoryPNG/inventory-slot.png")

# Set function to update the highlighted slot visuals.
func set_active_slot(value: bool):
	slot_active = value
	update_active_slot_visual()


#func _on_mouse_entered() -> void:
#	if current_slot and current_slot.hotbar_item:
#		hovering_on.emit(current_slot.hotbar_item.name)


#func _on_mouse_exited() -> void:
#	hovering_off.emit()
