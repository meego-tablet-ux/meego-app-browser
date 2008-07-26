// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
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

// This file implements PEImage, a generic class to manipulate PE files.
// This file was adapted from GreenBorder's Code.

#include "sandbox/src/pe_image.h"

namespace sandbox {

// Structure to perform imports enumerations.
struct EnumAllImportsStorage {
  sandbox::PEImage::EnumImportsFunction callback;
  PVOID cookie;
};

// Callback used to enumerate imports. See EnumImportChunksFunction.
bool ProcessImportChunk(const PEImage &image, LPCSTR module,
                        PIMAGE_THUNK_DATA name_table,
                        PIMAGE_THUNK_DATA iat, PVOID cookie) {
  EnumAllImportsStorage &storage = *reinterpret_cast<EnumAllImportsStorage*>(
                                       cookie);

  return image.EnumOneImportChunk(storage.callback, module, name_table, iat,
                                  storage.cookie);
}

// Callback used to enumerate delay imports. See EnumDelayImportChunksFunction.
bool ProcessDelayImportChunk(const PEImage &image,
                             PImgDelayDescr delay_descriptor,
                             LPCSTR module, PIMAGE_THUNK_DATA name_table,
                             PIMAGE_THUNK_DATA iat, PIMAGE_THUNK_DATA bound_iat,
                             PIMAGE_THUNK_DATA unload_iat, PVOID cookie) {
  EnumAllImportsStorage &storage = *reinterpret_cast<EnumAllImportsStorage*>(
                                       cookie);

  return image.EnumOneDelayImportChunk(storage.callback, delay_descriptor,
                                       module, name_table, iat, bound_iat,
                                       unload_iat, storage.cookie);
}

}  // namespace

namespace sandbox {

void PEImage::set_module(HMODULE module) {
  module_ = module;
}

PIMAGE_DOS_HEADER PEImage::GetDosHeader() const {
  return reinterpret_cast<PIMAGE_DOS_HEADER>(module_);
}

PIMAGE_NT_HEADERS PEImage::GetNTHeaders() const {
  PIMAGE_DOS_HEADER dos_header = GetDosHeader();

  return reinterpret_cast<PIMAGE_NT_HEADERS>(
      reinterpret_cast<char*>(dos_header) + dos_header->e_lfanew);
}

PIMAGE_SECTION_HEADER PEImage::GetSectionHeader(UINT section) const {
  PIMAGE_NT_HEADERS nt_headers = GetNTHeaders();
  PIMAGE_SECTION_HEADER first_section = IMAGE_FIRST_SECTION(nt_headers);

  if (section < nt_headers->FileHeader.NumberOfSections)
    return first_section + section;
  else
    return NULL;
}

WORD PEImage::GetNumSections() const {
  return GetNTHeaders()->FileHeader.NumberOfSections;
}

DWORD PEImage::GetImageDirectoryEntrySize(UINT directory) const {
  PIMAGE_NT_HEADERS nt_headers = GetNTHeaders();

  return nt_headers->OptionalHeader.DataDirectory[directory].Size;
}

PVOID PEImage::GetImageDirectoryEntryAddr(UINT directory) const {
  PIMAGE_NT_HEADERS nt_headers = GetNTHeaders();

  return RVAToAddr(
      nt_headers->OptionalHeader.DataDirectory[directory].VirtualAddress);
}

PIMAGE_SECTION_HEADER PEImage::GetImageSectionFromAddr(PVOID address) const {
  PBYTE target = reinterpret_cast<PBYTE>(address);
  PIMAGE_SECTION_HEADER section;

  for (UINT i = 0; NULL != (section = GetSectionHeader(i)); i++) {
    // Don't use the virtual RVAToAddr.
    PBYTE start = reinterpret_cast<PBYTE>(
                      PEImage::RVAToAddr(section->VirtualAddress));

    DWORD size = section->Misc.VirtualSize;

    if ((start <= target) && (start + size > target))
      return section;
  }

  return NULL;
}

PIMAGE_SECTION_HEADER PEImage::GetImageSectionHeaderByName(
    LPCSTR section_name) const {
  if (NULL == section_name)
    return NULL;

  PIMAGE_SECTION_HEADER ret = NULL;
  int num_sections = GetNumSections();

  for (int i = 0; i < num_sections; i++) {
    PIMAGE_SECTION_HEADER section = GetSectionHeader(i);
    if (0 == _strnicmp(reinterpret_cast<LPCSTR>(section->Name), section_name,
                       sizeof(section->Name))) {
      ret = section;
      break;
    }
  }

  return ret;
}

PDWORD PEImage::GetExportEntry(LPCSTR name) const {
  PIMAGE_EXPORT_DIRECTORY exports = GetExportDirectory();

  if (NULL == exports)
    return NULL;

  WORD ordinal = 0;
  if (!GetProcOrdinal(name, &ordinal))
    return NULL;

  PDWORD functions = reinterpret_cast<PDWORD>(
                         RVAToAddr(exports->AddressOfFunctions));

  return functions + ordinal - exports->Base;
}

FARPROC PEImage::GetProcAddress(LPCSTR function_name) const {
  PDWORD export_entry = GetExportEntry(function_name);
  if (NULL == export_entry)
    return NULL;

  PBYTE function = reinterpret_cast<PBYTE>(RVAToAddr(*export_entry));

  PBYTE exports = reinterpret_cast<PBYTE>(
      GetImageDirectoryEntryAddr(IMAGE_DIRECTORY_ENTRY_EXPORT));
  DWORD size = GetImageDirectoryEntrySize(IMAGE_DIRECTORY_ENTRY_EXPORT);

  // Check for forwarded exports as a special case.
  if (exports <= function && exports + size > function)
#pragma warning(push)
#pragma warning(disable: 4312)
    // This cast generates a warning because it is 32 bit specific.
    return reinterpret_cast<FARPROC>(0xFFFFFFFF);
#pragma warning(pop)

  return reinterpret_cast<FARPROC>(function);
}

bool PEImage::GetProcOrdinal(LPCSTR function_name, WORD *ordinal) const {
  if (NULL == ordinal)
    return false;

  PIMAGE_EXPORT_DIRECTORY exports = GetExportDirectory();

  if (NULL == exports)
    return false;

  if (IsOrdinal(function_name)) {
    *ordinal = ToOrdinal(function_name);
  } else {
    PDWORD names = reinterpret_cast<PDWORD>(RVAToAddr(exports->AddressOfNames));
    PDWORD lower = names;
    PDWORD upper = names + exports->NumberOfNames;
    int cmp = -1;

    // Binary Search for the name.
    while (lower != upper) {
      PDWORD middle = lower + (upper - lower) / 2;
      LPCSTR name = reinterpret_cast<LPCSTR>(RVAToAddr(*middle));

      cmp = strcmp(function_name, name);

      if (cmp == 0) {
        lower = middle;
        break;
      }

      if (cmp > 0)
        lower = middle + 1;
      else
        upper = middle;
    }

    if (cmp != 0)
      return false;


    PWORD ordinals = reinterpret_cast<PWORD>(
                         RVAToAddr(exports->AddressOfNameOrdinals));

    *ordinal = ordinals[lower - names] + static_cast<WORD>(exports->Base);
  }

  return true;
}

bool PEImage::EnumSections(EnumSectionsFunction callback, PVOID cookie) const {
  PIMAGE_NT_HEADERS nt_headers = GetNTHeaders();
  UINT num_sections = nt_headers->FileHeader.NumberOfSections;
  PIMAGE_SECTION_HEADER section = GetSectionHeader(0);

  for (UINT i = 0; i < num_sections; i++, section++) {
    PVOID section_start = RVAToAddr(section->VirtualAddress);
    DWORD size = section->Misc.VirtualSize;

    if (!callback(*this, section, section_start, size, cookie))
      return false;
  }

  return true;
}

bool PEImage::EnumExports(EnumExportsFunction callback, PVOID cookie) const {
  PVOID directory = GetImageDirectoryEntryAddr(IMAGE_DIRECTORY_ENTRY_EXPORT);
  DWORD size = GetImageDirectoryEntrySize(IMAGE_DIRECTORY_ENTRY_EXPORT);

  // Check if there are any exports at all.
  if (NULL == directory || 0 == size)
    return true;

  PIMAGE_EXPORT_DIRECTORY exports = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
                                        directory);
  UINT ordinal_base = exports->Base;
  UINT num_funcs = exports->NumberOfFunctions;
  UINT num_names = exports->NumberOfNames;
  PDWORD functions  = reinterpret_cast<PDWORD>(RVAToAddr(
                          exports->AddressOfFunctions));
  PDWORD names = reinterpret_cast<PDWORD>(RVAToAddr(exports->AddressOfNames));
  PWORD ordinals = reinterpret_cast<PWORD>(RVAToAddr(
                       exports->AddressOfNameOrdinals));

  for (UINT count = 0; count < num_funcs; count++) {
    PVOID func = RVAToAddr(functions[count]);
    if (NULL == func)
      continue;

    // Check for a name.
    LPCSTR name = NULL;
    UINT hint;
    for (hint = 0; hint < num_names; hint++) {
      if (ordinals[hint] == count) {
        name = reinterpret_cast<LPCSTR>(RVAToAddr(names[hint]));
        break;
      }
    }

    if (name == NULL)
      hint = 0;

    // Check for forwarded exports.
    LPCSTR forward = NULL;
    if (reinterpret_cast<char*>(func) >= reinterpret_cast<char*>(directory) &&
        reinterpret_cast<char*>(func) <= reinterpret_cast<char*>(directory) +
            size) {
      forward = reinterpret_cast<LPCSTR>(func);
      func = 0;
    }

    if (!callback(*this, ordinal_base + count, hint, name, func, forward,
                  cookie))
      return false;
  }

  return true;
}

bool PEImage::EnumRelocs(EnumRelocsFunction callback, PVOID cookie) const {
  PVOID directory = GetImageDirectoryEntryAddr(IMAGE_DIRECTORY_ENTRY_BASERELOC);
  DWORD size = GetImageDirectoryEntrySize(IMAGE_DIRECTORY_ENTRY_BASERELOC);
  PIMAGE_BASE_RELOCATION base = reinterpret_cast<PIMAGE_BASE_RELOCATION>(
      directory);

  if (directory == NULL || size < sizeof(IMAGE_BASE_RELOCATION))
    return true;

  while (base->SizeOfBlock) {
    PWORD reloc = reinterpret_cast<PWORD>(base + 1);
    UINT num_relocs = (base->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) /
        sizeof(WORD);

    for (UINT i = 0; i < num_relocs; i++, reloc++) {
      WORD type = *reloc >> 12;
      PVOID address = RVAToAddr(base->VirtualAddress + (*reloc & 0x0FFF));

      if (!callback(*this, type, address, cookie))
        return false;
    }

    base = reinterpret_cast<PIMAGE_BASE_RELOCATION>(
               reinterpret_cast<char*>(base) + base->SizeOfBlock);
  }

  return true;
}

bool PEImage::EnumImportChunks(EnumImportChunksFunction callback,
                               PVOID cookie) const {
  DWORD size = GetImageDirectoryEntrySize(IMAGE_DIRECTORY_ENTRY_IMPORT);
  PIMAGE_IMPORT_DESCRIPTOR import = GetFirstImportChunk();

  if (import == NULL || size < sizeof(IMAGE_IMPORT_DESCRIPTOR))
    return true;

  for (; import->FirstThunk; import++) {
    LPCSTR module_name = reinterpret_cast<LPCSTR>(RVAToAddr(import->Name));
    PIMAGE_THUNK_DATA name_table = reinterpret_cast<PIMAGE_THUNK_DATA>(
                                       RVAToAddr(import->OriginalFirstThunk));
    PIMAGE_THUNK_DATA iat = reinterpret_cast<PIMAGE_THUNK_DATA>(
                                RVAToAddr(import->FirstThunk));

    if (!callback(*this, module_name, name_table, iat, cookie))
      return false;
  }

  return true;
}

bool PEImage::EnumOneImportChunk(EnumImportsFunction callback,
                                 LPCSTR module_name,
                                 PIMAGE_THUNK_DATA name_table,
                                 PIMAGE_THUNK_DATA iat, PVOID cookie) const {
  if (NULL == name_table)
    return false;

  for (; name_table && name_table->u1.Ordinal; name_table++, iat++) {
    LPCSTR name = NULL;
    WORD ordinal = 0;
    WORD hint = 0;

    if (IMAGE_SNAP_BY_ORDINAL(name_table->u1.Ordinal)) {
      ordinal = static_cast<WORD>(IMAGE_ORDINAL32(name_table->u1.Ordinal));
    } else {
      PIMAGE_IMPORT_BY_NAME import = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
          RVAToAddr(name_table->u1.ForwarderString));

      hint = import->Hint;
      name = reinterpret_cast<LPCSTR>(&import->Name);
    }

    if (!callback(*this, module_name, ordinal, name, hint, iat, cookie))
      return false;
  }

  return true;
}

bool PEImage::EnumAllImports(EnumImportsFunction callback, PVOID cookie) const {
  EnumAllImportsStorage temp = { callback, cookie };
  return EnumImportChunks(ProcessImportChunk, &temp);
}

bool PEImage::EnumDelayImportChunks(EnumDelayImportChunksFunction callback,
                                    PVOID cookie) const {
  PVOID directory = GetImageDirectoryEntryAddr(
                        IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
  DWORD size = GetImageDirectoryEntrySize(IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
  PImgDelayDescr delay_descriptor = reinterpret_cast<PImgDelayDescr>(directory);

  if (directory == NULL || size == 0)
    return true;

  for (; delay_descriptor->rvaHmod; delay_descriptor++) {
    PIMAGE_THUNK_DATA name_table;
    PIMAGE_THUNK_DATA iat;
    PIMAGE_THUNK_DATA bound_iat;    // address of the optional bound IAT
    PIMAGE_THUNK_DATA unload_iat;   // address of optional copy of original IAT
    LPCSTR module_name;

    // check if VC7-style imports, using RVAs instead of
    // VC6-style addresses.
    bool rvas = (delay_descriptor->grAttrs & dlattrRva) != 0;

    if (rvas) {
      module_name = reinterpret_cast<LPCSTR>(
                        RVAToAddr(delay_descriptor->rvaDLLName));
      name_table = reinterpret_cast<PIMAGE_THUNK_DATA>(
                       RVAToAddr(delay_descriptor->rvaINT));
      iat = reinterpret_cast<PIMAGE_THUNK_DATA>(
                RVAToAddr(delay_descriptor->rvaIAT));
      bound_iat = reinterpret_cast<PIMAGE_THUNK_DATA>(
                      RVAToAddr(delay_descriptor->rvaBoundIAT));
      unload_iat = reinterpret_cast<PIMAGE_THUNK_DATA>(
                       RVAToAddr(delay_descriptor->rvaUnloadIAT));
    } else {
#pragma warning(push)
#pragma warning(disable: 4312)
      // These casts generate warnings because they are 32 bit specific.
      module_name = reinterpret_cast<LPCSTR>(delay_descriptor->rvaDLLName);
      name_table = reinterpret_cast<PIMAGE_THUNK_DATA>(
                       delay_descriptor->rvaINT);
      iat = reinterpret_cast<PIMAGE_THUNK_DATA>(delay_descriptor->rvaIAT);
      bound_iat = reinterpret_cast<PIMAGE_THUNK_DATA>(
                      delay_descriptor->rvaBoundIAT);
      unload_iat = reinterpret_cast<PIMAGE_THUNK_DATA>(
                       delay_descriptor->rvaUnloadIAT);
#pragma warning(pop)
    }

    if (!callback(*this, delay_descriptor, module_name, name_table, iat,
                  bound_iat, unload_iat, cookie))
      return false;
  }

  return true;
}

bool PEImage::EnumOneDelayImportChunk(EnumImportsFunction callback,
                                      PImgDelayDescr delay_descriptor,
                                      LPCSTR module_name,
                                      PIMAGE_THUNK_DATA name_table,
                                      PIMAGE_THUNK_DATA iat,
                                      PIMAGE_THUNK_DATA bound_iat,
                                      PIMAGE_THUNK_DATA unload_iat,
                                      PVOID cookie) const {
  UNREFERENCED_PARAMETER(bound_iat);
  UNREFERENCED_PARAMETER(unload_iat);

  for (; name_table->u1.Ordinal; name_table++, iat++) {
    LPCSTR name = NULL;
    WORD ordinal = 0;
    WORD hint = 0;

    if (IMAGE_SNAP_BY_ORDINAL(name_table->u1.Ordinal)) {
      ordinal = static_cast<WORD>(IMAGE_ORDINAL32(name_table->u1.Ordinal));
    } else {
      PIMAGE_IMPORT_BY_NAME import;
      bool rvas = (delay_descriptor->grAttrs & dlattrRva) != 0;

      if (rvas) {
        import = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
                     RVAToAddr(name_table->u1.ForwarderString));
      } else {
#pragma warning(push)
#pragma warning(disable: 4312)
        // This cast generates a warning because it is 32 bit specific.
        import = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
                     name_table->u1.ForwarderString);
#pragma warning(pop)
      }

      hint = import->Hint;
      name = reinterpret_cast<LPCSTR>(&import->Name);
    }

    if (!callback(*this, module_name, ordinal, name, hint, iat, cookie))
      return false;
  }

  return true;
}

bool PEImage::EnumAllDelayImports(EnumImportsFunction callback,
                                  PVOID cookie) const {
  EnumAllImportsStorage temp = { callback, cookie };
  return EnumDelayImportChunks(ProcessDelayImportChunk, &temp);
}

bool PEImage::VerifyMagic() const {
  PIMAGE_DOS_HEADER dos_header = GetDosHeader();

  if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
    return false;

  PIMAGE_NT_HEADERS nt_headers = GetNTHeaders();

  if (nt_headers->Signature != IMAGE_NT_SIGNATURE)
    return false;

  if (nt_headers->FileHeader.SizeOfOptionalHeader !=
      sizeof(IMAGE_OPTIONAL_HEADER))
    return false;

  if (nt_headers->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
    return false;

  return true;
}

bool PEImage::ImageRVAToOnDiskOffset(DWORD rva, DWORD *on_disk_offset) const {
  LPVOID address = RVAToAddr(rva);
  return ImageAddrToOnDiskOffset(address, on_disk_offset);
}

bool PEImage::ImageAddrToOnDiskOffset(LPVOID address,
                                      DWORD *on_disk_offset) const {
  if (NULL == address)
    return false;

  // Get the section that this address belongs to.
  PIMAGE_SECTION_HEADER section_header = GetImageSectionFromAddr(address);
  if (NULL == section_header)
    return false;

#pragma warning(push)
#pragma warning(disable: 4311)
  // These casts generate warnings because they are 32 bit specific.
  // Don't follow the virtual RVAToAddr, use the one on the base.
  DWORD offset_within_section = reinterpret_cast<DWORD>(address) -
                                    reinterpret_cast<DWORD>(PEImage::RVAToAddr(
                                        section_header->VirtualAddress));
#pragma warning(pop)

  *on_disk_offset = section_header->PointerToRawData + offset_within_section;
  return true;
}

PVOID PEImage::RVAToAddr(DWORD rva) const {
  if (rva == 0)
    return NULL;

  return reinterpret_cast<char*>(module_) + rva;
}

PVOID PEImageAsData::RVAToAddr(DWORD rva) const {
  if (rva == 0)
    return NULL;

  PVOID in_memory = PEImage::RVAToAddr(rva);
  DWORD dummy;

  if (!ImageAddrToOnDiskOffset(in_memory, &dummy))
    return NULL;

  return in_memory;
}

}  // namespace sandbox
