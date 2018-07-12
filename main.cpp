// FunLedStrip
//

#include "stdafx.h"
#include <mmdeviceapi.h>
#include <audioclient.h>

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
	format.nBlockAlign = 4; //Frame size
	format.wBitsPerSample = 16;
	format.cbSize = 0;

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

	printf("%u", bufferSize);

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
	
	return hr;
}


HRESULT recordAudio(BYTE** data) {
	HRESULT hr;

	UINT32 framesAvailable = 0; // Frames available during capture
	DWORD flags; // buffer flags

	Sleep(hnsActualDuration/10000);
	hr = captureClient->GetBuffer(data, &framesAvailable, &flags, NULL, NULL);
	if (FAILED(hr)) {
		printf("Unable to capture audio %x\n", hr);
		return hr;
	}

	// debug
	// Note that there will be discontinuity if the sleep parameters 
	// and hnsRequestedDuration aren't compatible. Also if you try 
	// to print with a low value of sleep CMD will crash, 
	// but the program still works. Tried writing to file with no crash.
	
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
	
	

	hr = captureClient->ReleaseBuffer(framesAvailable);
	if (FAILED(hr)) {
		printf("Unable to release buffer %x\n", hr);
		return hr;
	}
	return hr;
}


int main() {
	BYTE *data = NULL; // audio data is stored here
	
	// Get audio device
	if (FAILED(initAudio())) {
		return EXIT_FAILURE;
	}

	// record audio loop
	while (1) {
		if (FAILED(recordAudio(&data))) {
			return EXIT_FAILURE;
		}		
	}

	// stop audio stream
	if (FAILED(audioClient->Stop())) {
		return EXIT_FAILURE;
	}

	// Release windows interfaces
	audioClient->Release();
	captureClient->Release();

	return 0;
}