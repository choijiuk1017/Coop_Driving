"""Run once with UnrealEditor-Cmd -run=pythonscript -script=... ."""
import unreal

menu_class = unreal.load_class(None, "/Script/Coop_Driving.LANMenuGameMode")
assert menu_class, "Native LAN menu class was not compiled"
path = "/Game/Maps/MenuMap"
if not unreal.EditorAssetLibrary.does_asset_exist(path):
    assert unreal.EditorLevelLibrary.new_level(path), "Could not create MenuMap"
else:
    unreal.EditorLoadingAndSavingUtils.load_map(path)
world = unreal.EditorLevelLibrary.get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", menu_class)
assert unreal.EditorLevelLibrary.save_current_level(), "Could not save MenuMap"

for bp_path in ["/Game/Blueprint/BP_LobbyGameMode", "/Game/Blueprint/BP_DrivingGameMode"]:
    bp = unreal.load_asset(bp_path)
    assert bp, bp_path
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_loaded_asset(bp)
    unreal.log("[LANSetup] Validated " + bp_path)
    if bp_path.endswith("BP_LobbyGameMode"):
        cls = unreal.EditorAssetLibrary.load_blueprint_class(bp_path)
        destination = unreal.get_default_object(cls).get_editor_property("next_level_path")
        assert unreal.EditorAssetLibrary.does_asset_exist(destination), destination
        unreal.log("[LANSetup] Lobby destination: " + destination)
unreal.log("[LANSetup] MenuMap saved with native LANMenuGameMode")
