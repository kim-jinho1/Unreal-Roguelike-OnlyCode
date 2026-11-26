using UnrealBuildTool;

public class Unreal_Roguelike : ModuleRules
{
	public Unreal_Roguelike(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[]
		{
			"Unreal_Roguelike"
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags",
			"UMG",
			"Slate",
			"SlateCore",
			"AIModule",
			"NavigationSystem",
			"GameplayTasks"
		});

		// 사격 카메라 흔들림(URGWeaponCameraShake.cpp)에서만 쓰고 헤더에는 드러나지 않는다
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"EngineCameras"
		});
	}
}
