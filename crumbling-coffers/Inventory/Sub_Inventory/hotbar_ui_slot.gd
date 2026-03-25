extends Panel

@onready var hotbarItemVisual: Sprite2D = $CenterContainer/Panel/item_display
@onready var hotbarAmtVisual: Label = $CenterContainer/Panel/amount

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
