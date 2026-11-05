@tool
extends Node3D

@export var animate := true
@export var wave := true
@export var walking := false
@export_range(0.2, 3.0, 0.05) var animation_speed := 1.0

var _time := 0.0
var _visual: Node3D
var _left_wing: Node3D
var _right_wing: Node3D
var _left_foot: MeshInstance3D
var _right_foot: MeshInstance3D
var _left_toes: Array[MeshInstance3D] = []
var _right_toes: Array[MeshInstance3D] = []
var _left_foot_y := 0.0
var _right_foot_y := 0.0

func _ready() -> void:
	_visual = get_node("Visual")
	_left_wing = get_node("Visual/LeftWingPivot")
	_right_wing = get_node("Visual/RightWingPivot")
	_left_foot = get_node("Visual/LeftFoot")
	_right_foot = get_node("Visual/RightFoot")
	_left_toes = [get_node("Visual/LeftToeA"), get_node("Visual/LeftToeB")]
	_right_toes = [get_node("Visual/RightToeA"), get_node("Visual/RightToeB")]
	_left_foot_y = _left_foot.position.y
	_right_foot_y = _right_foot.position.y

func _process(delta: float) -> void:
	if not animate or not _visual:
		return
	_time += delta * animation_speed
	_visual.position.y = abs(sin(_time * 7.0)) * 0.055 if walking else sin(_time * 2.0) * 0.025
	_visual.rotation.z = sin(_time * 1.35) * 0.018
	_left_wing.rotation.z = -0.16 + sin(_time * 1.7) * 0.055
	_right_wing.rotation.z = 0.16
	_right_wing.rotation.x = -0.08
	if wave:
		_right_wing.rotation.z = 2.72 + sin(_time * 3.1) * 0.16
	var step := sin(_time * 7.0) if walking else 0.0
	_left_foot.position.y = _left_foot_y + maxf(step, 0.0) * 0.09
	_right_foot.position.y = _right_foot_y + maxf(-step, 0.0) * 0.09
	for toe in _left_toes:
		toe.position.y = 0.23 + maxf(step, 0.0) * 0.09
	for toe in _right_toes:
		toe.position.y = 0.23 + maxf(-step, 0.0) * 0.09

func set_wave(enabled: bool) -> void:
	wave = enabled

func set_walking(enabled: bool) -> void:
	walking = enabled

