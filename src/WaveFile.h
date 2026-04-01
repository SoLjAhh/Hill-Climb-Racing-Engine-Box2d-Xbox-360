#pragma once
//============================================================================================
// WaveFile.h  — Drop-in replacement for ATG::WaveFile
// Uses the exact FindChunk/ReadChunkData pattern from the working XAudio2 MSDN sample.
//
// BYTE ORDER FIX — Xbox 360 big-endian:
//   WAV/RIFF is a LITTLE-ENDIAN format. All chunk sizes and WAVEFORMATEX fields are LE.
//   Xbox 360 (PowerPC) is big-endian, so every multi-byte integer read from the file
//   must be byte-swapped before use. Without this, chunk sizes like 16 (0x10000000)
//   read as 268 million, causing every seek and read to go to the wrong position.
//
//   FOURCC tags (4 ASCII bytes) compare correctly on both endians because the compiler
//   multi-char literal and the file bytes are in the same order.
//
//   16-bit PCM audio samples are also little-endian in WAV files and must be swapped
//   so XAudio2 on Xbox 360 gets native big-endian sample data.
//============================================================================================
#include <xtl.h>
#include <xaudio2.h>

//============================================================================================
// FOURCC CONSTANTS
// Xbox 360 is big-endian: multi-char literal 'RIFF' = 0x52494646, file bytes 'R','I','F','F'
// read into DWORD = 0x52494646 on BE. They match directly.
// On PC (little-endian), byte order is reversed so we use reversed literals.
//============================================================================================
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

//============================================================================================
// BYTE-SWAP HELPERS
// RIFF stores all integer fields (chunk sizes, WAVEFORMATEX members) in little-endian.
// On big-endian Xbox 360 we must swap after reading from file.
// On little-endian PC these are no-ops.
//============================================================================================
#ifdef _XBOX
inline DWORD WF_SwapDWORD(DWORD val) {
    return ((val & 0x000000FF) << 24) |
           ((val & 0x0000FF00) << 8)  |
           ((val & 0x00FF0000) >> 8)  |
           ((val & 0xFF000000) >> 24);
}
inline WORD WF_SwapWORD(WORD val) {
    return (WORD)(((val & 0x00FF) << 8) | ((val & 0xFF00) >> 8));
}
inline void WF_SwapSamples16(BYTE* pData, DWORD cbBytes) {
    // Swap every 16-bit PCM sample from LE to BE
    WORD* pSamples = (WORD*)pData;
    DWORD count    = cbBytes / 2;
    for (DWORD i = 0; i < count; i++) {
        pSamples[i] = WF_SwapWORD(pSamples[i]);
    }
}
#else
inline DWORD WF_SwapDWORD(DWORD val) { return val; }
inline WORD  WF_SwapWORD(WORD val)   { return val; }
inline void  WF_SwapSamples16(BYTE*, DWORD) {}
#endif

namespace ATG {

//---------------------------------------------------------------------------------------------
// Low-level chunk helpers — direct port from the MSDN XAudio2 sample
// with byte-swapping added for Xbox 360 big-endian
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

        // RIFF chunk sizes are little-endian — swap on big-endian Xbox 360
        // FOURCC tags are 4 ASCII bytes and compare correctly without swapping
        dwChunkDataSize = WF_SwapDWORD(dwChunkDataSize);

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
// WF_SwapWaveFormat — byte-swap all WAVEFORMATEX fields from LE (file) to native (BE) order
// Must be called after reading the fmt chunk on Xbox 360.
//---------------------------------------------------------------------------------------------
static void WF_SwapWaveFormat(WAVEFORMATEXTENSIBLE* pWfx) {
#ifdef _XBOX
    // WAVEFORMATEX base fields (all stored little-endian in WAV files)
    pWfx->Format.wFormatTag      = WF_SwapWORD(pWfx->Format.wFormatTag);
    pWfx->Format.nChannels       = WF_SwapWORD(pWfx->Format.nChannels);
    pWfx->Format.nSamplesPerSec  = WF_SwapDWORD(pWfx->Format.nSamplesPerSec);
    pWfx->Format.nAvgBytesPerSec = WF_SwapDWORD(pWfx->Format.nAvgBytesPerSec);
    pWfx->Format.nBlockAlign     = WF_SwapWORD(pWfx->Format.nBlockAlign);
    pWfx->Format.wBitsPerSample  = WF_SwapWORD(pWfx->Format.wBitsPerSample);
    pWfx->Format.cbSize          = WF_SwapWORD(pWfx->Format.cbSize);

    // WAVEFORMATEXTENSIBLE extra fields (only present if cbSize >= 22)
    if (pWfx->Format.cbSize >= 22) {
        pWfx->Samples.wValidBitsPerSample = WF_SwapWORD(pWfx->Samples.wValidBitsPerSample);
        pWfx->dwChannelMask = WF_SwapDWORD(pWfx->dwChannelMask);
        // SubFormat GUID: first 3 fields are LE, rest are bytes (no swap needed)
        // Data1 = DWORD, Data2 = WORD, Data3 = WORD
        pWfx->SubFormat.Data1 = WF_SwapDWORD(pWfx->SubFormat.Data1);
        pWfx->SubFormat.Data2 = WF_SwapWORD(pWfx->SubFormat.Data2);
        pWfx->SubFormat.Data3 = WF_SwapWORD(pWfx->SubFormat.Data3);
    }
#else
    (void)pWfx; // no-op on little-endian
#endif
}

//---------------------------------------------------------------------------------------------
// ATG::WaveFile — same API surface used by audio.h
//---------------------------------------------------------------------------------------------
class WaveFile {
public:
    WaveFile() : m_hFile(INVALID_HANDLE_VALUE), m_dataOffset(0), m_dataSize(0),
                 m_bitsPerSample(0) {
        memset(&m_wfx, 0, sizeof(m_wfx));
    }
    ~WaveFile() { Close(); }

    HRESULT Open(const char* szFilename) {
        Close();

        m_hFile = CreateFileA(szFilename, GENERIC_READ, FILE_SHARE_READ,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (m_hFile == INVALID_HANDLE_VALUE) {
            char msg[256];
            sprintf_s(msg, sizeof(msg),
                "WaveFile::Open — CreateFile FAILED for '%s' (err=%lu)\n",
                szFilename, GetLastError());
            OutputDebugStringA(msg);
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

        // Byte-swap WAVEFORMATEX fields from LE (file) to native BE (Xbox 360)
        WF_SwapWaveFormat(&m_wfx);

        // Cache bits-per-sample BEFORE any further processing (needed for PCM swap)
        m_bitsPerSample = m_wfx.Format.wBitsPerSample;

        // Record data chunk position and size
        if (FAILED(WF_FindChunk(m_hFile, WF_DATA, chunkSize, chunkPos))) {
            OutputDebugStringA("WaveFile::Open — data chunk not found\n");
            Close(); return E_FAIL;
        }
        m_dataSize   = chunkSize;
        m_dataOffset = chunkPos;

        {
            char msg[256];
            sprintf_s(msg, sizeof(msg),
                "WaveFile::Open — OK (%s) fmt=%d ch=%d rate=%lu bits=%d dataSize=%lu\n",
                szFilename,
                (int)m_wfx.Format.wFormatTag,
                (int)m_wfx.Format.nChannels,
                (unsigned long)m_wfx.Format.nSamplesPerSec,
                (int)m_wfx.Format.wBitsPerSample,
                (unsigned long)m_dataSize);
            OutputDebugStringA(msg);
        }
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
        if (SUCCEEDED(hr)) {
            // Byte-swap 16-bit PCM samples from LE to BE on Xbox 360
            // XAudio2 on Xbox 360 expects native big-endian sample data.
            // 8-bit PCM is single bytes (no swap needed).
            // Compressed formats (ADPCM etc) handle their own byte order.
            if (m_bitsPerSample == 16 && m_wfx.Format.wFormatTag == WAVE_FORMAT_PCM) {
                WF_SwapSamples16((BYTE*)pBuffer, toRead);
            }
            // EXTENSIBLE with PCM sub-format also needs swapping
            if (m_bitsPerSample == 16 && m_wfx.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
                // KSDATAFORMAT_SUBTYPE_PCM first DWORD = 0x00000001
                if (m_wfx.SubFormat.Data1 == 1) {
                    WF_SwapSamples16((BYTE*)pBuffer, toRead);
                }
            }
        }
        if (pRead) *pRead = SUCCEEDED(hr) ? toRead : 0;
        return hr;
    }

    void Close() {
        if (m_hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(m_hFile);
            m_hFile = INVALID_HANDLE_VALUE;
        }
        m_dataOffset    = 0;
        m_dataSize      = 0;
        m_bitsPerSample = 0;
        memset(&m_wfx, 0, sizeof(m_wfx));
    }

private:
    HANDLE               m_hFile;
    WAVEFORMATEXTENSIBLE m_wfx;
    DWORD                m_dataOffset;
    DWORD                m_dataSize;
    WORD                 m_bitsPerSample;  // cached for PCM swap in ReadSample
};

} // namespace ATG
