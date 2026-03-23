#pragma once
#include "stdafx.h"
#include "WaveFile.h"   // Self-contained ATG::WaveFile replacement (no XDK ATG samples needed)

//============================================================================================
// VoiceCallback — frees WAV buffer when voice finishes playing
// Matches the pattern from the working XDK sample exactly.
//============================================================================================
class VoiceCallback : public IXAudio2VoiceCallback {
public:
    BYTE* pBuffer;
    VoiceCallback(BYTE* b) : pBuffer(b) {}

    STDMETHOD_(void, OnBufferEnd)(void* pBufferContext) {
        delete[] pBuffer;
        pBuffer = NULL;
    }
    // Required interface stubs
    STDMETHOD_(void, OnVoiceProcessingPassStart)(UINT32) {}
    STDMETHOD_(void, OnVoiceProcessingPassEnd)()         {}
    STDMETHOD_(void, OnStreamEnd)()                      {}
    STDMETHOD_(void, OnBufferStart)(void*)               {}
    STDMETHOD_(void, OnLoopEnd)(void*)                   {}
    STDMETHOD_(void, OnVoiceError)(void*, HRESULT)       {}
};

//============================================================================================
// AudioEntry — fixed-size name + file path, no std::string
//============================================================================================
#define AUDIO_MAX_MUSIC   8
#define AUDIO_MAX_SFX     8
#define AUDIO_NAME_LEN   32
#define AUDIO_PATH_LEN  128

struct AudioEntry {
    char name[AUDIO_NAME_LEN];
    char path[AUDIO_PATH_LEN];
    bool loaded;
    AudioEntry() { name[0] = '\0'; path[0] = '\0'; loaded = false; }
};

//============================================================================================
// AudioManager
// Uses ATG::WaveFile (XDK built-in) to load and play WAV files.
// Matches the working XDK sample pattern exactly.
//============================================================================================
class AudioManager {
public:
    IXAudio2* pXAudio2;
    bool      initialized;

    AudioManager() {
        pXAudio2        = NULL;
        pMasteringVoice = NULL;
        pMusicVoice     = NULL;
        pMusicBuffer    = NULL;
        pMusicCallback  = NULL;
        initialized     = false;
        masterVolume    = 1.0f;
        numMusic        = 0;
        numSfx          = 0;
    }

    ~AudioManager() { Shutdown(); }

    // ------------------------------------------------------------------
    bool Initialize() {
        UINT32 flags = 0;
#ifdef _DEBUG
        flags |= XAUDIO2_DEBUG_ENGINE;
#endif
        // Xbox 360 requires the processor affinity argument (XboxThread5 = hardware audio thread).
        // Without it, XAudio2 never starts its processing thread and produces no sound.
        // On PC (non-_XBOX) the two-arg form is used instead.
#ifdef _XBOX
        if (FAILED(XAudio2Create(&pXAudio2, flags, XboxThread5))) {
#else
        if (FAILED(XAudio2Create(&pXAudio2, flags))) {
#endif
            OutputDebugStringA("Audio: XAudio2Create FAILED\n");
            return false;
        }

        // Xbox 360: explicit 2 channels, 44100 Hz for maximum WAV compatibility
        if (FAILED(pXAudio2->CreateMasteringVoice(
                &pMasteringVoice, 2, 44100, 0, 0, NULL))) {
            OutputDebugStringA("Audio: CreateMasteringVoice FAILED\n");
            pXAudio2->Release();
            pXAudio2 = NULL;
            return false;
        }

        initialized = true;
        OutputDebugStringA("Audio: Initialized OK\n");
        return true;
    }

    // ------------------------------------------------------------------
    void AddMusic(const char* name, const char* filePath) {
        if (numMusic >= AUDIO_MAX_MUSIC) return;
        strncpy(musicEntries[numMusic].name, name,     AUDIO_NAME_LEN - 1);
        strncpy(musicEntries[numMusic].path, filePath, AUDIO_PATH_LEN - 1);
        musicEntries[numMusic].loaded = true;
        numMusic++;
    }

    void AddSFX(const char* name, const char* filePath) {
        if (numSfx >= AUDIO_MAX_SFX) return;
        strncpy(sfxEntries[numSfx].name, name,     AUDIO_NAME_LEN - 1);
        strncpy(sfxEntries[numSfx].path, filePath, AUDIO_PATH_LEN - 1);
        sfxEntries[numSfx].loaded = true;
        numSfx++;
    }

    // ------------------------------------------------------------------
    // PlayMusic — streams from file using ATG::WaveFile, loops via LoopCount
    void PlayMusic(const char* name, float volume = 0.5f, bool loop = true) {
        if (!initialized || !pXAudio2) return;

        const char* path = findMusicPath(name);
        if (!path) {
            OutputDebugStringA("Audio: PlayMusic - name not registered\n");
            return;
        }

        // Stop current music voice
        if (pMusicVoice) {
            pMusicVoice->Stop(0);
            pMusicVoice->FlushSourceBuffers();
            pMusicVoice = NULL;
        }
        // Previous buffer cleaned up by callback, but if we're restarting
        // before it finishes, clean up manually
        if (pMusicCallback) {
            delete pMusicCallback;
            pMusicCallback = NULL;
            pMusicBuffer = NULL;  // callback owns the buffer
        }

        // Load WAV using ATG::WaveFile (XDK built-in — handles all formats)
        ATG::WaveFile waveFile;
        if (FAILED(waveFile.Open(path))) {
            OutputDebugStringA("Audio: PlayMusic WaveFile.Open FAILED\n");
            return;
        }

        WAVEFORMATEXTENSIBLE wfx = {0};
        waveFile.GetFormat(&wfx);

        DWORD cbSize = 0;
        waveFile.GetDuration(&cbSize);

        pMusicBuffer = new BYTE[cbSize];
        if (FAILED(waveFile.ReadSample(0, pMusicBuffer, cbSize, &cbSize))) {
            delete[] pMusicBuffer;
            pMusicBuffer = NULL;
            return;
        }

        pMusicCallback = new VoiceCallback(pMusicBuffer);

        // For standard PCM, use WAVEFORMATEX size — EXTENSIBLE causes E_INVALIDARG on Xbox
        WAVEFORMATEX* pFmt = (WAVEFORMATEX*)&wfx;
        if (FAILED(pXAudio2->CreateSourceVoice(
                &pMusicVoice, pFmt, 0, 2.0f, pMusicCallback))) {
            OutputDebugStringA("Audio: PlayMusic CreateSourceVoice FAILED\n");
            delete pMusicCallback;
            pMusicCallback = NULL;
            pMusicBuffer   = NULL;
            pMusicVoice    = NULL;
            return;
        }

        pMusicVoice->SetVolume(volume * masterVolume);

        XAUDIO2_BUFFER buffer = {0};
        buffer.pAudioData = pMusicBuffer;
        buffer.AudioBytes = cbSize;
        buffer.Flags      = XAUDIO2_END_OF_STREAM;
        buffer.pContext   = pMusicCallback;
        if (loop) buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

        pMusicVoice->SubmitSourceBuffer(&buffer);
        pMusicVoice->Start(0);
        OutputDebugStringA("Audio: PlayMusic started OK\n");
    }

    // ------------------------------------------------------------------
    // PlaySFX — fire-and-forget using ATG::WaveFile + VoiceCallback cleanup
    void PlaySFX(const char* name, float volume = 1.0f, bool loop = false) {
        if (!initialized || !pXAudio2) return;

        const char* path = findSFXPath(name);
        if (!path) return;

        ATG::WaveFile waveFile;
        if (FAILED(waveFile.Open(path))) return;

        WAVEFORMATEXTENSIBLE wfx = {0};
        waveFile.GetFormat(&wfx);

        DWORD cbSize = 0;
        waveFile.GetDuration(&cbSize);

        BYTE* pData = new BYTE[cbSize];
        if (FAILED(waveFile.ReadSample(0, pData, cbSize, &cbSize))) {
            delete[] pData;
            return;
        }

        VoiceCallback* pCB = new VoiceCallback(pData);

        IXAudio2SourceVoice* pVoice = NULL;
        WAVEFORMATEX* pFmtSFX = (WAVEFORMATEX*)&wfx;
        if (FAILED(pXAudio2->CreateSourceVoice(
                &pVoice, pFmtSFX, 0, 2.0f, pCB))) {
            delete[] pData;
            delete pCB;
            return;
        }

        pVoice->SetVolume(volume * masterVolume);

        XAUDIO2_BUFFER buffer = {0};
        buffer.pAudioData = pData;
        buffer.AudioBytes = cbSize;
        buffer.Flags      = XAUDIO2_END_OF_STREAM;
        buffer.pContext   = pCB;
        if (loop) buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

        pVoice->SubmitSourceBuffer(&buffer);
        pVoice->Start(0);
        // Voice and buffer are self-cleaning via VoiceCallback::OnBufferEnd
    }

    // ------------------------------------------------------------------
    void StopMusic() {
        if (pMusicVoice) {
            pMusicVoice->Stop(0);
            pMusicVoice = NULL;
        }
    }

    void SetMasterVolume(float v) {
        masterVolume = (v < 0.0f) ? 0.0f : (v > 1.0f ? 1.0f : v);
        if (pMasteringVoice) pMasteringVoice->SetVolume(masterVolume);
    }

    // ------------------------------------------------------------------
    void Shutdown() {
        if (pMusicVoice) { pMusicVoice->Stop(0); pMusicVoice = NULL; }
        // Note: pMusicBuffer freed by VoiceCallback::OnBufferEnd
        if (pXAudio2) { pXAudio2->Release(); pXAudio2 = NULL; }
        pMasteringVoice = NULL;
        initialized     = false;
        numMusic        = 0;
        numSfx          = 0;
        OutputDebugStringA("Audio: Shutdown OK\n");
    }

private:
    IXAudio2MasteringVoice* pMasteringVoice;
    IXAudio2SourceVoice*    pMusicVoice;
    BYTE*                   pMusicBuffer;
    VoiceCallback*          pMusicCallback;
    float                   masterVolume;

    AudioEntry  musicEntries[AUDIO_MAX_MUSIC];
    AudioEntry  sfxEntries[AUDIO_MAX_SFX];
    int         numMusic;
    int         numSfx;

    const char* findMusicPath(const char* name) {
        for (int i = 0; i < numMusic; i++)
            if (strcmp(musicEntries[i].name, name) == 0) return musicEntries[i].path;
        return NULL;
    }
    const char* findSFXPath(const char* name) {
        for (int i = 0; i < numSfx; i++)
            if (strcmp(sfxEntries[i].name, name) == 0) return sfxEntries[i].path;
        return NULL;
    }
};

// Audio path constants defined and used in audio.cpp only

void LoadAudioFiles();
