#ifndef _UEFI_H
#define _UEFI_H

struct EFI_GRAPHICS_OUTPUT_INFORMATION {
    unsigned int horizontalResolution;
    unsigned int verticalResolution;
    unsigned int pixelsPerScanLine;

    unsigned long frameBufferBase;
    unsigned long frameBufferSize;
};

struct EFI_E820_MEMORY_DESCRIPTOR {
    unsigned long address;
    unsigned long length;
    unsigned int  type;
}__attribute__((packed));

struct EFI_E820_MEMORY_DESCRIPTOR_INFORMATION {
    unsigned int E820_Entry_count;
    struct EFI_E820_MEMORY_DESCRIPTOR E820_Entry[0];
};

struct KERNEL_BOOT_PARAMETER_INFORMATION {
    struct EFI_GRAPHICS_OUTPUT_INFORMATION graphics_info;
    struct EFI_E820_MEMORY_DESCRIPTOR_INFORMATION E820_info;
};

extern struct KERNEL_BOOT_PARAMETER_INFORMATION *boot_param;

#endif

