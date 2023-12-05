#include <mmdeviceapi.h>
#include <objbase.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <iostream>
#include <Windows.h>
#include <Endpointvolume.h>
#include <iostream>

void HandleError(HRESULT hr, const char* errorMessage) {
    if (FAILED(hr)) {
        std::cerr << errorMessage << " HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
        // Perform cleanup or handle the error as needed
        CoUninitialize();
        exit(EXIT_FAILURE);
    }
}

void SetAudioBalanceForDevice(IMMDevice* pDevice) {
    IAudioEndpointVolume* pVolume = NULL;
    HRESULT hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pVolume);
    if (SUCCEEDED(hr)) {
        // Set the balance (range: 0.0 to 1.0, 0.5 for center)
        float balance = 1;  // Example balance value
        hr = pVolume->SetChannelVolumeLevelScalar(0, balance, NULL);
        hr = pVolume->SetChannelVolumeLevelScalar(1, balance, NULL);

        if (SUCCEEDED(hr)) {
            // Successfully set the audio balance for the device
            std::cout << "Audio balance set successfully for a device." << std::endl;
        } else {
            std::cerr << "Failed to set audio balance for a device. HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
        }
        // Release the volume interface
        pVolume->Release();
    } else {
        std::cerr << "Failed to activate IAudioEndpointVolume for a device. HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
    }
}

template<typename T>
struct com_pointer
{
    ~com_pointer() { ptr_->Release(); }
    operator T*() { return ptr_; }
    T* operator->() { return ptr_; }
    T** operator&() { return &ptr_; }

    T* ptr_;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // CoInitialize is necessary for COM initialization
    HRESULT hr = CoInitialize(NULL);
    HandleError(hr, "Failed to initialize COM.");

    IMMDeviceEnumerator* pEnumerator = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    HandleError(hr, "Failed to create MMDeviceEnumerator.");

    // Enumerate audio devices
    IMMDeviceCollection* pDeviceCollection = NULL;
    hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDeviceCollection);
    HandleError(hr, "Failed to enumerate audio endpoints.");

    UINT deviceCount = 0;
    hr = pDeviceCollection->GetCount(&deviceCount);
    HandleError(hr, "Failed to get the device count.");

    for (UINT i = 0; i < deviceCount; ++i) {
        IMMDevice* pDevice = NULL;
        hr = pDeviceCollection->Item(i, &pDevice);
        if (SUCCEEDED(hr)) {
	        com_pointer<IPropertyStore> properties;
	        if (pDevice->OpenPropertyStore(STGM_READ, &properties) != S_OK)
		        continue;

	        PROPVARIANT name;
	        if (properties->GetValue(PKEY_Device_FriendlyName, &name) != S_OK)
		        continue;

	        LPWSTR id;
	        if (pDevice->GetId(&id) != S_OK)
		        continue;

            std::wcout << id << " - " << name.pwszVal << "\n";

	        if (wcsstr(name.pwszVal, L"Arctis") != nullptr) { SetAudioBalanceForDevice(pDevice); }

            pDevice->Release();
        } else {
            std::cerr << "Failed to get the device at index " << i << ". HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
        }
    }

    // Release interfaces and clean up
    pDeviceCollection->Release();
    pEnumerator->Release();

    // CoUninitialize to clean up COM
    CoUninitialize();

    return 0;
}
