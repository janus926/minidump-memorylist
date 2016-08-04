// Copyright (c) 2010 The Mozilla Foundation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of The Mozilla Foundation nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/minidump.h"
#include "google_breakpad/processor/minidump_processor.h"
#include "google_breakpad/processor/system_info.h"
#include "processor/pathname_stripper.h"

using namespace google_breakpad;

void error(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);

  exit(1);
}

int main(int argc, char** argv)
{
  if (argc != 2) {
    error("Usage: %s <minidump>", argv[0]);
  }

  Minidump dump(argv[1]);
  if (!dump.Read()) {
    error("Couldn't read minidump %s", argv[1]);
  }

  MinidumpMemoryInfoList* memory_info_list = dump.GetMemoryInfoList();
  if (!memory_info_list->valid()) {
    error("Invalid MinidumpMemoryInfoList");
  }
//  printf("BaseAddress\tAllocationBase\tAllocationProtect\tRegionSize\tState\tProtect\tType\n");
#define MEM_COMMIT 0x1000
#define MEM_FREE   0x10000
#define MEM_PROTECT_WRITECOMBINE 0x400
  size_t sumFree = 0;
  size_t sumTiny = 0;
  size_t sumMisaligned = 0;
  size_t sumUsable = 0;
  size_t sumOther = 0;
  size_t sumWC = 0;
  for (int i = 0; i < memory_info_list->info_count(); ++i) {
    const MinidumpMemoryInfo* memory_info = memory_info_list->GetMemoryInfoAtIndex(i);
    if (!memory_info->valid()) {
//      printf("INVALID\n");
      continue;
    }
    const MDRawMemoryInfo* rawinfo = memory_info->info();
    if (rawinfo->state & MEM_FREE) {
      size_t size = rawinfo->region_size;
      sumFree += size;

      if (size < 0x100000) {
        sumTiny += size;
      } else if (size < 0x200000) {
        uint32_t base = (uint32_t)rawinfo->base_address;
        uint32_t nextMB = (base + 0xFFFFF) & ~0xFFFFF;
        uint32_t required = 0x100000 + (nextMB-base);
        if (size < required) {
          sumMisaligned += size;
        } else {
          sumUsable += size;
        }
      } else {
        sumOther += size;
      }
    }
    if (rawinfo->state == MEM_COMMIT &&
        rawinfo->protection & MEM_PROTECT_WRITECOMBINE) {
      sumWC += rawinfo->region_size;
    }

#if 0
    printf("%llx\t%llx\t%lx\t%llx\t%lx\t%lx\t%lx\n",
           rawinfo->base_address,
           rawinfo->allocation_base,
           rawinfo->allocation_protection,
           rawinfo->region_size,
           rawinfo->state,
           rawinfo->protection,
           rawinfo->type);
#endif
  }

  printf("Free=%luM,", sumFree >> 20);
  printf("Tiny=%luM,", sumTiny >> 20);
  printf("WriteCombine=%luM,", sumWC >> 20);
  printf("Misaligned=%luM,", sumMisaligned >> 20);
  printf("Usable=%luM,", sumUsable >> 20);
  printf("Other=%luM", sumOther >> 20);
  printf("\n");
  return 0;
}
