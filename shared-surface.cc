#include <d3d11.h>
#include <stdio.h>
int main(int argc, char **argv)
{
  int size;
  ID3D11Device *device;
  ID3D11DeviceContext *context;
  D3D_FEATURE_LEVEL level;
  D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_DEBUG,
                    NULL, 0, D3D11_SDK_VERSION, &device,
                    &level, &context);

  HRESULT hr;
  ID3D11Texture2D *texture;
  ID3D11Texture2D *textureCPU;
  {
    D3D11_TEXTURE2D_DESC desc;
    desc.Width = 400;
    desc.Height = 400;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    hr = device->CreateTexture2D(&desc, NULL, &texture);
  }
  printf("texture %x %p\n", hr, texture);
  {
    D3D11_TEXTURE2D_DESC desc;
    desc.Width = 400;
    desc.Height = 400;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;
    desc.BindFlags = 0;
    hr = device->CreateTexture2D(&desc, NULL, &textureCPU);
  }

  printf("textureCPU %x %p\n", hr, textureCPU);
  HANDLE shareHandle;
  IDXGIResource* otherResource(NULL);
  hr = texture->QueryInterface( __uuidof(IDXGIResource), (void**)&otherResource );
  hr = otherResource->GetSharedHandle(&shareHandle);
  printf("%x %p\n", hr, shareHandle);

  D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
  rtDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
  rtDesc.Texture2D.MipSlice = 0;
  ID3D11RenderTargetView *rtView;
  hr = device->CreateRenderTargetView(texture, &rtDesc, &rtView);
  printf("%x %p\n", hr, rtView);
  FLOAT clearColor[4] = {0.5, 0, 0.5, 0.5};
  context->ClearRenderTargetView(rtView, clearColor);

  ID3D11Resource *sharedResource;
  ID3D11Texture2D *sharedTexture;
  hr = device->OpenSharedResource(shareHandle, __uuidof(ID3D11Resource), (void**)(&sharedResource));

  sharedResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)(&sharedTexture));
  printf("OpenShared %x %p %p\n", hr, sharedResource, sharedTexture);
  context->CopyResource(textureCPU, texture);
  {
    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = context->Map(textureCPU, 0, D3D11_MAP_READ, 0, &mapped);
    printf("%x %p\n", hr, mapped.pData);
    printf("color %x\n", *(int*)mapped.pData);
  }
  context->CopyResource(textureCPU, sharedTexture);
  {
    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = context->Map(textureCPU, 0, D3D11_MAP_READ, 0, &mapped);
    printf("%x %p\n", hr, mapped.pData);
    printf("color %x\n", *(int*)mapped.pData);
  }


  otherResource->Release();
  texture->Release();
  textureCPU->Release();
  rtView->Release();
  context->Release();
  device->Release();

}

