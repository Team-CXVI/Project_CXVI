// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using UnrealBuildTool.Rules;

public class CXVI : ModuleRules
{
	public CXVI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"CommonUI",
			"CommonInput",
			"DeveloperSettings",

			// Field 기능을 위한 모듈
			"Chooser",
			"ProxyTable",
			"AnimationWarpingRuntime",
			"BlendStack",
			"MotionWarping",
			"PoseSearch",
			"DrawDebugLibrary",
			"CurveExpression",
		});

		// Field 기능 에디터 전용 모듈 (패키징 시 제외)
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
			"ChooserUncooked",
			"ChooserEditor",
			"ProxyTableUncooked",
			"ProxyTableEditor",
			"AnimationWarpingEditor",
			"BlendStackEditor",
			"PoseSearchEditor",
			"CurveExpressionEditor" });
		}

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
