extends Node

class UDPPacket:
	var payload: PackedByteArray
	var port: int

	func _init(p: PackedByteArray, prt: int) -> void:
		payload = p
		port = prt

const FramerTCP = preload("res://Netcode/tcp_framer.gd")

const PACKET_SIZE: int = 200
const MAX_RECONNECT_ATTEMPTS: int = 3
const RECONNECT_DELAY_MS: int = 2000
const TCP_CONNECT_TIMEOUT_MS: int = 5000
const THREAD_TICK_MS: int = 1

var server_ip: String = "129.146.77.151"
var server_tcp_port: int = 10000

var _server_tcp: StreamPeerTCP
var _udp: PacketPeerUDP
var _tcp_framer: FramerTCP

# Incoming queues (populated by thread, consumed by game)
var _in_tcp: Array[PackedByteArray] = []
var _in_udp: Array[PackedByteArray] = []

# Outgoing queues (populated by game, consumed by thread)
var _out_tcp: Array[PackedByteArray] = []
var _out_udp: Array[UDPPacket] = []

var _mutex_in_tcp: Mutex
var _mutex_in_udp: Mutex
var _mutex_out_tcp: Mutex
var _mutex_out_udp: Mutex
var _thread: Thread
var _running: bool = false

var _notification_panel: Panel
var _notification_panel_style: StyleBoxFlat
var _notification_label: Label
var _reconnect_btn: Button

func _ready() -> void:
	_init_notification_ui()
	startup()

func startup() -> void:
	_mutex_in_tcp = Mutex.new()
	_mutex_in_udp = Mutex.new()
	_mutex_out_tcp = Mutex.new()
	_mutex_out_udp = Mutex.new()
	_in_tcp.clear()
	_in_udp.clear()
	_out_tcp.clear()
	_out_udp.clear()
	_running = true
	_thread = Thread.new()
	_thread.start(_thread_main)

func _exit_tree() -> void:
	_running = false
	if _thread and _thread.is_started():
		_thread.wait_to_finish()
	if _udp:
		_udp.close()
	if _server_tcp:
		_server_tcp.disconnect_from_host()

# ========================= Thread =========================

func _thread_main() -> void:
	_udp = PacketPeerUDP.new()
	var udp_err := _udp.bind(0)
	if udp_err != OK:
		push_error("NetworkManager: Failed to bind UDP socket: %s" % udp_err)
		call_deferred("_show_disconnect_notification")
		return

	if not _connect_with_retries():
		call_deferred("_show_disconnect_notification")
		return

	call_deferred("_on_connection_restored")

	while _running:
		_server_tcp.poll()
		if _server_tcp.get_status() != StreamPeerTCP.STATUS_CONNECTED:
			if not _connect_with_retries():
				call_deferred("_show_disconnect_notification")
				break
			call_deferred("_on_connection_restored")

		# Read incoming TCP frames into _in_tcp
		_tcp_framer.process()
		while _tcp_framer.has_frame():
			var frame := _tcp_framer.get_frame()
			# tcp_framer already guarantees PACKET_SIZE frames, but double-check
			if frame.size() != PACKET_SIZE:
				push_error("NetworkManager: Dropping malformed TCP frame of size %d" % frame.size())
				continue
			_mutex_in_tcp.lock()
			_in_tcp.append(frame)
			_mutex_in_tcp.unlock()

		# Read incoming UDP packets into _in_udp
		while _udp.get_available_packet_count() > 0:
			var packet := _udp.get_packet()
			if _udp.get_packet_error() != OK:
				continue
			if packet.size() != PACKET_SIZE:
				push_error("NetworkManager: Dropping UDP packet with wrong size: %d" % packet.size())
				continue
			_mutex_in_udp.lock()
			_in_udp.append(packet)
			_mutex_in_udp.unlock()

		# Drain outgoing TCP queue
		_mutex_out_tcp.lock()
		var tcp_out := _out_tcp.duplicate()
		_out_tcp.clear()
		_mutex_out_tcp.unlock()
		for pkt in tcp_out:
			_tcp_framer.send_server_tcp(pkt)

		# Drain outgoing UDP queue
		_mutex_out_udp.lock()
		var udp_out := _out_udp.duplicate()
		_out_udp.clear()
		_mutex_out_udp.unlock()
		for udp_pkt: UDPPacket in udp_out:
			_udp.set_dest_address(server_ip, udp_pkt.port)
			_udp.put_packet(udp_pkt.payload)

		OS.delay_msec(THREAD_TICK_MS)

func _connect_with_retries() -> bool:
	for attempt in range(MAX_RECONNECT_ATTEMPTS):
		print("NetworkManager: TCP connect attempt %d/%d..." % [attempt + 1, MAX_RECONNECT_ATTEMPTS])
		if _try_connect_tcp():
			print("NetworkManager: TCP connected.")
			return true
		if attempt < MAX_RECONNECT_ATTEMPTS - 1:
			OS.delay_msec(RECONNECT_DELAY_MS)
	push_error("NetworkManager: All reconnection attempts failed.")
	return false

func _try_connect_tcp() -> bool:
	if _server_tcp and _server_tcp.get_status() != StreamPeerTCP.STATUS_NONE:
		_server_tcp.disconnect_from_host()

	_server_tcp = StreamPeerTCP.new()
	var err := _server_tcp.connect_to_host(server_ip, server_tcp_port)
	if err != OK:
		return false

	var elapsed := 0
	while elapsed < TCP_CONNECT_TIMEOUT_MS:
		_server_tcp.poll()
		var status := _server_tcp.get_status()
		if status == StreamPeerTCP.STATUS_CONNECTED:
			_tcp_framer = FramerTCP.new(_server_tcp, PACKET_SIZE)
			return true
		elif status == StreamPeerTCP.STATUS_ERROR:
			return false
		OS.delay_msec(10)
		elapsed += 10

	return false

# ========================= Notification UI =========================

func _init_notification_ui() -> void:
	var canvas := CanvasLayer.new()
	canvas.layer = 100
	add_child(canvas)

	var root_control := Control.new()
	root_control.set_anchors_preset(Control.PRESET_FULL_RECT)
	root_control.mouse_filter = Control.MOUSE_FILTER_IGNORE
	canvas.add_child(root_control)

	_notification_panel = Panel.new()
	_notification_panel_style = StyleBoxFlat.new()
	_notification_panel_style.set_corner_radius_all(4)
	_notification_panel_style.set_content_margin_all(12.0)
	_notification_panel_style.set_border_width_all(2)
	_set_panel_error_style()
	_notification_panel.add_theme_stylebox_override("panel", _notification_panel_style)
	_notification_panel.anchor_left = 1.0
	_notification_panel.anchor_right = 1.0
	_notification_panel.anchor_top = 0.0
	_notification_panel.anchor_bottom = 0.0
	_notification_panel.offset_left = -300.0
	_notification_panel.offset_right = -12.0
	_notification_panel.offset_top = 12.0
	_notification_panel.offset_bottom = 115.0
	_notification_panel.visible = false
	root_control.add_child(_notification_panel)

	var margin := MarginContainer.new()
	margin.set_anchors_preset(Control.PRESET_FULL_RECT)
	margin.add_theme_constant_override("margin_left", 12)
	margin.add_theme_constant_override("margin_right", 12)
	margin.add_theme_constant_override("margin_top", 10)
	margin.add_theme_constant_override("margin_bottom", 10)
	_notification_panel.add_child(margin)

	var vbox := VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 8)
	margin.add_child(vbox)

	_notification_label = Label.new()
	_notification_label.text = "CONNECTION DISRUPTED"
	_notification_label.add_theme_color_override("font_color", Color(1.0, 0.85, 0.85, 1.0))
	_notification_label.add_theme_color_override("font_shadow_color", Color(0.0, 0.0, 0.0, 0.8))
	_notification_label.add_theme_constant_override("shadow_offset_x", 1)
	_notification_label.add_theme_constant_override("shadow_offset_y", 1)
	_notification_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_notification_label.add_theme_font_size_override("font_size", 15)
	vbox.add_child(_notification_label)

	_reconnect_btn = Button.new()
	_reconnect_btn.text = "Reconnect"
	var btn_style := StyleBoxFlat.new()
	btn_style.bg_color = Color(0.85, 0.15, 0.15, 1.0)
	btn_style.set_corner_radius_all(3)
	btn_style.set_content_margin_all(6.0)
	btn_style.border_color = Color(1.0, 0.5, 0.5, 1.0)
	btn_style.set_border_width_all(1)
	var btn_style_hover := btn_style.duplicate() as StyleBoxFlat
	btn_style_hover.bg_color = Color(1.0, 0.2, 0.2, 1.0)
	_reconnect_btn.add_theme_stylebox_override("normal", btn_style)
	_reconnect_btn.add_theme_stylebox_override("hover", btn_style_hover)
	_reconnect_btn.add_theme_stylebox_override("pressed", btn_style)
	_reconnect_btn.add_theme_stylebox_override("disabled", btn_style)
	_reconnect_btn.add_theme_color_override("font_color", Color.WHITE)
	_reconnect_btn.add_theme_color_override("font_hover_color", Color.WHITE)
	_reconnect_btn.add_theme_color_override("font_pressed_color", Color.WHITE)
	_reconnect_btn.add_theme_color_override("font_disabled_color", Color(1.0, 1.0, 1.0, 0.5))
	_reconnect_btn.focus_mode = Control.FOCUS_NONE
	_reconnect_btn.pressed.connect(_on_reconnect_pressed)
	vbox.add_child(_reconnect_btn)

func _unhandled_input(event: InputEvent) -> void:
	if not _notification_panel.visible or _reconnect_btn.disabled:
		return
	if event is InputEventKey and event.pressed and not event.echo:
		if event.keycode == KEY_ENTER or event.keycode == KEY_KP_ENTER:
			get_viewport().set_input_as_handled()
			_on_reconnect_pressed()

func _set_panel_error_style() -> void:
	_notification_panel_style.bg_color = Color(0.45, 0.04, 0.04, 1.0)
	_notification_panel_style.border_color = Color(0.95, 0.25, 0.25, 1.0)

func _set_panel_success_style() -> void:
	_notification_panel_style.bg_color = Color(0.07, 0.38, 0.1, 1.0)
	_notification_panel_style.border_color = Color(0.3, 0.85, 0.35, 1.0)

func _show_disconnect_notification() -> void:
	_set_panel_error_style()
	_notification_label.text = "CONNECTION DISRUPTED"
	_notification_label.add_theme_color_override("font_color", Color(1.0, 0.85, 0.85, 1.0))
	_reconnect_btn.visible = true
	_reconnect_btn.disabled = false
	_notification_panel.visible = true

func _on_connection_restored() -> void:
	if not _notification_panel.visible:
		return
	_set_panel_success_style()
	_notification_label.text = "CONNECTED"
	_notification_label.add_theme_color_override("font_color", Color(0.85, 1.0, 0.87, 1.0))
	_reconnect_btn.visible = false
	await get_tree().create_timer(2.5).timeout
	_notification_panel.visible = false
	_set_panel_error_style()

func _on_reconnect_pressed() -> void:
	_notification_label.text = "Reconnecting..."
	_notification_label.add_theme_color_override("font_color", Color(1.0, 0.85, 0.85, 1.0))
	_reconnect_btn.disabled = true
	if _thread and _thread.is_started():
		_thread.wait_to_finish()
	startup()

# ========================= Public API =========================

## Queues a 200-byte TCP packet to be sent to the server.
## Returns -1 if the packet is not exactly PACKET_SIZE bytes.
func send_tcp(packet: PackedByteArray) -> int:
	if packet.size() != PACKET_SIZE:
		push_error("send_tcp(): Packet size mismatch: %d (expected %d)" % [packet.size(), PACKET_SIZE])
		return -1
	_mutex_out_tcp.lock()
	_out_tcp.append(packet)
	_mutex_out_tcp.unlock()
	return 0

## Pops the next received TCP packet. Returns empty PackedByteArray if none available.
func receive_tcp() -> PackedByteArray:
	_mutex_in_tcp.lock()
	if _in_tcp.is_empty():
		_mutex_in_tcp.unlock()
		return PackedByteArray()
	var pkt := _in_tcp[0]
	_in_tcp.remove_at(0)
	_mutex_in_tcp.unlock()
	return pkt

## Queues a UDPPacket (payload + port) to be sent to server_ip:port.
## Returns -1 if the payload is not exactly PACKET_SIZE bytes.
func send_udp(udp_packet: UDPPacket) -> int:
	if udp_packet.payload.size() != PACKET_SIZE:
		push_error("send_udp(): Payload size mismatch: %d (expected %d)" % [udp_packet.payload.size(), PACKET_SIZE])
		return -1
	_mutex_out_udp.lock()
	_out_udp.append(udp_packet)
	_mutex_out_udp.unlock()
	return 0

## Pops the next received UDP packet. Returns empty PackedByteArray if none available.
func receive_udp() -> PackedByteArray:
	_mutex_in_udp.lock()
	if _in_udp.is_empty():
		_mutex_in_udp.unlock()
		return PackedByteArray()
	var pkt := _in_udp[0]
	_in_udp.remove_at(0)
	_mutex_in_udp.unlock()
	return pkt
