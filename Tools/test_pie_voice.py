"""Editor integration smoke test; use -ExecutePythonScript=<absolute path>.

Launch with an INI command-line override for the unexposed editor net-mode enum:
-ini:EditorPerProjectUserSettings:[/Script/UnrealEd.LevelEditorPlaySettings]:PlayNetMode=PIE_ListenServer
Uses temporary in-memory play settings; never saves maps or user settings.
The external runner must inspect [PIEVoiceTest] and [Voice] in the log.
"""
import time
import unreal

separate_processes = "-VoiceTestSeparateProcesses" in unreal.SystemLibrary.get_command_line()
settings = unreal.get_default_object(unreal.load_class(None, "/Script/UnrealEd.LevelEditorPlaySettings"))
overrides = {
    "PlayNumberOfClients": 4,
    "RunUnderOneProcess": not separate_processes,
    "CreateAudioDeviceForEveryPlayer": True,
    "EnableGameSound": True,
}
if separate_processes:
    overrides["AdditionalLaunchParameters"] = "-nullrhi -unattended -nosplash"
original = {key: settings.get_editor_property(key) for key in overrides}
for key, value in overrides.items():
    settings.set_editor_property(key, value)

unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/Untitled")
level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
started = time.monotonic()
stage = 0
callback = None


def tick(delta):
    global stage, callback
    elapsed = time.monotonic() - started
    if stage == 0 and elapsed > 5:
        unreal.log("[PIEVoiceTest] Starting four-player listen PIE")
        level_editor.editor_request_begin_play()
        stage = 1
    elif stage == 1 and elapsed > 30:
        worlds = unreal.EditorLevelLibrary.get_pie_worlds(False)
        unreal.log("[PIEVoiceTest] Lobby worlds=" + str(len(worlds)))
        assert len(worlds) == (1 if separate_processes else 4), "Unexpected PIE world count"
        for world in worlds:
            pc = unreal.GameplayStatics.get_player_controller(world, 0)
            unreal.SystemLibrary.execute_console_command(world, "LANVoiceStatus", pc)
            if pc.has_authority():
                # Exercise the real lobby transition instead of editor console travel.
                zone_class = unreal.load_class(None, "/Script/Coop_Driving.LevelTransitionZone")
                zones = unreal.GameplayStatics.get_all_actors_of_class(world, zone_class)
                assert zones, "Lobby needs its existing LevelTransitionZone"
                zones[0].set_editor_property("HoldSeconds", 0.2)
                trigger = zones[0].get_component_by_class(unreal.BoxComponent)
                trigger.set_box_extent(unreal.Vector(100000, 100000, 100000), True)
                unreal.log("[PIEVoiceTest] Expanded runtime-only lobby trigger")
        stage = 2
    elif stage == 2 and elapsed > 55:
        worlds = unreal.EditorLevelLibrary.get_pie_worlds(False)
        unreal.log("[PIEVoiceTest] First run worlds=" + str(len(worlds)))
        for world in worlds:
            if "InGameMapTest" not in world.get_path_name():
                unreal.log_error("[PIEVoiceTest] Travel incomplete: " + world.get_path_name())
        for world in worlds:
            pc = unreal.GameplayStatics.get_player_controller(world, 0)
            unreal.SystemLibrary.execute_console_command(world, "LANVoiceStatus", pc)
        level_editor.editor_request_end_play()
        stage = 3
    elif stage == 3 and elapsed > 62:
        # Run a second PIE session to verify the first one's hidden sessions were released.
        unreal.EditorLoadingAndSavingUtils.load_map("/Game/Maps/InGameMapTest")
        unreal.log("[PIEVoiceTest] Starting second PIE run")
        level_editor.editor_request_begin_play()
        stage = 4
    elif stage == 4 and elapsed > 82:
        for world in unreal.EditorLevelLibrary.get_pie_worlds(False):
            unreal.SystemLibrary.execute_console_command(world, "LANVoiceStatus",
                unreal.GameplayStatics.get_player_controller(world, 0))
        # EndPlay persists editor play settings. Restore BEFORE requesting it,
        # otherwise headless child arguments leak into the user's next run.
        for key, value in original.items():
            settings.set_editor_property(key, value)
        level_editor.editor_request_end_play()
        stage = 5
    elif stage == 5 and elapsed > 88:
        for key, value in original.items():
            settings.set_editor_property(key, value)
        unreal.unregister_slate_post_tick_callback(callback)
        unreal.log("[PIEVoiceTest] Completed")
        unreal.SystemLibrary.quit_editor()


callback = unreal.register_slate_post_tick_callback(tick)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
