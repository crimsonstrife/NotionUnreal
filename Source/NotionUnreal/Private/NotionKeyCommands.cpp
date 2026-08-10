// Copyright (c) 2023 kaleidoscube GmbH, Germany. All rights reserved.
#include "NotionKeyCommands.h"



bool UNotionKeyCommands::IsBindableKey(const FKey& Key)
{
    return Key.IsValid() && Key != EKeys::AnyKey && !Key.IsGamepadKey() && !Key.IsModifierKey() && !Key.IsMouseButton()
        && !Key.IsAxis1D() && !Key.IsAxis2D() && !Key.IsAxis3D();
}


namespace
{
    // FKeyBind has no operator==, so compare the fields Notion actually manages.
    bool KeyBindsMatch(const FKeyBind& A, const FKeyBind& B)
    {
        return A.Key == B.Key
            && A.Command.Equals(B.Command, ESearchCase::IgnoreCase)
            && A.Control == B.Control
            && A.Shift == B.Shift
            && A.Alt == B.Alt
            && A.Cmd == B.Cmd
            && A.bIgnoreCtrl == B.bIgnoreCtrl
            && A.bIgnoreShift == B.bIgnoreShift
            && A.bIgnoreAlt == B.bIgnoreAlt
            && A.bIgnoreCmd == B.bIgnoreCmd;
    }
}


bool UNotionKeyCommands::UpdatePlayerInput(UPlayerInput* PlayerInputRef, const FKeyBind& NewKeyBind)
{
    const int32 Index = PlayerInputRef->DebugExecBindings.IndexOfByPredicate([&](const FKeyBind& PlayerKeyBind)
        {
            return PlayerKeyBind.Command.Equals(NewKeyBind.Command, ESearchCase::IgnoreCase);
        });

    if (IsBindableKey(NewKeyBind.Key))
    {
        if (Index != INDEX_NONE)
        {
            // Already bound identically -> nothing to change.
            if (KeyBindsMatch(PlayerInputRef->DebugExecBindings[Index], NewKeyBind))
            {
                return false;
            }
            PlayerInputRef->DebugExecBindings[Index] = NewKeyBind;
        }
        else
        {
            PlayerInputRef->DebugExecBindings.Add(NewKeyBind);
        }
        return true;
    }
    else
    {
        if (Index != INDEX_NONE)
        {
            PlayerInputRef->DebugExecBindings.RemoveAt(Index);
            return true;
        }
        return false;
    }
}


FKeyBind UNotionKeyCommands::CreateUnrealKeyBinding(const FNotionKeyInfo& KeyInfo, const FString& Command)
{
    FKeyBind KeyBind;
    KeyBind.Command = Command;
    KeyBind.Key = KeyInfo.Key;
    KeyBind.bDisabled = false;

#define FILL_MODIFIER_DATA(KeyInfoProperty, BindProperty, BindIgnoreProperty)\
			if (KeyInfo.KeyInfoProperty == ECheckBoxState::Undetermined)\
			{\
				KeyBind.BindProperty = KeyBind.BindIgnoreProperty = false;\
			}\
			else\
			{\
				KeyBind.BindProperty = (KeyInfo.KeyInfoProperty == ECheckBoxState::Checked);\
				KeyBind.BindIgnoreProperty = !KeyBind.BindProperty;\
			}

    FILL_MODIFIER_DATA(Shift, Shift, bIgnoreShift);
    FILL_MODIFIER_DATA(Ctrl, Control, bIgnoreCtrl);
    FILL_MODIFIER_DATA(Alt, Alt, bIgnoreAlt);
    FILL_MODIFIER_DATA(Cmd, Cmd, bIgnoreCmd);

#undef FILL_MODIFIER_DATA

    return KeyBind;
}


void UNotionKeyCommands::SetKeyToCommand(const FNotionKeyInfo& KeyInfo, const TCHAR* CommandName)
{
    const FString& Command = CommandName;

    checkf(!Command.IsEmpty(), TEXT("Command is empty."));

    const FKeyBind KeyBind = CreateUnrealKeyBinding(KeyInfo, Command);

    if (UPlayerInput* CopyOfPlayerInput = GetMutableDefault<UPlayerInput>())
    {
        // Merge only Notion's binding (keyed by command) rather than clobbering the
        // whole array, so any other DebugExecBindings (and InvertedAxis entries) survive.
        const bool bChanged = UpdatePlayerInput(CopyOfPlayerInput, KeyBind);

        // Only touch DefaultInput.ini when the binding actually changed, to avoid
        // needless rewrites / VCS churn on every editor start.
        if (bChanged)
        {
            // NOTE: Do NOT use SaveConfig(CPF_Config, GetDefaultConfigFilename()) here.
            // That writes a file containing only the UPlayerInput section, wiping every
            // other section that shares Config/DefaultInput.ini (EnhancedInputDeveloperSettings'
            // bEnableUserSettings, EnhancedInputEditorProjectSettings' DefaultEditorInputClass /
            // DefaultMappingContexts, etc.). TryUpdateDefaultConfigFile() merges ONLY this
            // object's section (via FConfigFile::UpdateSections), preserving the rest.
            CopyOfPlayerInput->TryUpdateDefaultConfigFile();
        }
    }

}







