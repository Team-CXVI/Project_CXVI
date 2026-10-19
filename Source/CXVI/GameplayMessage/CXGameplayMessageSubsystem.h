// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"

#include "CXGameplayMessageSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGameplayMessageSubsystem, Log, All);

/**
 * 
 */
USTRUCT(BlueprintType)
struct CXVI_API FCXGameplayMessageListenerHandle
{
	GENERATED_BODY()

	friend UCXGameplayMessageSubsystem;

public:
	FCXGameplayMessageListenerHandle() = default;

	FCXGameplayMessageListenerHandle(UCXGameplayMessageSubsystem* InSubsystem, const FGameplayTag& InChannel, int32 InId)
		: SubsystemRef(InSubsystem), Channel(InChannel), Id(InId)
	{

	}

	void Unregister();

	bool IsValid() const { return Id != 0; }

private:

	UPROPERTY()
	TWeakObjectPtr<UCXGameplayMessageSubsystem> SubsystemRef;

	UPROPERTY()
	FGameplayTag Channel;

	UPROPERTY()
	int32 Id = 0;

	FDelegateHandle StateClearedHandle;

};

USTRUCT()
struct FCXGameplayMessageListenerData
{
	GENERATED_BODY()

	TFunction<void(const FGameplayTag&, const UScriptStruct*, const void*)> ReceivedCallback;

	UPROPERTY()
	TWeakObjectPtr<const UScriptStruct> ListenerStructType;

	int32 HandleId;

	bool bHadValidType = false;
};

UCLASS()
class CXVI_API UCXGameplayMessageSubsystem : public UGameInstanceSubsystem
{
	friend class UCXAsyncAction_ListenForGameplayMessage;

	GENERATED_BODY()

public:
	static UCXGameplayMessageSubsystem& Get(const UObject* WorldContextObject);

	static bool HasInstance(const UObject* WorldContextObject);

	virtual void Deinitialize() override;

	void UnregisterListener(const FCXGameplayMessageListenerHandle& InHandle);

public:
	template <typename FMessageStructType, typename TOwner = UObject>
	FCXGameplayMessageListenerHandle RegisterListener(const FGameplayTag InChannel, TFunction<void(const FGameplayTag&, const FMessageStructType&)>&& InCallback)
	{
		auto ThunkCallbak = [InnerCallback = MoveTemp( InCallback )](const FGameplayTag& ActualTag, const UScriptStruct* SenderStructType, const void* SenderPayload)
			{
				InnerCallback(ActualTag, *reinterpret_cast<const FMessageStructType*>(SenderPayload));
			};
		
		const UScriptStruct* StructType = TBaseStructure<FMessageStructType>::Get();
		return RegisterListenerInternal(InChannel, ThunkCallbak, StructType);
	}

	template<typename FMessageStructType, typename TOwner = UObject>
	FCXGameplayMessageListenerHandle RegisterListener(const FGameplayTag& InChannel, TOwner* InObject, void(TOwner::* InFunction)(const FGameplayTag&, const FMessageStructType&))
	{
		TWeakObjectPtr<TOwner> WeakObject(InObject);
		return RegisterListener<FMessageStructType>(InChannel,
			[WeakObject, InFunction](FGameplayTag Channel, const FMessageStructType& Payload)
			{
				if (TOwner* Object = WeakObject.Get())
				{
					(Object->*InFunction)(Channel, Payload);
				}
			});
	}

	template<typename FMessageStructType>
	void BroadcastMessage(const FGameplayTag& InChannel, const FMessageStructType& InMessage)
	{
		const UScriptStruct* StructType = TBaseStructure<FMessageStructType>::Get();
		BroadcastMessageInternal(InChannel, StructType, &InMessage);
	}

protected:
	// CustomThunk -> Thunk 함수를 UHT에게 맡기는 것이 아닌 직접 생성하겠다는 선언
	// 지금 같은 경우는 InMessage를 와일드카드 핀(어떤 타입이든 받는 핀 -> 메시지 구조체)으로 사용하기 위함
	UFUNCTION(BlueprintCallable, CustomThunk, Category = Messaging, meta = (CustomStructureParam = "Message", AllowAbstract = "false", DisplayName = "Broadcast Message"))
	void K2_BroadcastMessage(const FGameplayTag& InChannel, const int32& InMessage);

	// BP쪽에서 C++함수를 호출할 때 쓰는 Thunk(썽크) 함수를 직접 처리하는 정의부
	// CustomThunk를 선언하지 않으면 자동으로 .generated.h, .gen.cpp에 만들어짐
	DECLARE_FUNCTION(execK2_BroadcastMessage);

private:
	FCXGameplayMessageListenerHandle RegisterListenerInternal(
		const FGameplayTag& InChannel, TFunction<void(const FGameplayTag&, const UScriptStruct*, const void*)>&& InCallback, const UScriptStruct* InStructType);

	void BroadcastMessageInternal(const FGameplayTag& InChannel, const UScriptStruct* InStructType, const void* InMessageBytes);

	void UnregisterListenerInternal(const FGameplayTag& InChannel, int32 InHandleId);


private:

	struct FCXChannelListenerList
	{
		TArray<FCXGameplayMessageListenerData> Listeners = {};
		int32 HandleId = 0;
	};

	TMap<FGameplayTag, FCXChannelListenerList> ListenerMap;
};
