// Fill out your copyright notice in the Description page of Project Settings.


#include "CXSceneDefinition.h"
#include "CXGameScene.h"

#if WITH_EDITOR
#include <Misc/DataValidation.h>

EDataValidationResult UCXSceneDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (!SceneClass)
	{
		Context.AddError(FText::FromString(TEXT("SceneClass가 지정되지 않았습니다.")));
		Result = EDataValidationResult::Invalid;
	}

	if (Sublevels.IsEmpty())
	{
		Context.AddError(FText::FromString(TEXT("Sublevels가 비어 있습니다.")));
		Result = EDataValidationResult::Invalid;
	}

	for (const UCXSceneDefinition* Preload : PreloadScenes)
	{
		if (Preload == this)
		{
			Context.AddError(FText::FromString(TEXT("PreloadScenes에 자기 자신이 들어 있습니다.")));
			Result = EDataValidationResult::Invalid;
		}
		else if (Preload && Preload->ExitPolicy == ECXSceneExitPolicy::Unload)
		{
			Context.AddWarning(FText::FromString(FString::Printf(
				TEXT("'%s'는 Exit 정책이 Unload라서, 한 번 진입했다가 나오면 Preload 효과가 사라집니다."), *Preload->GetName())));
		}
	}

	return Result;
}
#endif
