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
			"ChooserUncooked",
			"ChooserEditor",
			"ProxyTable",
			"ProxyTableUncooked",
			"ProxyTableEditor",
			"AnimationWarpingRuntime",
			"AnimationWarpingEditor",
			"BlendStack",
			"BlendStackEditor",
			"MotionWarping",
			"PoseSearch",
			"PoseSearchEditor",
			"DrawDebugLibrary",
			"CurveExpression",
			"CurveExpressionEditor"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
