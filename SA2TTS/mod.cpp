#include "pch.h"
#include "SA2ModLoader.h"
#include "UsercallFunctionHandler.h"
#include "IniFile.hpp"
#include <sapi.h>
#include <sphelper.h>
#include <atlbase.h>
#include <vector>

CComPtr<ISpVoice> pVoice = nullptr;
CComPtr<ISpObjectToken> voicetoken = nullptr;
std::vector<wchar_t> textbuf;
UsercallFuncVoid(ProcessSubtitles, (char* message, int audiobasenum, int* voicenum, char** start, int* wait, char** end), (message, audiobasenum, voicenum, start, wait, end), 0x6B6CA0, rEAX, rEBX, rESI, stack4, stack4, stack4);
void ProcessSubtitles_r(char* message, int audiobasenum, int* voicenum, char** start, int* wait, char** end)
{
	ProcessSubtitles.Original(message, audiobasenum, voicenum, start, wait, end);
	if (voicenum)
		*voicenum = -1;
	size_t len;
	if (*end)
		len = *end - *start;
	else
		len = strlen(*start);
	int cp = TextLanguage ? 1252 : 932;
	auto len2 = MultiByteToWideChar(cp, 0, *start, len, nullptr, 0);
	textbuf.resize(len2 + 1, 0);
	MultiByteToWideChar(cp, 0, *start, len, textbuf.data(), textbuf.size());
	textbuf[len2] = 0;
	for (wchar_t& ch : textbuf)
		if (ch == '\n')
			ch = ' ';
	pVoice->Speak(textbuf.data(), SPF_ASYNC | SPF_PURGEBEFORESPEAK | SPF_IS_NOT_XML, nullptr);
}

extern "C"
{
	__declspec(dllexport) void Init(const char* path, const HelperFunctions& helperFunctions)
	{
		if (SUCCEEDED(pVoice.CoCreateInstance(CLSID_SpVoice)))
		{
			char pathbuf[MAX_PATH];
			sprintf_s(pathbuf, "%s\\config.ini", path);
			IniFile config(pathbuf);
			const auto group = config.getGroup("Voice");
			if (group->cbegin() != group->cend())
			{
				std::wstring attribs;
				if (group->hasKeyNonEmpty("Name"))
					attribs += L"Name=" + group->getWString("Name");
				if (group->hasKeyNonEmpty("Vendor"))
				{
					if (!attribs.empty())
						attribs += L';';
					attribs += L"Vendor=" + group->getWString("Vendor");
				}
				if (group->hasKeyNonEmpty("Language"))
				{
					if (!attribs.empty())
						attribs += L';';
					attribs += L"Language=" + group->getWString("Language");
				}
				if (group->hasKeyNonEmpty("Age"))
				{
					if (!attribs.empty())
						attribs += L';';
					attribs += L"Age=" + group->getWString("Age");
				}
				if (group->hasKeyNonEmpty("Gender"))
				{
					if (!attribs.empty())
						attribs += L';';
					attribs += L"Gender=" + group->getWString("Gender");
				}
				if (SUCCEEDED(SpFindBestToken(SPCAT_VOICES, attribs.c_str(), nullptr, &voicetoken)))
					pVoice->SetVoice(voicetoken);
				else
					PrintDebug("SA2 TTS: Failed to find voice matching criteria!");
			}
			ProcessSubtitles.Hook(ProcessSubtitles_r);
			WriteData((char*)0x602FF0, (char)0xC3); // disable cutscene voice playback
			WriteData((char*)0x457D20, (char)0xC3); // disable mini-cutscene voice playback
		}
		else
			PrintDebug("SA2 TTS: Failed to initialize text-to-speech engine!");
	}

	__declspec(dllexport) ModInfo SA2ModInfo = { ModLoaderVer };
}