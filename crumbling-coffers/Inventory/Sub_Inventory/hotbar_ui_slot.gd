extends Panel

@onready var hotbarItemVisual: Sprite2D = $CenterContainer/Panel/item_display
@onready var hotbarAmtVisual: Label = $CenterContainer/Panel/amount
# Adding variable for the bg node in order to update slot.
@onready var slot_visual: Sprite2D = $bg
# initially set the slot that's being selected to be false.
var slot_active: bool = false


# This update function is the same as the inventory's logic. Name conventions are changed to
# associate with hotbar instead of inventory.
func update(hotbar_slot: HotbarSlot):
	if !hotbar_slot.hotbar_item:
		hotbarItemVisual.visible = false
		hotbarAmtVisual.visible = false
	else:
		hotbarItemVisual.visible = true
		hotbarItemVisual.texture = hotbar_slot.hotbar_item.texture
		if hotbar_slot.amount > 1: 
			hotbarAmtVisual.visible = true
		hotbarAmtVisual.text = str(hotbar_slot.amount)

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
