// FunLedStrip
//

#include "stdafx.h"
#include <mmdeviceapi.h>
#include <audioclient.h>
//#include "kfr/base.hpp"
//#include "kfr/dft.hpp"
//#include <kfr/io.hpp>


#include<fstream>

// global windows interfaces
IAudioCaptureClient *captureClient = NULL;
IAudioClient *audioClient = NULL;
REFERENCE_TIME hnsActualDuration;

// Opens an audio stream on default multimedia device.
HRESULT initAudio() {
	HRESULT hr;

	// local windows interfaces
	IMMDeviceEnumerator *deviceEnumerator = NULL;
	IMMDevice *device = NULL;

	REFERENCE_TIME hnsRequestedDuration = 1000000; // buffer size in 100-nanosecond units. 10 million = 1 s
	UINT32 bufferSize; // in frames. Is also sample size
	
	WAVEFORMATEX format; // struct containing info about wanted format
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 2;
	format.nSamplesPerSec = 48000;
	format.nAvgBytesPerSec = 192000;
	format.nBlockAlign = 4; //Frame size in bytes
	format.wBitsPerSample = 16;
	format.cbSize = 0; // init to 0 b/c pcm, see microsoft docs

	WAVEFORMATEX *pFormat = &format;

	hr = CoInitializeEx((void**)deviceEnumerator, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		printf("Unable to initialize COM: %x\n", hr);
		return hr;
	}

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator), (void**)&deviceEnumerator);
	if (FAILED(hr)) {
		printf("Unable to instantiate device enumerator: %x\n", hr);
		return hr;
	}

	hr = deviceEnumerator->GetDefaultAudioEndpoint(eCapture, eMultimedia, &device);
	if (FAILED(hr)) {
		printf("Unable to get default multimedia device: %x\n", hr);
		return hr;
	}
	
	hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&audioClient);
	if (FAILED(hr)) {
		printf("Unable to activate device: %x\n", hr);
		return hr;
	}

	hr = audioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &format, &pFormat);
	if (hr != S_OK) {
		printf("Format not supported. Change values of format variable.");
		return hr;
	}

	hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, hnsRequestedDuration, 
								0, &format, NULL);
	if (FAILED(hr)) {
		printf("Unable to initialize audio client: %x\n", hr);
		return hr;
	}

	hr = audioClient->GetBufferSize(&bufferSize);
	if (FAILED(hr)) {
		printf("Unable to get buffer size: %x\n", hr);
		return hr;
	}

	hr = audioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&captureClient);
	if (FAILED(hr)) {
		printf("Unable to get capture client: %x\n", hr);
		return hr;
	}

	hr = audioClient->Start();
	if (FAILED(hr)) {
		printf("Unable to start audio stream: %x\n", hr);
		return hr;
	}
	
	// Calculate the actual duration of the allocated buffer.
	hnsActualDuration = (double)hnsRequestedDuration * bufferSize / format.nSamplesPerSec;

	deviceEnumerator->Release();
	device->Release();
	CoTaskMemFree(pFormat);

	return hr;
}

// Fills buffer with recorded audio.
HRESULT recordAudio(short *data) {
	HRESULT hr;

	UINT32 framesAvailable = 0; // Frames available during capture
	DWORD flags; // buffer flags

	BYTE *temp = NULL;

	// Note that there will be discontinuity if the sleep parameters 
	// and hnsRequestedDuration aren't compatible.
	Sleep(hnsActualDuration/10000);
	hr = captureClient->GetBuffer(&temp, &framesAvailable, &flags, NULL, NULL);
	if (FAILED(hr)) {
		printf("Unable to capture audio %x\n", hr);
		return hr;
	}

	data = reinterpret_cast<short*>(temp);

	
	// debug 
	/*
	if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
		printf("silent\n");
	}
	else if (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) {
		printf("discontinuity\n");
	}
	else if (flags & AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR) {
		printf("time\n");
	}
	else {
		printf("clear\n");
	}
	*/

	hr = captureClient->ReleaseBuffer(framesAvailable);
	if (FAILED(hr)) {
		printf("Unable to release buffer %x\n", hr);
		return hr;
	}
	return hr;
}

// Normalizes 16 bit integers to floats in range -1.0 to 1.0
void normalize(short *input, float *output) {
	for (int i = 0; i < sizeof(input); i++) {
		output[i] = input[i] / 32768.0; // 2^15 = 32768
	}
}

//
//void fft(float *data)
//{
//	const size_t size = sizeof(data);			// fft size
//
//	kfr::univector<kfr::complex<float>, 0> in(data, sizeof(data));
//
//	kfr::dft_plan<kfr::i16> dft(size);		// initialize plan
//	kfr::univector<kfr::i16> temp(dft.size);	// allocate work buffer
//	dft.execute(freq, samples, temp);			// do the actual transform
//												// work with freq
//}

int main() {
	short data[1920/2]; // audio data is stored in this buffer.
						// short b/c 16 bit audio.
						// size (in bytes) can be calculated through 
						// no. of channels*bytes per sample*desired sample time*samples/sec
						// in this case: 10 ms.
						// divide by two to get 16 bit values.

	float fData[1920/2]; // used for normalized values.

	// initialize audio and start audio stream
	if (FAILED(initAudio())) {
		printf("Failed to initialize audio.");
		return EXIT_FAILURE;
	}


	// loop
	// record audio
	if (FAILED(recordAudio(data))) {
		printf("Failed to record audio.");
		return EXIT_FAILURE;
	}

	// normalize to float values between -1 and 1

	std::fstream fs;
	fs.open("test.txt", std::fstream::out);

	normalize(data, fData); // normalize data and store in fData.

	for (int i =0; i < sizeof(fData); i++) {
		printf("%f\n", fData[i]);
		fs << fData[i] << "\n";
	}


	fs.close();

	// transform to freq domain with fft

	// end loop.

	// stop audio stream
	if (FAILED(audioClient->Stop())) {
		printf("Failed to stop audio stream.");
		return EXIT_FAILURE;
	}

	// Release windows interfaces
	audioClient->Release();
	captureClient->Release();

	return 0;
}