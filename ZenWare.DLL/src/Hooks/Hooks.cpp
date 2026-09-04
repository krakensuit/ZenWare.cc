#include "Hooks.h"

#include "../Util/Logger/Logger.h"

using namespace Hooks;

#define LOG_HOOK_STAGE(name) U::Log.Write("    hooks: %-22s ...", name)
#define LOG_HOOK_OK(name)     U::Log.Write("    hooks: %-22s ok", name)

void CGlobal_Hooks::Init()
{
	const MH_STATUS MH_INIT_STATUS = MH_Initialize();
	XASSERT(MH_INIT_STATUS != MH_STATUS::MH_OK);

	if (MH_INIT_STATUS == MH_STATUS::MH_OK)
	{
		U::Log.Write("[+] MinHook initialized.");

		LOG_HOOK_STAGE("BaseClient");          BaseClient::Init(); LOG_HOOK_OK("BaseClient");
		LOG_HOOK_STAGE("BasePlayer");          BasePlayer::Init(); LOG_HOOK_OK("BasePlayer");
		LOG_HOOK_STAGE("CL_Main");             CL_Main::Init();
		LOG_HOOK_STAGE("ClientMode");          ClientMode::Init(); LOG_HOOK_OK("ClientMode");
		LOG_HOOK_STAGE("ClientPrediction");    ClientPrediction::Init();
		LOG_HOOK_STAGE("EngineVGui");          EngineVGui::Init(); LOG_HOOK_OK("EngineVGui");
		LOG_HOOK_STAGE("ModelRender");         ModelRender::Init();
		LOG_HOOK_STAGE("ModelRenderSystem");   ModelRenderSystem::Init();

		//Disabled: the CheckForSequenceChange pattern matches TWO functions on
		//build 23990068 (02.07.2026). Hooking the wrong one crashes animations.
		//Re-enable only after confirming the correct candidate in IDA/x64dbg.
		//SequenceTransitioner::Init();

		LOG_HOOK_STAGE("TerrorGameRules");     TerrorGameRules::Init();
		LOG_HOOK_STAGE("TerrorPlayer");        TerrorPlayer::Init();
		LOG_HOOK_STAGE("WndProc");             WndProc::Init();
	}
	else
	{
		U::Log.Write("[!] MH_Initialize failed (%d). No hooks installed.", static_cast<int>(MH_INIT_STATUS));
	}

	const MH_STATUS MH_ENABLE_STATUS = MH_EnableHook(MH_ALL_HOOKS);
	XASSERT(MH_ENABLE_STATUS != MH_STATUS::MH_OK);
	U::Log.Write((MH_ENABLE_STATUS == MH_STATUS::MH_OK) ? "[+] All hooks enabled." : "[!] MH_EnableHook failed (%d).", static_cast<int>(MH_ENABLE_STATUS));
}

