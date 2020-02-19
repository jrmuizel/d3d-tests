#include <d3d11.h>
#include <stdio.h>
#include <windows.h>
#include <assert.h>
#define WIDTH 8196
#define HEIGHT 8196

int buf[WIDTH*HEIGHT];
int buf2[WIDTH*HEIGHT];

void
delupms(void *b, size_t length)
{
  int sum = 0;
  char *buf = (char*)b;
  char *end = buf + length;
  while (buf < end) {
    sum += *buf;
    buf += 0x1000;
  }
  printf("sum: %d\n", sum);
}

double ticksToMS;
int main(int argc, char **argv)
{
  int size;
  ID3D11Device *device;
  ID3D11DeviceContext *context;
  D3D_FEATURE_LEVEL level;
  D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
                    NULL, 0, D3D11_SDK_VERSION, &device,
                    &level, &context);
  printf("%p\n", device);
  if (argc > 1)
    size = strtol(argv[1], 0, NULL);
  else
    size = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
  int size_low = 0;
  int trial_size;
  HRESULT result;
  size_t byte_size = sizeof(buf);
      
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    printf("frequency %lld\n", freq.QuadPart);
    ticksToMS = 1000./freq.QuadPart;
    trial_size = size;
    result = 0x800000;
    D3D11_TEXTURE2D_DESC desc;
    desc.Width = WIDTH;
    desc.Height = HEIGHT;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    {
      LARGE_INTEGER before;
      LARGE_INTEGER after;
      LONGLONG time;


      ID3D11Texture2D *small_texture;
      D3D11_TEXTURE2D_DESC small_desc = desc;
      //small_desc.Height = 1;
      result = device->CreateTexture2D(&small_desc, NULL, &small_texture);

      QueryPerformanceCounter(&before);
      D3D11_BOX box;
      box.left = 0;
      box.right = WIDTH;
      box.top = 0;
      box.bottom = 1;
      box.front = 0;
      box.back = 1;
      //context->UpdateSubresource(small_texture, 0, &box, buf, WIDTH*4, sizeof(buf));
      QueryPerformanceCounter(&after);
      time = after.QuadPart - before.QuadPart;
      printf("%x updatesubresource time: %.1fms\n", result, time*ticksToMS);


      ID3D11Texture2D *texture2;
      result = device->CreateTexture2D(&desc, NULL, &texture2);

      D3D11_TEXTURE2D_DESC staging_desc = desc;
      staging_desc.Usage = D3D11_USAGE_STAGING;
      staging_desc.BindFlags = 0;
      staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
      ID3D11Texture2D *staging_texture;
      QueryPerformanceCounter(&before);
      result = device->CreateTexture2D(&staging_desc, NULL, &staging_texture);
      QueryPerformanceCounter(&after);
      time = after.QuadPart - before.QuadPart;
      printf("%x staging time: %.1fms\n", result, time*ticksToMS);

      D3D11_MAPPED_SUBRESOURCE map;
      QueryPerformanceCounter(&before);
      result = context->Map(staging_texture, 0, D3D11_MAP_READ_WRITE, 0, &map);
      QueryPerformanceCounter(&after);
      time = after.QuadPart - before.QuadPart;
      printf("%x staging map time: %.1fms\n", result, time*ticksToMS);

      QueryPerformanceCounter(&before);
      delupms(map.pData, HEIGHT*WIDTH*4);
      QueryPerformanceCounter(&after);
      time = after.QuadPart - before.QuadPart;
      printf("%x delup mapped data time: %.1fms\n", result, time*ticksToMS);

/*      QueryPerformanceCounter(&before);
      delupms(buf, HEIGHT*WIDTH*4);
      QueryPerformanceCounter(&after);
      time = after.QuadPart - before.QuadPart;
      printf("%x delup buf data time: %.1fms\n", result, time*ticksToMS);
*/

      QueryPerformanceCounter(&before);
      delupms(buf, HEIGHT*WIDTH*4);
      delupms(buf2, HEIGHT*WIDTH*4);
      memcpy(buf2, buf, HEIGHT*WIDTH*4);
      QueryPerformanceCounter(&after);
      time = after.QuadPart - before.QuadPart;
      printf("%x buf memcpy time: %.1fms %fMB/s\n", result, time*ticksToMS, (byte_size/(1000.*1000.))/(time*ticksToMS/1000));

      QueryPerformanceCounter(&before);
      memcpy(buf2, buf, HEIGHT*WIDTH*4);
      QueryPerformanceCounter(&after);
      time = after.QuadPart - before.QuadPart;
      printf("%x buf memcpy time: %.1fms %fMB/s\n", result, time*ticksToMS, (byte_size/(1000.*1000.))/(time*ticksToMS/1000));

      QueryPerformanceCounter(&before);
      memcpy(map.pData, buf, HEIGHT*WIDTH*4);
      QueryPerformanceCounter(&after);
      time = after.QuadPart - before.QuadPart;
      printf("%x mapped memcpy time: %.1fms %fMB/s\n", result, time*ticksToMS, (byte_size/(1000.*1000.))/(time*ticksToMS/1000));



      QueryPerformanceCounter(&before);
      context->Unmap(staging_texture, 0);
      QueryPerformanceCounter(&after);
      time = after.QuadPart - before.QuadPart;
      printf("%x staging unmap time: %.1fms\n", result, time*ticksToMS);

      {
      D3D11_QUERY_DESC query_desc = {};
      query_desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
      ID3D11Query *query;
      device->CreateQuery(&query_desc, &query);

      D3D11_QUERY_DESC time_query_desc = {};
      time_query_desc.Query = D3D11_QUERY_TIMESTAMP;
      ID3D11Query *time_before_query;
      device->CreateQuery(&time_query_desc, &time_before_query);
      ID3D11Query *time_after_query;
      device->CreateQuery(&time_query_desc, &time_after_query);


      context->Begin(query);
      context->Begin(time_before_query);
      context->End(time_before_query);
      QueryPerformanceCounter(&before);
      context->CopyResource(staging_texture, texture2);
      context->CopyResource(staging_texture, small_texture);
      QueryPerformanceCounter(&after);
      time = after.QuadPart - before.QuadPart;
      context->Begin(time_after_query);
      context->End(time_after_query);
      context->End(query);
      printf("%x copy time: %.1fms %lld %lld\n", result, time*ticksToMS, after.QuadPart, before.QuadPart);

      QueryPerformanceCounter(&before);
      D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data;
      do {
	      result = context->GetData(query, &data, sizeof(data), 0);
      } while (result != 0);
      UINT64 time_before;
      UINT64 time_after;
            do {
	      result = context->GetData(time_after_query, &time_after, sizeof(time_after), 0);
      } while (result != 0);
      QueryPerformanceCounter(&after);
	    do {
	      result = context->GetData(time_before_query, &time_before, sizeof(time_before), 0);
      } while (result != 0);
      assert(!data.Disjoint);
      printf("copy time: %fms\n", (time_after-time_before)*1000./data.Frequency);
      time = after.QuadPart - before.QuadPart;
      printf("%x copy event time: %.1fms\n", result, time*ticksToMS);
      }
      Sleep(1000);
      printf("\n\n");

      ID3D11Texture2D *texture3, *texture5;
      {
      LARGE_INTEGER before;
      LARGE_INTEGER after;
      QueryPerformanceCounter(&before);

      result = device->CreateTexture2D(&desc, NULL, &texture3);
      D3D11_QUERY_DESC query_desc = {};
      query_desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
      ID3D11Query *query;
      device->CreateQuery(&query_desc, &query);

      D3D11_QUERY_DESC time_query_desc = {};
      time_query_desc.Query = D3D11_QUERY_TIMESTAMP;
      ID3D11Query *time_before_query;
      device->CreateQuery(&time_query_desc, &time_before_query);
      ID3D11Query *time_after_query;
      device->CreateQuery(&time_query_desc, &time_after_query);


      context->Begin(query);
      context->Begin(time_before_query);
      context->End(time_before_query);
      QueryPerformanceCounter(&before);
      context->CopyResource(texture2, texture3);
      QueryPerformanceCounter(&after);
      time = after.QuadPart - before.QuadPart;
      context->Begin(time_after_query);
      context->End(time_after_query);
      context->End(query);
      printf("%x copy time: %.1fms %lld %lld\n", result, time*ticksToMS, after.QuadPart, before.QuadPart);

      D3D11_BOX box;
      QueryPerformanceCounter(&before);
      box.left = 0;
      box.right = WIDTH;
      box.top = 0;
      box.bottom = 1;
      box.front = 0;
      box.back = 1;
      context->UpdateSubresource(small_texture, 0, &box, buf, WIDTH*4, sizeof(buf));
      QueryPerformanceCounter(&after);
      time = after.QuadPart - before.QuadPart;
      printf("%x updatesubresource time: %.1fms\n", result, time*ticksToMS);


      QueryPerformanceCounter(&before);
      D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data;
      do {
	      result = context->GetData(query, &data, sizeof(data), 0);
      } while (result != 0);
      UINT64 time_before;
      UINT64 time_after;
            do {
	      result = context->GetData(time_after_query, &time_after, sizeof(time_after), 0);
      } while (result != 0);
      QueryPerformanceCounter(&after);
	    do {
	      result = context->GetData(time_before_query, &time_before, sizeof(time_before), 0);
      } while (result != 0);

      assert(!data.Disjoint);
      printf("copy time: %fms\n", (time_after-time_before)*1000./data.Frequency);
      time = after.QuadPart - before.QuadPart;
      printf("%x copy event time: %.1fms\n", result, time*ticksToMS);
      }

      Sleep(1000);
      printf("\n\n");

      {
      LARGE_INTEGER before;
      LARGE_INTEGER after;
      QueryPerformanceCounter(&before);

      desc.Usage = D3D11_USAGE_DEFAULT;
      desc.CPUAccessFlags = 0;
      desc.MiscFlags = 0;
      desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

      D3D11_QUERY_DESC query_desc = {};
      query_desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
      ID3D11Query *query;
      device->CreateQuery(&query_desc, &query);

      D3D11_QUERY_DESC time_query_desc = {};
      time_query_desc.Query = D3D11_QUERY_TIMESTAMP;
      ID3D11Query *time_before_query;
      device->CreateQuery(&time_query_desc, &time_before_query);
      ID3D11Query *time_after_query;
      device->CreateQuery(&time_query_desc, &time_after_query);


      result = device->CreateTexture2D(&desc, NULL, &texture5);
      context->Begin(query);
      context->Begin(time_before_query);
      context->End(time_before_query);
      QueryPerformanceCounter(&before);
      context->CopyResource(texture2, texture5);
      QueryPerformanceCounter(&after);
      time = after.QuadPart - before.QuadPart;
      context->Begin(time_after_query);
      context->End(time_after_query);
      context->End(query);
      printf("%x copy time: %.1fms\n", result, time*ticksToMS);

      QueryPerformanceCounter(&before);
      D3D11_BOX box;
      box.left = 0;
      box.right = WIDTH;
      box.top = 0;
      box.bottom = 1;
      box.front = 0;
      box.back = 1;
      context->UpdateSubresource(small_texture, 0, &box, buf, WIDTH*4, sizeof(buf));
      QueryPerformanceCounter(&after);
      time = after.QuadPart - before.QuadPart;
      printf("%x updatesubresource time: %.1fms\n", result, time*ticksToMS);


      QueryPerformanceCounter(&before);
      D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data;
      do {
	      result = context->GetData(query, &data, sizeof(data), 0);
      } while (result != 0);
      UINT64 time_before;
      UINT64 time_after;
            do {
	      result = context->GetData(time_after_query, &time_after, sizeof(time_after), 0);
      } while (result != 0);
      QueryPerformanceCounter(&after);
	    do {
	      result = context->GetData(time_before_query, &time_before, sizeof(time_before), 0);
      } while (result != 0);

      assert(!data.Disjoint);
      printf("copy time: %fms\n", (time_after-time_before)*1000./data.Frequency);
      time = after.QuadPart - before.QuadPart;
      printf("%x copy event time: %.1fms\n", result, time*ticksToMS);
      }


      Sleep(1000);
      printf("\n\n");
    ID3D11Texture2D *texture4;
      {
      LARGE_INTEGER before;
      LARGE_INTEGER after;
      QueryPerformanceCounter(&before);

      result = device->CreateTexture2D(&desc, NULL, &texture4);
      D3D11_QUERY_DESC query_desc = {};
      query_desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
      ID3D11Query *query;
      device->CreateQuery(&query_desc, &query);

      D3D11_QUERY_DESC time_query_desc = {};
      time_query_desc.Query = D3D11_QUERY_TIMESTAMP;
      ID3D11Query *time_before_query;
      device->CreateQuery(&time_query_desc, &time_before_query);
      ID3D11Query *time_after_query;
      device->CreateQuery(&time_query_desc, &time_after_query);


      context->Begin(query);
      context->Begin(time_before_query);
      context->End(time_before_query);
      QueryPerformanceCounter(&before);
      context->CopyResource(texture2, texture4);
      QueryPerformanceCounter(&after);
      time = after.QuadPart - before.QuadPart;
      context->Begin(time_after_query);
      context->End(time_after_query);
      context->End(query);
      printf("%x copy time: %.1fms\n", result, time*ticksToMS);

      QueryPerformanceCounter(&before);
      result = context->Map(staging_texture, 0, D3D11_MAP_READ_WRITE, 0, &map);
      QueryPerformanceCounter(&after);
      time = after.QuadPart - before.QuadPart;
      printf("%x staging map time: %.1fms\n", result, time*ticksToMS);

      QueryPerformanceCounter(&before);
      context->Unmap(staging_texture, 0);
      QueryPerformanceCounter(&after);
      time = after.QuadPart - before.QuadPart;
      printf("%x staging unmap time: %.1fms\n", result, time*ticksToMS);


      QueryPerformanceCounter(&before);
      D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data;
      do {
	      result = context->GetData(query, &data, sizeof(data), 0);
      } while (result != 0);
      UINT64 time_before;
      UINT64 time_after;
            do {
	      result = context->GetData(time_after_query, &time_after, sizeof(time_after), 0);
      } while (result != 0);
      QueryPerformanceCounter(&after);
	    do {
	      result = context->GetData(time_before_query, &time_before, sizeof(time_before), 0);
      } while (result != 0);

      assert(!data.Disjoint);
      printf("copy time: %fms\n", (time_after-time_before)*1000./data.Frequency);
      time = after.QuadPart - before.QuadPart;
      printf("%x copy event time: %.1fms\n", result, time*ticksToMS);
      }

      Sleep(1000);
      printf("\n\n");

      {
      LARGE_INTEGER before;
      LARGE_INTEGER after;
      QueryPerformanceCounter(&before);

      D3D11_QUERY_DESC query_desc = {};
      query_desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
      ID3D11Query *query;
      device->CreateQuery(&query_desc, &query);

      D3D11_QUERY_DESC time_query_desc = {};
      time_query_desc.Query = D3D11_QUERY_TIMESTAMP;
      ID3D11Query *time_before_query;
      device->CreateQuery(&time_query_desc, &time_before_query);
      ID3D11Query *time_after_query;
      device->CreateQuery(&time_query_desc, &time_after_query);

      ID3D11Texture2D* texture6;
      result = device->CreateTexture2D(&desc, NULL, &texture6);

      context->Begin(query);
      context->Begin(time_before_query);
      context->End(time_before_query);
      QueryPerformanceCounter(&before);
      context->CopyResource(texture2, texture6);
      QueryPerformanceCounter(&after);
      time = after.QuadPart - before.QuadPart;
      context->Begin(time_after_query);
      context->End(time_after_query);
      context->End(query);
      printf("%x copy time: %.1fms\n", result, time*ticksToMS);

      QueryPerformanceCounter(&before);
      result = context->Map(staging_texture, 0, D3D11_MAP_READ_WRITE, 0, &map);
      QueryPerformanceCounter(&after);
      time = after.QuadPart - before.QuadPart;
      printf("%x staging map time: %.1fms\n", result, time*ticksToMS);

      QueryPerformanceCounter(&before);
      D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data;
      do {
	      result = context->GetData(query, &data, sizeof(data), 0);
      } while (result != 0);
      UINT64 time_before;
      UINT64 time_after;
            do {
	      result = context->GetData(time_after_query, &time_after, sizeof(time_after), 0);
      } while (result != 0);
      QueryPerformanceCounter(&after);
	    do {
	      result = context->GetData(time_before_query, &time_before, sizeof(time_before), 0);
      } while (result != 0);

      assert(!data.Disjoint);
      printf("copy time: %fms\n", (time_after-time_before)*1000./data.Frequency);
      time = after.QuadPart - before.QuadPart;
      printf("%x copy event time: %.1fms\n", result, time*ticksToMS);

    }

    }

}

