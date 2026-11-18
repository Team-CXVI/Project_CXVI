// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/CancellableAsyncAction.h"
#include "CXGameplayMessageSubsystem.h"

#include "CXAsyncAction_ListenForMessage.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAsyncGameplayMessageDelegate, UCXAsyncAction_ListenForGameplayMessage*, ProxyObject, const FGameplayTag&, ActualChannel);

/**
 * 
 */
UCLASS(BlueprintType)
class CXVI_API UCXAsyncAction_ListenForGameplayMessage : public UCancellableAsyncAction
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintCallable, Category = Messaging, meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true"))
	static UCXAsyncAction_ListenForGameplayMessage* ListenForGameplayMessages(UObject* WorldContextObject, const FGameplayTag& InChannel, UScriptStruct* InPayloadType);

	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Messaging", meta = (CustomStructureParam = "OutPayload"))
	bool GetPayload(UPARAM(ref) int32& OutPayload);

	DECLARE_FUNCTION(execGetPayload);


	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

private:
	void HandleMessageReceived(const FGameplayTag InChannel, const UScriptStruct* InStructType, const void* InPayload);

public:
	UPROPERTY(BlueprintAssignable)
	FAsyncGameplayMessageDelegate OnMessageReceived;

private:
	
	const void* ReceivedMessagePayloadPtr = nullptr;

	TWeakObjectPtr<UWorld> WorldPtr;
	FGameplayTag ChannelToRegister;
	TWeakObjectPtr<UScriptStruct> MessageStructType = nullptr;
	FCXGameplayMessageListenerHandle ListenerHandle;
	
};
