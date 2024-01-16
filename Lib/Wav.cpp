// Wav.cpp

#include <glad.h>
#include <time.h>
#include "Wav-old.h"

using std::vector;

// read

void PrintWAV(const char *name, int format) {
	char *c = (char *) &format;
	printf("%s: %c%c%c%c\n", name, c[0], c[1], c[2], c[3]);
}

bool Wav::ReadWAV(const char *filename, vector<short> &samples, bool verbose) {
	FILE *in = fopen(filename, "rb");
	if (!in)
		return false;
	struct RiffHeader {
		int fileID;		// should be "RIFF" (resource interchange file format)
		int fileSize;	// size of file in bytes less 8
		int typeID;		// should be "WAVE" (waveform audio file format)
	} r;
	struct FmtHeader {
		int waveID;		// should be "fmt "
		int headerSize;	// in bytes (less 4), should be 16 unless extra bytes
		short compression;
		short nChannels;
		int samplingRate;
		int aveBytesPerSec;
		short blockAlign;
		short sigBitsPerSamp;
	} fmt;
	if (fread(&r, sizeof(RiffHeader), 1, in) != 1 ||
	    fread(&fmt, sizeof(FmtHeader), 1, in) != 1) {
			printf("can't read header\n");
			fclose(in);
			return false;
	}
	if (fmt.nChannels < 1 || fmt.nChannels > 2) printf("%i channels!\n", fmt.nChannels);
	if (fmt.samplingRate != 44100) printf("sampling rate = %i!\n", fmt.samplingRate);
	if (fmt.sigBitsPerSamp != 16) printf("%i bits per sample!\n", fmt.sigBitsPerSamp);
	if (fmt.blockAlign/fmt.nChannels != 2) printf("blockAlign = %i!\n", fmt.blockAlign);
	if (verbose) {
		PrintWAV("fileID", r.fileID);
		PrintWAV("typeID", r.typeID);
		printf("file size = %i\n", r.fileSize);
		PrintWAV("waveID", fmt.waveID);
		printf("header size = %i\n", fmt.headerSize);
		printf("compression code = %i\n", fmt.compression);
		printf("%i channels\n", fmt.nChannels);
		printf("sampling rate = %i\n", fmt.samplingRate);
		printf("aveBytesPerSec = %i\n", fmt.aveBytesPerSec);
		printf("block align = %i\n", fmt.blockAlign);
		printf("sigBitsperSamp = %i\n", fmt.sigBitsPerSamp);
		printf("RiffHeader %i bytes, FmtHeader %i bytes\n", sizeof(RiffHeader), sizeof(FmtHeader));
	}
	if (fmt.headerSize > 16) {
		short nExtraFormatBytes;
		if (fread(&nExtraFormatBytes, 2, 1, in) == 1) {
			char buf[100];
			fread(buf, nExtraFormatBytes, 1, in);
			if (verbose)
				printf("%i extra bytes in Wave header\n", nExtraFormatBytes);
		}
	}
	int nSampleBytes = r.fileSize-12-fmt.headerSize;
	int nSamples = nSampleBytes/(2*sizeof(short))-68; // 68 determined empirically
	samples.resize(2*nSamples);
	int nSamplesRead = fread(samples.data(), 2*sizeof(short), nSamples, in);
	if (nSamplesRead != nSamples) {
		printf("** only read %i samples\n", nSamplesRead);
		samples.resize(nSamplesRead);
	}
	if (verbose)
		printf("%i 16-bit stereo samples (%3.2f secs)\n", nSamples, (float) nSamples/44100);
	fclose(in);
	return true;
}

// play

int playState = WOM_CLOSE;	// WOM_CLOSE, WOM_OPEN, WOM_DONE
 
void CALLBACK waveOutCallback(HWAVEOUT wo, UINT msg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
	playState = msg;
}

int nLoops = 5;

void Wav::Play(short *samples, int nSamples, int samplingRate, bool mono, WAVEHDR &w, HWAVEOUT &waveOut) {
	// https://docs.microsoft.com/en-us/windows/win32/api/mmeapi/ns-mmeapi-waveformatex
	CloseDevice(); // *** needed?
	OpenDevice(mono, samplingRate);
	playState = WOM_OPEN;
	w.lpData = (char *) samples;
	w.dwBufferLength = nSamples*(mono? 2 : 4);
	w.dwBytesRecorded = 0;
	w.dwUser = NULL;
	if (nLoops) {
		w.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
		w.dwLoops = nLoops;
	}
	else {
		w.dwFlags = 0;
		w.dwLoops = 0;
	}
	w.lpNext = 0;
	w.reserved = 0;
	MMRESULT r2 = waveOutPrepareHeader(waveOut, &w, sizeof(WAVEHDR));
	startPlay = clock();
	MMRESULT r3 = waveOutWrite(waveOut, &w, sizeof(WAVEHDR));
}

void Wav::CloseDevice() {
	if (waveOut)
		waveOutClose(waveOut);
}

void Wav::OpenDevice(bool mono, int samplingRate) {
	WAVEFORMATEX f;
	f.wFormatTag = WAVE_FORMAT_PCM;
	f.nChannels = mono? 1 : 2;						// fmt.nChannels
	f.nSamplesPerSec = samplingRate;				// fmt.samplingRate (# samples per sec)
		// **** does not appear to influence playback (stuck at 44.1 khz)
	f.nBlockAlign = mono? 2 : 4;					// fmt.blockAlignfmt.sigBitsPerSamp
	f.nAvgBytesPerSec = samplingRate*f.nBlockAlign;	// fmt.aveBytesPerSec
	f.wBitsPerSample = 16;
	f.cbSize = 0;
	MMRESULT r1 = waveOutOpen(&waveOut, WAVE_MAPPER, &f, (DWORD_PTR) &waveOutCallback, NULL, WAVE_FORMAT_DIRECT | CALLBACK_FUNCTION);
}

void Wav::PlayStereo(short *samples, int nStereoPairs, int samplingRate) {
	source = samples;
	nSamplesToPlay = nStereoPairs;
	Play(samples, nStereoPairs, samplingRate, false, wh, waveOut);
}

void Wav::PlayMonoScale(short *samples, int nMonoSamples, int samplingRate, float scale) {
	short scaleSamples[2000000];
	source = samples;
	nSamplesToPlay = nMonoSamples;
	memcpy(scaleSamples, samples, nMonoSamples*sizeof(short));
	for (int i = 0; i < nMonoSamples; i++)
		scaleSamples[i] = (short) (scale*samples[i]);
	Play(scaleSamples, nMonoSamples, samplingRate, true, wh, waveOut);
}

void Wav::PlayMono(short *samples, int nMonoSamples, int samplingRate) {
	source = samples;
	nSamplesToPlay = nMonoSamples;
	Play(samples, nMonoSamples, samplingRate, true, wh, waveOut);
}

void StereoToMono(vector<short> &stereo, vector<short> &mono) {
	int nsamples = stereo.size()/2;
	mono.resize(nsamples);
	for (int i = 0; i < nsamples; i++)
		mono[i] = stereo[2*i]+stereo[2*i+1];
}

int Wav::PlayState() {
	return playState;
}

float Wav::ElapsedTime() {
	return (float)(clock()-startPlay)/CLOCKS_PER_SEC;
}

float Wav::FractionPlayed() {
	return PlayState() == WOM_DONE? 1 : ElapsedTime()/((float)nSamplesToPlay/44100.f);
}

/*
void PrintResult(const char * title, MMRESULT r) {
	struct Msg { int n; const char *c; } msg[] = {
		{0, "success"}, {MMSYSERR_ALLOCATED, "already allocated"}, {MMSYSERR_BADDEVICEID, "bad device id"}, {MMSYSERR_NODRIVER, "no device driver"},
		{MMSYSERR_NOMEM, "can't allocate/lock memory"}, {WAVERR_BADFORMAT, "unsupported format"}, {WAVERR_SYNC, "synch device, no WAVE_ALLOWSYNC"},
		{MMSYSERR_INVALHANDLE, "invalid device handle"}, {WAVERR_UNPREPARED, "data block unprepared"}
	};
	for (int i =  0; i < sizeof(msg)/sizeof(Msg); i++)
		if (msg[i].n == r) { printf("%s: %s\n", title, msg[i].c); return; }
	printf("failure code %i\n", r);
}
*/
