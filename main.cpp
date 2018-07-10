// FunLedStrip
//

#include "stdafx.h"
#include <mmdeviceapi.h>
#include <iostream>


// Gets default audio/multimedia device and allocates it to the input argument.
bool getDefaultDevice(IMMDevice **device) {
	IMMDeviceEnumerator *deviceEnumerator = NULL;
	HRESULT hr;
	hr = CoInitializeEx((void**)deviceEnumerator, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		printf("Unable to initialize COM: %x\n", hr);
		return false;
	}

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator), (void**)&deviceEnumerator);
	if (FAILED(hr)) {
		printf("Unable to instantiate device enumerator: %x\n", hr);
		return false;
	}

	hr = deviceEnumerator->GetDefaultAudioEndpoint(eCapture, eMultimedia, device);
	if (FAILED(hr)) {
		printf("Unable to get default multimedia device: %x\n", hr);
		return false;
	}
	return true;
}

int main() {
	IMMDevice *device = NULL;

	// Get audio device
	if (!getDefaultDevice(&device)) {
		return EXIT_FAILURE;
	}

	return 0;
}