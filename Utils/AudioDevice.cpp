#include "AudioDevice.h"
#include "Mmdeviceapi.h"
#include "PolicyConfig.h"
#include "Propidl.h"
#include "Functiondiscoverykeys_devpkey.h"
#include <Windows.h>

bool AudioDevice::isNull()
{
    return id.isEmpty();
}

bool AudioDevice::setDefaultOutputDevice(QString devID)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); //单线程并发
    if (FAILED(hr)) return false;
    IPolicyConfigVista* pPolicyConfig;
    ERole reserved = eConsole;

    hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient), NULL, CLSCTX_ALL, __uuidof(IPolicyConfigVista), (LPVOID*)&pPolicyConfig);
    if (SUCCEEDED(hr)) {
        hr = pPolicyConfig->SetDefaultEndpoint(devID.toStdWString().c_str(), reserved);
        pPolicyConfig->Release();
    }
    ::CoUninitialize();
    return SUCCEEDED(hr);
}

QList<AudioDevice> AudioDevice::enumOutputDevice()
{
    QList<AudioDevice> res;
    HRESULT hr = CoInitialize(NULL); // == CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return res;
    IMMDeviceEnumerator* pEnum = NULL;
    // Create a multimedia device enumerator.
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
    if (FAILED(hr)) return res;
    IMMDeviceCollection* pDevices;
    // Enumerate the output devices.
    hr = pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices); //eRender == out; eCapture == in
    if (FAILED(hr)) return res;
    UINT count;
    hr = pDevices->GetCount(&count);
    if (FAILED(hr)) return res;
    for (UINT i = 0; i < count; i++) {
        IMMDevice* pDevice;
        hr = pDevices->Item(i, &pDevice);
        if (SUCCEEDED(hr)) {
            LPWSTR wstrID = NULL;
            hr = pDevice->GetId(&wstrID);
            if (SUCCEEDED(hr)) {
                IPropertyStore* pStore;
                hr = pDevice->OpenPropertyStore(STGM_READ, &pStore);
                if (SUCCEEDED(hr)) {
                    PROPVARIANT friendlyName;
                    PropVariantInit(&friendlyName);
                    hr = pStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);
                    if (SUCCEEDED(hr)) {
                        res << AudioDevice(QString::fromWCharArray(wstrID), QString::fromWCharArray(friendlyName.pwszVal));
                        PropVariantClear(&friendlyName);
                    }
                    pStore->Release();
                }
            }
            pDevice->Release();
        }
    }
    pDevices->Release();
    pEnum->Release();
    ::CoUninitialize();
    return res;
}

AudioDevice AudioDevice::defaultOutputDevice()
{
    AudioDevice res;
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) return res;
    IMMDeviceEnumerator* pEnum = NULL;
    // Create a multimedia device enumerator.
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
    if (FAILED(hr)) return res;
    // Enumerate the output devices.
    IMMDevice* pDevice;
    hr = pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice); //eRender == out; eCapture == in
    if (FAILED(hr)) return res;
    LPWSTR wstrID = NULL;
    hr = pDevice->GetId(&wstrID);
    if (SUCCEEDED(hr)) {
        IPropertyStore* pStore;
        hr = pDevice->OpenPropertyStore(STGM_READ, &pStore);
        if (SUCCEEDED(hr)) {
            PROPVARIANT friendlyName;
            PropVariantInit(&friendlyName);
            hr = pStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);
            if (SUCCEEDED(hr)) {
                res.id = QString::fromWCharArray(wstrID);
                res.name = QString::fromWCharArray(friendlyName.pwszVal);
                PropVariantClear(&friendlyName);
            }
            pStore->Release();
        }
    }
    pDevice->Release();
    pEnum->Release();
    ::CoUninitialize();
    return res;
}

QDebug operator<<(QDebug dbg, const AudioDevice& dev)
{
    dbg << dev.id << " " << dev.name;
    return dbg;
}
bool operator==(const AudioDevice& lhs, const AudioDevice& rhs)
{
    return lhs.id == rhs.id;
}
