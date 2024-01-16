// Wav.h

#ifndef WAV_HDR
#define WAV_HDR

#include "mmeapi.h"				// linker: include winmm.lib
#include <vector>

using std::vector;

enum Channel { C_Left, C_Right, C_Mono };

class Wav {
public:
	WAVEHDR wh;
	HWAVEOUT waveOut;
	short *source = NULL;
	int nSamplesToPlay;
	time_t startPlay = 0;
	bool ReadWAV(const char *filename, vector<short> &samples, bool verbose = false);
	void OpenDevice(bool mono, int samplingRate = 44100);
	void CloseDevice();
	void PlayStereo(short *samples, int nStereoPairs, int samplingRate = 44100);
	void PlayMono(short *samples, int nMonoSamples, int samplingRate = 44100);
	void PlayMonoScale(short *samples, int nMonoSamples, int samplingRate = 44100, float scale = 1);
	float FractionPlayed();
	int PlayState(); // WOM_CLOSE, WOM_OPEN, WOM_DONE
	float ElapsedTime();
private:
	void Play(short *samples, int nSamples, int samplingRate, bool mono, WAVEHDR &w, HWAVEOUT &waveOut);
};

void StereoToMono(vector<short> &stereo, vector<short> &mono);

#endif // WAV_HDR
