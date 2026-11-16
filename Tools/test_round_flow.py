"""Four-player, two-round PIE smoke test. Does not save maps or BP assets.

Run a separate editor with -ExecutePythonScript=<this file> and
-ini:EditorPerProjectUserSettings:[/Script/UnrealEd.LevelEditorPlaySettings]:PlayNetMode=PIE_ListenServer
Inspect [RoundFlowTest] in the log; Python quits the test editor on completion.
"""
import time
import unreal

settings = unreal.get_default_object(unreal.load_class(None, "/Script/UnrealEd.LevelEditorPlaySettings"))
overrides = {"PlayNumberOfClients": 4, "RunUnderOneProcess": True, "EnableGameSound": False}
original = {key: settings.get_editor_property(key) for key in overrides}
for key, value in overrides.items():
    settings.set_editor_property(key, value)
unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Untitled")
editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
role_class = unreal.load_class(None, "/Script/Coop_Driving.PlayerRoleComponent")
zone_class = unreal.load_class(None, "/Script/Coop_Driving.LevelTransitionZone")
destination_class = unreal.load_class(None, "/Script/Coop_Driving.DrivingDestinationZone")
vehicle_class = unreal.load_class(None, "/Script/Coop_Driving.CoopVehicle")
character_class = unreal.load_class(None, "/Script/Coop_Driving.PlayerCharacter")
started = time.monotonic()
phase_started = started
phase = "start"
round_index = 0
callback = None


def restore():
    for key, value in original.items():
        settings.set_editor_property(key, value)


def assignments(world):
    state = unreal.GameplayStatics.get_game_state(world)
    if not state:
        return []
    result = []
    for player in state.get_editor_property("PlayerArray"):
        roles = player.get_component_by_class(role_class)
        if not roles:
            return []
        result.append(roles.get_editor_property("Assignment"))
    return result


def all_on_map(worlds, name):
    return len(worlds) == 4 and all(world.get_name().endswith(name) for world in worlds)


def verify_lobby_spawns(worlds):
    for world in worlds:
        characters = unreal.GameplayStatics.get_all_actors_of_class(world, character_class)
        if len(characters) != 4:
            return False
        for index, character in enumerate(characters):
            location = character.get_actor_location()
            radius = character.get_component_by_class(unreal.CapsuleComponent).get_scaled_capsule_radius()
            for other in characters[index + 1:]:
                delta = location - other.get_actor_location()
                distance = (delta.x ** 2 + delta.y ** 2) ** 0.5
                other_radius = other.get_component_by_class(unreal.CapsuleComponent).get_scaled_capsule_radius()
                assert distance >= radius + other_radius + 10, "Lobby capsules overlap or lack clearance"
                assert distance <= 805, "Lobby players spawned too far apart"
    return True


def tick(delta):
    global phase, phase_started, round_index, callback
    try:
        now = time.monotonic()
        if phase == "shutdown":
            if now - phase_started > 3:
                restore()
                unreal.unregister_slate_post_tick_callback(callback)
                unreal.SystemLibrary.quit_editor()
            return
        if now - phase_started > 80:
            raise AssertionError("Timeout in phase " + phase)
        worlds = unreal.EditorLevelLibrary.get_pie_worlds(False)
        if phase == "start" and now - started > 3:
            editor.editor_request_begin_play()
            phase = "lobby"
            phase_started = now
        elif phase == "lobby" and all_on_map(worlds, "Untitled"):
            data = [assignments(world) for world in worlds]
            if any(len(items) != 4 for items in data):
                return
            assert all(item.role == unreal.DrivingRole.NONE and not item.disability_ids
                       for items in data for item in items), "Lobby assignment did not reset"
            if now - phase_started < 3:
                return
            if not verify_lobby_spawns(worlds):
                return
            for world in worlds:
                pc = unreal.GameplayStatics.get_player_controller(world, 0)
                unreal.SystemLibrary.execute_console_command(world, "LANVoiceStatus", pc)
                if pc.has_authority():
                    mode = unreal.GameplayStatics.get_game_mode(world)
                    # Exercise a different hand-authored map in the second round.
                    path = "/Game/Maps/InGameMapTest" if round_index == 0 else "/Game/Maps/TestVehicle"
                    mode.set_driving_map(unreal.load_asset(path))
                    zones = unreal.GameplayStatics.get_all_actors_of_class(world, zone_class)
                    assert zones, "Missing lobby ready zone"
                    zones[0].set_editor_property("HoldSeconds", 0.2)
                    zones[0].get_component_by_class(unreal.BoxComponent).set_box_extent(unreal.Vector(100000, 100000, 100000), True)
            unreal.log("[RoundFlowTest] Lobby reset verified; starting round " + str(round_index + 1))
            phase = "driving"
            phase_started = now
        elif phase == "driving" and all_on_map(worlds, "InGameMapTest" if round_index == 0 else "TestVehicle"):
            data = [assignments(world) for world in worlds]
            if any(len(items) != 4 or len({item.role for item in items}) != 4
                   or any(item.role == unreal.DrivingRole.NONE for item in items) for items in data):
                return
            for world in worlds:
                pc = unreal.GameplayStatics.get_player_controller(world, 0)
                if pc.has_authority():
                    vehicles = unreal.GameplayStatics.get_all_actors_of_class(world, vehicle_class)
                    assert len(vehicles) == 1, "Expected one shared vehicle"
                    vehicle = vehicles[0]
                    unreal.SystemLibrary.execute_console_command(world, "EnableCheats", pc)
                    unreal.SystemLibrary.execute_console_command(world, "Summon /Script/Coop_Driving.DrivingDestinationZone", pc)
                    destinations = unreal.GameplayStatics.get_all_actors_of_class(world, destination_class)
                    assert destinations, "Could not spawn runtime test destination"
                    destinations[-1].set_actor_location(vehicle.get_actor_location(), False, True)
            unreal.log("[RoundFlowTest] Four unique roles replicated; destination spawned for round " + str(round_index + 1))
            round_index += 1
            phase = "returned" if round_index == 2 else "lobby"
            phase_started = now
        elif phase == "returned" and all_on_map(worlds, "Untitled"):
            data = [assignments(world) for world in worlds]
            if any(len(items) != 4 for items in data):
                return
            if not verify_lobby_spawns(worlds):
                return
            assert all(item.role == unreal.DrivingRole.NONE and not item.disability_ids
                       for items in data for item in items), "Second return did not reset"
            for world in worlds:
                unreal.SystemLibrary.execute_console_command(world, "LANVoiceStatus",
                    unreal.GameplayStatics.get_player_controller(world, 0))
            unreal.log("[RoundFlowTest] SUCCESS: two different driving maps completed; all four clients returned with no role/disabilities")
            restore()
            editor.editor_request_end_play()
            phase = "shutdown"
            phase_started = now
    except Exception as error:
        unreal.log_error("[RoundFlowTest] FAILED: " + str(error))
        restore()
        editor.editor_request_end_play()
        phase = "shutdown"
        phase_started = time.monotonic()


callback = unreal.register_slate_post_tick_callback(tick)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
