#include "common.hpp"
//void ByteBeat(DWORD(*expression)(DWORD)) {
//	HWAVEOUT hWaveOut = 0;
//	WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
//	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
//	char buffer[8000 * 30] = {};
//	for (DWORD t = 0; t < sizeof(buffer); ++t)
//		buffer[t] = static_cast<char>(expression(t));
//	WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
//	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
//	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
//	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
//	waveOutClose(hWaveOut);
//}
//
//DWORD myExpression(DWORD t) {
//	// return t * ((t & 4096 ? t % 655368 < 59392 ? 7 : t >> 6 : 32) + (1 & t >> 14)) >> (3 & t >> (t & 2048 ? 2 : 10)) | t >> (t & 16384 ? t & 4096 ? 4 : 3 : 2) ^ ((t >> 6 | t | t >> (t >> 16)) * 10 + ((t >> 11) & 7)) + t % 125 & t >> 8 | t >> 4 | t * t >> 8 & t >> 8;
//}

// µ÷ÓÃ ByteBeat(myExpression)