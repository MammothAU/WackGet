
#include "pch.h"
#include "main.h"

HANDLE heap;

void memory_init()
{
  HMODULE lib;

  heap= HeapCreate(HEAP_NO_SERIALIZE, 1024*1024, 0);

  /* Try to use low memory fragmentation on XP */
  if(lib= LoadLibrary("kernel32.dll")) {
    typedef BOOL (WINAPI *HeapSetInformation_t)(HANDLE HeapHandle,
                                                HEAP_INFORMATION_CLASS HeapInformationClass,
                                                PVOID HeapInformation,
                                                SIZE_T HeapInformationLength);
    HeapSetInformation_t hsi;

    if(hsi= (HeapSetInformation_t)GetProcAddress(lib, "HeapSetInformation")) {
      ULONG ul;

      ul= 2;
      hsi(heap, HeapCompatibilityInformation, &ul, sizeof(ul));
    }
    FreeLibrary(lib);
  }
}

void memory_cleanup()
{
  HeapDestroy(heap);
}
