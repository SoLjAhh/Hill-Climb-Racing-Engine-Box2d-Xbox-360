#pragma once
//============================================================================================
// WaveFile.h  — Drop-in replacement for ATG::WaveFile
// Uses the exact FindChunk/ReadChunkData pattern from the working XAudio2 MSDN sample.
//
// FOURCC BYTE ORDER — critical for Xbox 360:
//   Xbox 360 is big-endian. ReadFile puts byte[0] at the MSB of a DWORD.
//   File contains 'R','I','F','F' → DWORD = 0x52494646 → matches literal 'RIFF'.
//   On PC (little-endian), byte[0] is at the LSB, so 'R','I','F','F' → 0x46464952
//   which matches the reversed literal 'FFIR'.
//   The sample defines _XBOX to use normal strings, non-_XBOX to use reversed.
//============================================================================================
#include <xtl.h>
#include <xaudio2.h>

#ifdef _XBOX
#define WF_RIFF 'RIFF'
#define WF_DATA 'data'
#define WF_FMT  'fmt '
#define WF_WAVE 'WAVE'
#else
// PC little-endian — byte-reversed so ReadFile comparison works
#define WF_RIFF 'FFIR'
#define WF_DATA 'atad'
#define WF_FMT  ' tmf'
#define WF_WAVE 'EVAW'
#endif

namespace ATG {

//---------------------------------------------------------------------------------------------
// Low-level chunk helpers — direct port from the MSDN XAudio2 sample
//---------------------------------------------------------------------------------------------
static HRESULT WF_FindChunk(HANDLE hFile, DWORD fourcc,
                             DWORD& dwChunkSize, DWORD& dwChunkDataPosition)
{
    HRESULT hr = S_OK;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
        return HRESULT_FROM_WIN32(GetLastError());

    DWORD dwChunkType, dwChunkDataSize, dwRIFFDataSize = 0;
    DWORD dwFileType, bytesRead = 0, dwOffset = 0;

    while (hr == S_OK)
    {
        DWORD dwRead;
        if (!ReadFile(hFile, &dwChunkType,     sizeof(DWORD), &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());
        if (!ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());

        switch (dwChunkType)
        {
        case WF_RIFF:
            dwRIFFDataSize  = dwChunkDataSize;
            dwChunkDataSize = 4;
            if (!ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
                hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        default:
            if (INVALID_SET_FILE_POINTER ==
                SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
                return HRESULT_FROM_WIN32(GetLastError());
        }

        dwOffset += sizeof(DWORD) * 2;

        if (dwChunkType == fourcc) {
            dwChunkSize         = dwChunkDataSize;
            dwChunkDataPosition = dwOffset;
            return S_OK;
        }

        dwOffset += dwChunkDataSize;
        if (bytesRead >= dwRIFFDataSize) return S_FALSE;
    }
    return S_OK;
}

static HRESULT WF_ReadChunkData(HANDLE hFile, void* buffer,
                                DWORD buffersize, DWORD bufferoffset)
{
    if (INVALID_SET_FILE_POINTER ==
        SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
        return HRESULT_FROM_WIN32(GetLastError());
    DWORD dwRead;
    if (!ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
        return HRESULT_FROM_WIN32(GetLastError());
    return S_OK;
}

//---------------------------------------------------------------------------------------------
// ATG::WaveFile — same API surface used by audio.h
//---------------------------------------------------------------------------------------------
class WaveFile {
public:
    WaveFile() : m_hFile(INVALID_HANDLE_VALUE), m_dataOffset(0), m_dataSize(0) {
        memset(&m_wfx, 0, sizeof(m_wfx));
    }
    ~WaveFile() { Close(); }

    HRESULT Open(const char* szFilename) {
        Close();

        m_hFile = CreateFileA(szFilename, GENERIC_READ, FILE_SHARE_READ,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (m_hFile == INVALID_HANDLE_VALUE) {
            OutputDebugStringA("WaveFile::Open — CreateFile FAILED\n");
            return E_FAIL;
        }

        // Verify RIFF/WAVE header
        DWORD chunkSize = 0, chunkPos = 0;
        if (FAILED(WF_FindChunk(m_hFile, WF_RIFF, chunkSize, chunkPos))) {
            OutputDebugStringA("WaveFile::Open — RIFF chunk not found\n");
            Close(); return E_FAIL;
        }
        DWORD fileType = 0;
        WF_ReadChunkData(m_hFile, &fileType, sizeof(DWORD), chunkPos);
        if (fileType != WF_WAVE) {
            OutputDebugStringA("WaveFile::Open — not WAVE format\n");
            Close(); return E_FAIL;
        }

        // Read fmt chunk
        if (FAILED(WF_FindChunk(m_hFile, WF_FMT, chunkSize, chunkPos))) {
            OutputDebugStringA("WaveFile::Open — fmt chunk not found\n");
            Close(); return E_FAIL;
        }
        DWORD fmtBytes = (chunkSize < sizeof(m_wfx)) ? chunkSize : sizeof(m_wfx);
        if (FAILED(WF_ReadChunkData(m_hFile, &m_wfx, fmtBytes, chunkPos))) {
            Close(); return E_FAIL;
        }

        // Record data chunk position and size
        if (FAILED(WF_FindChunk(m_hFile, WF_DATA, chunkSize, chunkPos))) {
            OutputDebugStringA("WaveFile::Open — data chunk not found\n");
            Close(); return E_FAIL;
        }
        m_dataSize   = chunkSize;
        m_dataOffset = chunkPos;

        OutputDebugStringA("WaveFile::Open — OK\n");
        return S_OK;
    }

    void GetFormat(WAVEFORMATEXTENSIBLE* pWfx) {
        if (pWfx) *pWfx = m_wfx;
    }

    void GetDuration(DWORD* pBytes) {
        if (pBytes) *pBytes = m_dataSize;
    }

    HRESULT ReadSample(DWORD /*dwPosition*/, void* pBuffer,
                       DWORD cbBuffer, DWORD* pRead)
    {
        if (!pBuffer || m_hFile == INVALID_HANDLE_VALUE) return E_FAIL;
        DWORD toRead = (cbBuffer < m_dataSize) ? cbBuffer : m_dataSize;
        HRESULT hr = WF_ReadChunkData(m_hFile, pBuffer, toRead, m_dataOffset);
        if (pRead) *pRead = SUCCEEDED(hr) ? toRead : 0;
        return hr;
    }

    void Close() {
        if (m_hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(m_hFile);
            m_hFile = INVALID_HANDLE_VALUE;
        }
        m_dataOffset = 0;
        m_dataSize   = 0;
        memset(&m_wfx, 0, sizeof(m_wfx));
    }

private:
    HANDLE               m_hFile;
    WAVEFORMATEXTENSIBLE m_wfx;
    DWORD                m_dataOffset;
    DWORD                m_dataSize;
};

} // namespace ATG
