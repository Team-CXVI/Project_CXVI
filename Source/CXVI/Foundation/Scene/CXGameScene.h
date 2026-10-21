// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CXSceneTypes.h"
#include "EnhancedInputComponent.h"

#include "CXGameScene.generated.h"

class UCXSceneDefinition;
class UCXSceneSubsystem;
class UInputAction;

/**
 * Scene Object의 베이스 클래스(추상 클래스)
 * Lifecycle : OnEnter -> (OnSuspend <-> OnResume)* -> OnExit
 * 베이스가 IMC, Scene 입력 바인딩, Suspendable 알림, Spawn Actor 정리를 처리하므로 자식은 반드시 Super롤 호출해야 함
 */
UCLASS(Abstract, Blueprintable)
class CXVI_API UCXGameScene : public UObject
{
	GENERATED_BODY()
	
	friend class UCXSceneSubsystem;
	
public:
	virtual UWorld* GetWorld() const override;

	FORCEINLINE const UCXSceneDefinition* GetDefinition() const { return Definition; }

	UCXSceneSubsystem* GetSceneSubsystem() const;

	// 현재 로드된 이 Scene의 레벨들
	TArray<ULevel*> GetSceneLevels() const;

	// Sublevels 중 메인 레벨(배열을 0번 인덱스)
	ULevel* GetPrimaryLevel() const;

	// Scene 레벨에서 T 타입 액터를 검색
	template<typename T>
	void FindActorsIsSceneLevels(TArray<T*>& OutActors) const
	{
		for (const ULevel* Level : GetSceneLevels())
		{
			for (const AActor* Actor : Level->Actors)
			{
				if (T* CastActor = Cast<T>(Actor))
				{
					OutActors.Emplace(CastActor);
				}
			}
		}
	}

	// Lifecycle
protected:
	virtual void OnEnter(const FCXSceneContext& InContext);

	virtual void OnExit();

	virtual void OnSuspend();

	virtual void OnResume(const FCXSceneContext& InContext);

	UFUNCTION(BlueprintImplementableEvent, Category = "Scene", meta = (Displayname = "On Enter"))
	void K2_OnEnter(const FCXSceneContext& InContext);

	UFUNCTION(BlueprintImplementableEvent, Category = "Scene", meta = (Displayname = "On Exit"))
	void K2_OnExit();

	UFUNCTION(BlueprintImplementableEvent, Category = "Scene", meta = (Displayname = "On Suspend"))
	void K2_OnSuspend();

	UFUNCTION(BlueprintImplementableEvent, Category = "Scene", meta = (DisplayName = "On Resume"))
	void K2_OnResume(const FCXSceneContext& InContext);

	// Scene Input
protected:
	virtual void SetupSceneInput() {}

	// Scene이 활성화 될 때(Enter / Resume) 호출 -> 이 타이밍에 BindSceneAction으로 바인딩하면 Suspend / Exit 때 자동 해제
	template<typename UserClass,  typename FuncType>
	void BindSceneAction(const UInputAction* InAction, ETriggerEvent InTrigger, UserClass* InObject, FuncType InFunc)
	{
		if (UEnhancedInputComponent* Input = GetPlayerInputComponent())
		{
			InputBindingHandles.Add(Input->BindAction(InAction, InTrigger, InObject, InFunc).GetHandle());
		}
	}

	// Primary Level에 스폰하고 추적. OnExit에서 자동 Destroy (KeepLoaded 레벨에 남지 않도록)
	template<typename T>
	T* SpawnSceneActor(TSubclassOf<T> InClass, const FTransform& InTransform)
	{
		UWorld* World = GetWorld();
		if (nullptr == World || nullptr == InClass)
		{
			return nullptr;
		}

		FActorSpawnParameters Params;
		Params.OverrideLevel = GetPrimaryLevel();
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		T* Actor = World->SpawnActor<T>(InClass, InTransform, Params);
		if (Actor)
		{
			SpawnedActors.Emplace(Actor);
		}

		return Actor;
	}

private:
	UEnhancedInputComponent* GetPlayerInputComponent() const;

	void ApplyInputMappings() const;

	// Definition에 정의된 IMC들을 등록 해제
	void RemoveInputMappings() const;

	void ClearSceneInput();

	// Persistent Level + 이 Scene 레벨의 ICXSceneSuspendable 액터에 알림
	void NotifySuspendables(bool bInSuspended) const;

	void DestroySpawnedActors();

private:
	UPROPERTY()
	TObjectPtr<UCXSceneDefinition> Definition;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	TArray<uint32> InputBindingHandles;
};
