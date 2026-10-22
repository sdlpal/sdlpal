/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2026, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// mtrr.c: MTRR Write-Combining (WC) setup for VGA and VESA LFB under DOS.
//         @Author: palxex, 2026
//
#include "mtrr.h"
#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <dpmi.h>
#include <sys/movedata.h>
#include <sys/segments.h>

#define MSR_IA32_MTRRCAP         0x00FE
#define MSR_IA32_MTRR_DEF_TYPE   0x02FF
#define MSR_IA32_MTRR_FIX64K_000 0x0250
#define MSR_IA32_MTRR_FIX16K_800 0x0258
#define MSR_IA32_MTRR_FIX16K_A00 0x0259
#define MSR_IA32_MTRR_PHYSBASE0  0x0200
#define MSR_IA32_MTRR_PHYSMASK0  0x0201

#define MTRR_TYPE_UC             0x00
#define MTRR_TYPE_WC             0x01
#define MTRR_TYPE_WT             0x04
#define MTRR_TYPE_WP             0x05
#define MTRR_TYPE_WB             0x06

#define MTRR_MASK_VALID          (1ULL << 11)

/* -------------------------------------------------------------------------- */
/* Logging helper                                                             */
/* -------------------------------------------------------------------------- */

/*
 * MTRR setup runs from UTIL_Platform_Startup, which happens before the
 * configuration file is read and therefore before the engine's log callback
 * is registered.  Routing these messages through UTIL_LogOutput would send
 * them nowhere.  Write a dedicated MTRR.LOG instead, and keep the same text
 * on the console so a screenshot taken during startup also shows it.
 *
 * The file is truncated once per run, not appended, so it always describes
 * the most recent startup.
 */
static FILE *g_mtrr_log_fp = NULL;

static void mtrr_log_open(const char *mode)
{
    g_mtrr_log_fp = fopen("MTRR.LOG", mode);
}

static void mtrr_log_close(void)
{
    if (g_mtrr_log_fp != NULL) {
        fclose(g_mtrr_log_fp);
        g_mtrr_log_fp = NULL;
    }
}

static void mtrr_log(const char *fmt, ...)
{
    char buf[512];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (g_mtrr_log_fp != NULL) {
        fprintf(g_mtrr_log_fp, "[MTRR] %s\n", buf);
        fflush(g_mtrr_log_fp);
    }
}

static const char *mtrr_type_name(uint8_t type)
{
    switch (type) {
        case MTRR_TYPE_UC: return "UC (Uncacheable)";
        case MTRR_TYPE_WC: return "WC (Write-Combining)";
        case MTRR_TYPE_WT: return "WT (Write-Through)";
        case MTRR_TYPE_WP: return "WP (Write-Protected)";
        case MTRR_TYPE_WB: return "WB (Write-Back)";
        default:           return "Unknown";
    }
}

/* -------------------------------------------------------------------------- */
/* CPUID and CPU feature checks                                               */
/* -------------------------------------------------------------------------- */

static bool cpu_has_cpuid(void)
{
    uint32_t flags_orig, flags_mod;
    __asm__ volatile (
        "pushfl\n\t"
        "popl %0\n\t"
        "movl %0, %1\n\t"
        "xorl $0x200000, %1\n\t"
        "pushl %1\n\t"
        "popfl\n\t"
        "pushfl\n\t"
        "popl %1\n\t"
        "pushl %0\n\t"
        "popfl\n\t"
        : "=r"(flags_orig), "=r"(flags_mod)
    );
    return ((flags_orig ^ flags_mod) & 0x200000) != 0;
}

static void cpuid(uint32_t eax_in, uint32_t ecx_in, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    uint32_t a, b, c, d;
    __asm__ volatile (
        "cpuid"
        : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
        : "a"(eax_in), "c"(ecx_in)
    );
    if (eax) *eax = a;
    if (ebx) *ebx = b;
    if (ecx) *ecx = c;
    if (edx) *edx = d;
}

static uint64_t get_physical_address_mask(void)
{
    uint32_t max_ext = 0;
    cpuid(0x80000000, 0, &max_ext, NULL, NULL, NULL);
    uint32_t phys_bits = 36; // Default standard Intel P6 / Pentium Pro+ physical address width
    if (max_ext >= 0x80000008) {
        uint32_t a = 0;
        cpuid(0x80000008, 0, &a, NULL, NULL, NULL);
        phys_bits = a & 0xFF;
        if (phys_bits < 32 || phys_bits > 52) {
            phys_bits = 36;
        }
    }
    uint64_t mask = (phys_bits >= 64) ? ~0ULL : ((1ULL << phys_bits) - 1ULL);
    return mask & ~0xFFFULL;
}

/* -------------------------------------------------------------------------- */
/* Ring 0 execution primitives (Direct vs Call Gate)                          */
/* -------------------------------------------------------------------------- */

static inline int get_cpl(void)
{
    uint16_t cs;
    __asm__ volatile ("mov %%cs, %0" : "=r"(cs));
    return cs & 3;
}

// Low-level Ring 0 thunks ending with lret (32-bit far return)
__asm__(
    ".text\n"
    ".globl _ring0_thunk_rdmsr\n"
    "_ring0_thunk_rdmsr:\n"
    "    rdmsr\n"
    "    lret\n"
    "\n"
    ".globl _ring0_thunk_wrmsr\n"
    "_ring0_thunk_wrmsr:\n"
    "    wrmsr\n"
    "    lret\n"
    "\n"
    ".globl _ring0_thunk_disable_cache\n"
    "_ring0_thunk_disable_cache:\n"
    "    mov %cr0, %eax\n"
    "    or  $0x40000000, %eax\n"
    "    mov %eax, %cr0\n"
    "    wbinvd\n"
    "    mov %cr3, %eax\n"
    "    mov %eax, %cr3\n"
    "    lret\n"
    "\n"
    ".globl _ring0_thunk_enable_cache\n"
    "_ring0_thunk_enable_cache:\n"
    "    wbinvd\n"
    "    mov %cr3, %eax\n"
    "    mov %eax, %cr3\n"
    "    mov %cr0, %eax\n"
    "    and $0xbfffffff, %eax\n"
    "    mov %eax, %cr0\n"
    "    lret\n"
);

extern void ring0_thunk_rdmsr(void);
extern void ring0_thunk_wrmsr(void);
extern void ring0_thunk_disable_cache(void);
extern void ring0_thunk_enable_cache(void);

#pragma pack(push, 1)
struct gdt_register {
    uint16_t limit;
    uint32_t base;
};
#pragma pack(pop)

static int g_sel_gdt  = -1;
static int g_sel_ldt  = -1;
static int g_sel_cs0  = -1;
static int g_sel_gate = -1;
static bool g_callgate_active = false;

/*
 * MTRR state captured before the first write, so it can be put back when the
 * program ends.  Without this the machine is left reprogrammed: the VGA
 * window stays WC, extra variable entries stay allocated, and any range we
 * disabled because it overlapped the framebuffer stays disabled for whatever
 * runs next.
 */
#define MTRR_MAX_SAVED_VAR 16

/*
 * Everything the setup can change is snapshotted before the first write and
 * written back on exit, so the machine is left exactly as it was found:
 * the fixed 16K entry covering the VGA window, and every variable pair.
 * Saving the whole set rather than tracking individual edits also restores
 * the PHYSBASE registers of entries that get invalidated for overlapping
 * the framebuffer, which a mask-only restore would leave behind.
 */
static bool     g_state_saved = false;
static uint64_t g_saved_fix_a0000;
static uint32_t g_saved_vcnt;
static uint64_t g_saved_physbase[MTRR_MAX_SAVED_VAR];
static uint64_t g_saved_physmask[MTRR_MAX_SAVED_VAR];

static bool setup_callgate(void)
{
    if (g_callgate_active) return true;

    // Check if Windows NT/2k/XP is detected
    char *os_env = getenv("OS");
    if (os_env && strstr(os_env, "Windows_NT")) {
        mtrr_log("Cannot modify descriptors under Windows NT/2k/XP NTVDM.");
        return false;
    }

    mtrr_log("callgate: reading GDTR/LDTR.");
    struct gdt_register gdtr;
    uint16_t ldtr = 0;
    __asm__ volatile ("sgdt %0" : "=m"(gdtr));
    __asm__ volatile ("sldt %0" : "=m"(ldtr));
    mtrr_log("callgate: GDTR base 0x%lX limit 0x%X, LDTR 0x%X.",
             (unsigned long)gdtr.base, (unsigned)gdtr.limit, (unsigned)ldtr);

    if (ldtr == 0) {
        mtrr_log("SLDT returned 0: no active LDT in current DPMI environment.");
        return false;
    }

    // Allocate descriptor for GDT alias
    mtrr_log("callgate: allocating GDT alias selector.");
    g_sel_gdt = __dpmi_allocate_ldt_descriptors(1);
    if (g_sel_gdt < 0) {
        mtrr_log("Failed to allocate GDT alias selector.");
        return false;
    }
    mtrr_log("callgate: setting GDT alias base/limit.");
    __dpmi_set_segment_base_address(g_sel_gdt, gdtr.base);
    __dpmi_set_segment_limit(g_sel_gdt, gdtr.limit);

    // Read LDT descriptor from GDT
    mtrr_log("callgate: reading the LDT descriptor through the alias.");
    uint8_t ldt_desc[8];
    uint32_t ldt_index = ldtr & ~7;
    movedata(g_sel_gdt, ldt_index, _my_ds(), (unsigned)ldt_desc, 8);

    uint32_t ldt_base = ldt_desc[2] | (ldt_desc[3] << 8) | (ldt_desc[4] << 16) | (ldt_desc[7] << 24);
    uint32_t ldt_limit = ldt_desc[0] | (ldt_desc[1] << 8) | ((ldt_desc[6] & 0x0F) << 16);
    if (ldt_desc[6] & 0x80) {
        ldt_limit = (ldt_limit << 12) | 0xFFF;
    }

    // Allocate descriptor for LDT alias
    mtrr_log("callgate: LDT base 0x%lX limit 0x%lX; allocating alias.",
             (unsigned long)ldt_base, (unsigned long)ldt_limit);
    g_sel_ldt = __dpmi_allocate_ldt_descriptors(1);
    if (g_sel_ldt < 0) {
        mtrr_log("Failed to allocate LDT alias selector.");
        __dpmi_free_ldt_descriptor(g_sel_gdt);
        g_sel_gdt = -1;
        return false;
    }
    mtrr_log("callgate: setting LDT alias base/limit.");
    __dpmi_set_segment_base_address(g_sel_ldt, ldt_base);
    __dpmi_set_segment_limit(g_sel_ldt, ldt_limit);

    // Create Ring 0 code selector from current CS descriptor
    mtrr_log("callgate: allocating the ring-0 code selector.");
    g_sel_cs0 = __dpmi_allocate_ldt_descriptors(1);
    if (g_sel_cs0 < 0) {
        mtrr_log("Failed to allocate Ring 0 CS selector.");
        __dpmi_free_ldt_descriptor(g_sel_ldt); g_sel_ldt = -1;
        __dpmi_free_ldt_descriptor(g_sel_gdt); g_sel_gdt = -1;
        return false;
    }
    uint8_t cs_desc[8];
    uint16_t current_cs = _my_cs();
    mtrr_log("callgate: reading the CS descriptor for selector 0x%X.", (unsigned)current_cs);
    movedata(g_sel_ldt, current_cs & ~7, _my_ds(), (unsigned)cs_desc, 8);
    // Set DPL=0 and Type=0xA (Code, Execute/Read)
    cs_desc[5] = (cs_desc[5] & 0x90) | 0x0A;
    mtrr_log("callgate: installing the ring-0 CS clone.");
    movedata(_my_ds(), (unsigned)cs_desc, g_sel_ldt, g_sel_cs0 & ~7, 8);

    // Create 32-bit Call Gate in LDT with DPL=3 pointing to Ring 0 CS
    mtrr_log("callgate: allocating the gate selector.");
    g_sel_gate = __dpmi_allocate_ldt_descriptors(1);
    if (g_sel_gate < 0) {
        mtrr_log("Failed to allocate Call Gate selector.");
        __dpmi_free_ldt_descriptor(g_sel_cs0); g_sel_cs0 = -1;
        __dpmi_free_ldt_descriptor(g_sel_ldt); g_sel_ldt = -1;
        __dpmi_free_ldt_descriptor(g_sel_gdt); g_sel_gdt = -1;
        return false;
    }
    uint8_t gate_desc[8];
    memset(gate_desc, 0, sizeof(gate_desc));
    gate_desc[2] = g_sel_cs0 & 0xFF;
    gate_desc[3] = (g_sel_cs0 >> 8) & 0xFF;
    gate_desc[4] = 0; // 0 parameters
    gate_desc[5] = 0xEC; // Present(0x80) | DPL=3(0x60) | 32-bit Call Gate(0x0C)
    mtrr_log("callgate: installing the call gate.");
    movedata(_my_ds(), (unsigned)gate_desc, g_sel_ldt, g_sel_gate & ~7, 8);

    mtrr_log("callgate: ready (cs0 0x%X, gate 0x%X).",
             (unsigned)g_sel_cs0, (unsigned)g_sel_gate);
    g_callgate_active = true;
    return true;
}

static void teardown_callgate(void)
{
    if (g_sel_gate != -1) { __dpmi_free_ldt_descriptor(g_sel_gate); g_sel_gate = -1; }
    if (g_sel_cs0 != -1)  { __dpmi_free_ldt_descriptor(g_sel_cs0);  g_sel_cs0 = -1; }
    if (g_sel_ldt != -1)  { __dpmi_free_ldt_descriptor(g_sel_ldt);  g_sel_ldt = -1; }
    if (g_sel_gdt != -1)  { __dpmi_free_ldt_descriptor(g_sel_gdt);  g_sel_gdt = -1; }
    g_callgate_active = false;
}

static void set_callgate_target(uint32_t func_ptr)
{
    uint16_t off_low  = func_ptr & 0xFFFF;
    uint16_t off_high = (func_ptr >> 16) & 0xFFFF;
    movedata(_my_ds(), (unsigned)&off_low,  g_sel_ldt, (g_sel_gate & ~7) + 0, 2);
    movedata(_my_ds(), (unsigned)&off_high, g_sel_ldt, (g_sel_gate & ~7) + 6, 2);
}

// Privilege-abstracted RDMSR / WRMSR / Cache operations
static uint64_t mtrr_rdmsr(uint32_t msr)
{
    if (get_cpl() == 0) {
        uint32_t lo, hi;
        __asm__ volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
        return ((uint64_t)hi << 32) | lo;
    }

    set_callgate_target((uint32_t)ring0_thunk_rdmsr);
    struct {
        uint32_t offset;
        uint16_t selector;
    } __attribute__((packed)) gate = { 0, (uint16_t)g_sel_gate };

    uint32_t lo, hi;
    __asm__ volatile (
        "cli\n\t"
        "lcall *(%2)\n\t"
        "sti\n\t"
        : "=a"(lo), "=d"(hi)
        : "r"(&gate), "c"(msr)
        : "memory"
    );
    return ((uint64_t)hi << 32) | lo;
}

static void mtrr_wrmsr(uint32_t msr, uint64_t val)
{
    uint32_t lo = (uint32_t)val;
    uint32_t hi = (uint32_t)(val >> 32);

    if (get_cpl() == 0) {
        __asm__ volatile ("wrmsr" : : "c"(msr), "a"(lo), "d"(hi));
        return;
    }

    set_callgate_target((uint32_t)ring0_thunk_wrmsr);
    struct {
        uint32_t offset;
        uint16_t selector;
    } __attribute__((packed)) gate = { 0, (uint16_t)g_sel_gate };

    __asm__ volatile (
        "cli\n\t"
        "lcall *(%2)\n\t"
        "sti\n\t"
        :
        : "a"(lo), "d"(hi), "r"(&gate), "c"(msr)
        : "memory"
    );
}

static void mtrr_disable_cache(void)
{
    if (get_cpl() == 0) {
        __asm__ volatile (
            "mov %%cr0, %%eax\n\t"
            "or $0x40000000, %%eax\n\t"
            "mov %%eax, %%cr0\n\t"
            "wbinvd\n\t"
            "mov %%cr3, %%eax\n\t"
            "mov %%eax, %%cr3\n\t"
            : : : "eax", "memory"
        );
        return;
    }

    set_callgate_target((uint32_t)ring0_thunk_disable_cache);
    struct {
        uint32_t offset;
        uint16_t selector;
    } __attribute__((packed)) gate = { 0, (uint16_t)g_sel_gate };

    __asm__ volatile (
        "cli\n\t"
        "lcall *(%0)\n\t"
        "sti\n\t"
        :
        : "r"(&gate)
        : "eax", "memory"
    );
}

static void mtrr_enable_cache(void)
{
    if (get_cpl() == 0) {
        __asm__ volatile (
            "wbinvd\n\t"
            "mov %%cr3, %%eax\n\t"
            "mov %%eax, %%cr3\n\t"
            "mov %%cr0, %%eax\n\t"
            "and $0xbfffffff, %%eax\n\t"
            "mov %%eax, %%cr0\n\t"
            : : : "eax", "memory"
        );
        return;
    }

    set_callgate_target((uint32_t)ring0_thunk_enable_cache);
    struct {
        uint32_t offset;
        uint16_t selector;
    } __attribute__((packed)) gate = { 0, (uint16_t)g_sel_gate };

    __asm__ volatile (
        "cli\n\t"
        "lcall *(%0)\n\t"
        "sti\n\t"
        :
        : "r"(&gate)
        : "eax", "memory"
    );
}

/* -------------------------------------------------------------------------- */
/* Cache-safe MTRR update window                                              */
/* -------------------------------------------------------------------------- */

/*
 * The original mtrrlfbe does exactly two things around an MTRR update: turn
 * caching off, write the registers, turn caching back on.  It never touches
 * IA32_MTRR_DEF_TYPE (0x2FF) -- searching the whole image for that immediate
 * finds zero occurrences -- and it never keeps interrupts disabled across a
 * sequence of MSR writes.
 *
 * Our earlier version did both: it wrapped the update in cli/sti and cleared
 * the global MTRR enable bit.  That was not merely redundant, it was broken,
 * because every individual MSR access also does its own cli/lcall/sti.  The
 * inner sti re-enabled interrupts while the outer section believed they were
 * still off, so the writes ran with interrupts live and caching disabled
 * while the MTRRs were half reprogrammed.  With a live timer IRQ that
 * corrupts video memory, which is what the black screen was.
 *
 * Matching the original removes the problem at the root: one cache-off
 * around each batch of writes, no 0x2FF traffic, and the interrupt state
 * left exactly as we found it.
 */
static void mtrr_begin_update(void)
{
    mtrr_disable_cache();
}

static void mtrr_end_update(void)
{
    mtrr_enable_cache();
}

/* -------------------------------------------------------------------------- */
/* VESA VBE Detection for LFB Address and Total VRAM Size                     */
/* -------------------------------------------------------------------------- */

#pragma pack(push, 1)
typedef struct {
    char     VESASignature[4];    /* "VESA" or "VBE2" */
    uint16_t VESAVersion;         /* 0x0200, 0x0300, etc. */
    uint32_t OEMStringPtr;
    uint32_t Capabilities;
    uint32_t VideoModePtr;        /* Real mode far pointer to 16-bit mode array */
    uint16_t TotalMemory;         /* 64KB memory blocks */
    uint16_t OEMSoftwareRev;
    uint32_t OEMVendorNamePtr;
    uint32_t OEMProductNamePtr;
    uint32_t OEMProductRevPtr;
    uint8_t  Reserved[222];
    uint8_t  OemData[256];
} VBE_InfoBlock;

typedef struct {
    uint16_t ModeAttributes;      /* Bit 7 = LFB supported */
    uint8_t  WinAAttributes;
    uint8_t  WinBAttributes;
    uint16_t WinGranularity;
    uint16_t WinSize;
    uint16_t WinASegment;
    uint16_t WinBSegment;
    uint32_t WinFuncPtr;
    uint16_t BytesPerScanLine;
    uint16_t XResolution;
    uint16_t YResolution;
    uint8_t  XCharSize;
    uint8_t  YCharSize;
    uint8_t  NumberOfPlanes;
    uint8_t  BitsPerPixel;
    uint8_t  NumberOfBanks;
    uint8_t  MemoryModel;
    uint8_t  BankSize;
    uint8_t  NumberOfImagePages;
    uint8_t  Reserved_page;
    uint8_t  RedMaskSize;
    uint8_t  RedMaskPos;
    uint8_t  GreenMaskSize;
    uint8_t  GreenMaskPos;
    uint8_t  BlueMaskSize;
    uint8_t  BlueMaskPos;
    uint8_t  ReservedMaskSize;
    uint8_t  ReservedMaskPos;
    uint8_t  DirectColorModeInfo;
    uint32_t PhysBasePtr;         /* Physical address of Linear Frame Buffer */
    uint32_t OffScreenMemOffset;
    uint16_t OffScreenMemSize;
    uint8_t  Reserved[206];
} VBE_ModeInfoBlock;
#pragma pack(pop)

/*
 * Query the VBE BIOS for a linear framebuffer mode.
 *
 * This must run BEFORE any display mode is set.  Issuing a VBE query while a
 * mode is active makes some BIOSes blank the display -- that is exactly the
 * black screen that showed up when this ran from UTIL_Platform_Init, i.e.
 * after SDL_Init had already switched the video mode.  Called from
 * UTIL_Platform_Startup instead, the situation matches the original mtrrlfbe
 * tool (which queries VESA while still in text mode) and is safe.
 */
static bool detect_vesa_lfb(uint32_t *out_lfb_base, uint32_t *out_lfb_size)
{
    *out_lfb_base = 0;
    *out_lfb_size = 0;

    /*
     * Room for BOTH blocks: the VBE info block at offset 0 and the mode info
     * block at offset sizeof(VBE_InfoBlock).  Allocating only one block's
     * worth (32 paragraphs) makes the BIOS write straight past the end of the
     * DOS allocation, which corrupts the memory control block chain and
     * halts the machine with "MCB chain corrupted".
     */
    const unsigned blocks_bytes = (unsigned)(sizeof(VBE_InfoBlock) + sizeof(VBE_ModeInfoBlock));
    // round up, then keep an extra 16 paragraphs: some BIOSes scribble a
    // little past the mode info block, and overrunning a DOS allocation
    // corrupts the memory control block chain.
    const int blocks_paras = (int)((blocks_bytes + 15u) / 16u) + 16;

    int dos_sel;
    mtrr_log("vesa: allocating %d paragraphs of conventional memory.", blocks_paras);
    int dos_seg = __dpmi_allocate_dos_memory(blocks_paras, &dos_sel);
    if (dos_seg == -1) {
        mtrr_log("Failed to allocate DOS conventional memory for VBE query.");
        return false;
    }

    VBE_InfoBlock vbe;
    memset(&vbe, 0, sizeof(vbe));
    memcpy(vbe.VESASignature, "VBE2", 4);
    dosmemput(&vbe, sizeof(vbe), dos_seg * 16);

    __dpmi_regs r;
    memset(&r, 0, sizeof(r));
    r.x.ax = 0x4F00;
    r.x.es = (uint16_t)dos_seg;
    r.x.di = 0;
    mtrr_log("vesa: calling int 10h/4F00.");
    __dpmi_int(0x10, &r);
    mtrr_log("vesa: 4F00 returned AX=0x%04X.", r.x.ax);

    if (r.x.ax != 0x004F) {
        mtrr_log("VBE 0x4F00 call failed (AX=0x%04X).", r.x.ax);
        __dpmi_free_dos_memory(dos_sel);
        return false;
    }

    dosmemget(dos_seg * 16, sizeof(vbe), &vbe);
    if (memcmp(vbe.VESASignature, "VESA", 4) != 0) {
        mtrr_log("VBE signature is not 'VESA'.");
        __dpmi_free_dos_memory(dos_sel);
        return false;
    }

    uint32_t total_vram = (uint32_t)vbe.TotalMemory * 65536;
    mtrr_log("VESA VBE %d.%d detected, Total VRAM: %u KB (%u MB).",
             vbe.VESAVersion >> 8, vbe.VESAVersion & 0xFF,
             total_vram / 1024, total_vram / (1024 * 1024));

    uint16_t mode_seg = (uint16_t)(vbe.VideoModePtr >> 16);
    uint16_t mode_off = (uint16_t)(vbe.VideoModePtr & 0xFFFF);
    uint32_t mode_ptr_linear = ((uint32_t)mode_seg << 4) + mode_off;

    uint32_t found_lfb = 0;
    for (int i = 0; i < 256; i++) {
        uint16_t mode_id = 0;
        dosmemget(mode_ptr_linear + i * 2, 2, &mode_id);
        if (mode_id == 0xFFFF) break;

        memset(&r, 0, sizeof(r));
        r.x.ax = 0x4F01;
        r.x.cx = mode_id;
        r.x.es = (uint16_t)dos_seg;
        r.x.di = sizeof(VBE_InfoBlock);
        __dpmi_int(0x10, &r);

        if (r.x.ax == 0x004F) {
            VBE_ModeInfoBlock minfo;
            dosmemget(dos_seg * 16 + sizeof(VBE_InfoBlock), sizeof(minfo), &minfo);
            if ((minfo.ModeAttributes & 0x80) && minfo.PhysBasePtr != 0) {
                found_lfb = minfo.PhysBasePtr;
                break;
            }
        }
    }

    __dpmi_free_dos_memory(dos_sel);

    if (found_lfb != 0) {
        *out_lfb_base = found_lfb;
        *out_lfb_size = total_vram;
        mtrr_log("VESA LFB physical address: 0x%08lX, size: %lu MB.",
                 (unsigned long)found_lfb, (unsigned long)(total_vram / (1024 * 1024)));
        return true;
    }

    mtrr_log("No VESA Linear Frame Buffer (LFB) mode found.");
    return false;
}

/* -------------------------------------------------------------------------- */
/* MTRR Query and Inspection                                                  */
/* -------------------------------------------------------------------------- */

static void dump_all_mtrrs(const char *label, uint32_t vcnt, bool has_fix, uint64_t phys_mask)
{
    mtrr_log("--- %s ---", label);

    uint64_t deftype = mtrr_rdmsr(MSR_IA32_MTRR_DEF_TYPE);
    mtrr_log("MTRRdefType: 0x%016llX (MTRR Enabled: %s, Fixed Enabled: %s, Default: %s)",
             deftype,
             (deftype & (1ULL << 11)) ? "YES" : "NO",
             (deftype & (1ULL << 10)) ? "YES" : "NO",
             mtrr_type_name(deftype & 0xFF));

    if (has_fix) {
        uint64_t fix_a0 = mtrr_rdmsr(MSR_IA32_MTRR_FIX16K_A00);
        uint8_t vga_t0 = (uint8_t)(fix_a0 & 0xFF);
        uint8_t vga_t1 = (uint8_t)((fix_a0 >> 8) & 0xFF);
        uint8_t vga_t2 = (uint8_t)((fix_a0 >> 16) & 0xFF);
        uint8_t vga_t3 = (uint8_t)((fix_a0 >> 24) & 0xFF);
        uint8_t mda_t0 = (uint8_t)((fix_a0 >> 32) & 0xFF);
        uint8_t mda_t1 = (uint8_t)((fix_a0 >> 40) & 0xFF);
        uint8_t mda_t2 = (uint8_t)((fix_a0 >> 48) & 0xFF);
        uint8_t mda_t3 = (uint8_t)((fix_a0 >> 56) & 0xFF);

        mtrr_log("Fixed MTRR 0x259 (A0000h-BFFFFh):");
        mtrr_log("  VGA buffer  [A0000h-AFFFFh] (64KB): %02X %02X %02X %02X -> %s",
                 vga_t0, vga_t1, vga_t2, vga_t3,
                 (vga_t0 == vga_t1 && vga_t1 == vga_t2 && vga_t2 == vga_t3) ? mtrr_type_name(vga_t0) : "Mixed");
        mtrr_log("  MDA/CGA mem [B0000h-BFFFFh] (64KB): %02X %02X %02X %02X -> %s",
                 mda_t0, mda_t1, mda_t2, mda_t3,
                 (mda_t0 == mda_t1 && mda_t1 == mda_t2 && mda_t2 == mda_t3) ? mtrr_type_name(mda_t0) : "Mixed");
        mtrr_log("  (Type codes: 00=UC, 01=WC, 04=WT, 05=WP, 06=WB)");
    }

    for (uint32_t i = 0; i < vcnt; i++) {
        // Reading these is the last place a host can hang us; a marker per
        // iteration would flood the log, so only the first one is noted and
        // the loop is bounded by vcnt from MTRRCAP.
        if (i == 0) {
            mtrr_log("dump: reading %u variable MTRR pair(s).", (unsigned)vcnt);
        }
        uint64_t base_msr = mtrr_rdmsr(MSR_IA32_MTRR_PHYSBASE0 + 2 * i);
        uint64_t mask_msr = mtrr_rdmsr(MSR_IA32_MTRR_PHYSMASK0 + 2 * i);

        if (mask_msr & MTRR_MASK_VALID) {
            uint64_t base = base_msr & phys_mask;
            uint8_t  type = base_msr & 0xFF;
            uint64_t mask = mask_msr & phys_mask;
            uint64_t size = ((~mask) & phys_mask) + 0x1000;
            uint64_t end  = base + size - 1;

            if (size >= 1024 * 1024) {
                mtrr_log("MTRR #%u: base = 0x%09llX, mask = 0x%09llX [%08llX - %08llX] (%5llu MB), %s [USED]",
                         i, base, mask, base, end, size / (1024 * 1024), mtrr_type_name(type));
            } else {
                mtrr_log("MTRR #%u: base = 0x%09llX, mask = 0x%09llX [%08llX - %08llX] (%5llu KB), %s [USED]",
                         i, base, mask, base, end, size / 1024, mtrr_type_name(type));
            }
        } else {
            mtrr_log("MTRR #%u: [UNUSED]", i);
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Setting VGA to WC via Fixed MTRR                                           */
/* -------------------------------------------------------------------------- */

static void set_vga_wc_fixed(void)
{
    uint64_t val = mtrr_rdmsr(MSR_IA32_MTRR_FIX16K_A00);
    uint8_t orig_type = (uint8_t)(val & 0xFF);

    // Bytes 0..3 cover 0xA0000 - 0xAFFFF (4 * 16KB = 64KB)
    // Set each of these 4 bytes to 0x01 (WC)
    val = (val & 0xFFFFFFFF00000000ULL) | 0x01010101ULL;

    mtrr_begin_update();
    mtrr_wrmsr(MSR_IA32_MTRR_FIX16K_A00, val);
    mtrr_end_update();

    mtrr_log("VGA buffer (0xA0000 - 0xAFFFF, 64KB) set to WC via Fixed MTRR 0x259 (was %s).",
             mtrr_type_name(orig_type));
}

/* -------------------------------------------------------------------------- */
/* Decomposing and Programming Variable MTRRs with Prioritization             */
/* -------------------------------------------------------------------------- */

struct mtrr_chunk {
    uint64_t base;
    uint64_t size;
};

// Decompose [base, base + size) into naturally-aligned power-of-2 chunks
static int decompose_range(uint64_t base, uint64_t size, struct mtrr_chunk *out_chunks, int max_chunks)
{
    int count = 0;
    uint64_t cur_base = base;
    uint64_t rem_size = size;

    while (rem_size > 0 && count < max_chunks) {
        // Find largest power of 2 <= rem_size that divides cur_base
        uint64_t chunk = 1ULL << 31; // Start from 2GB
        while (chunk > rem_size || (cur_base & (chunk - 1)) != 0) {
            chunk >>= 1;
        }
        if (chunk < 4096) break;

        out_chunks[count].base = cur_base;
        out_chunks[count].size = chunk;
        count++;

        cur_base += chunk;
        rem_size -= chunk;
    }
    return count;
}

static void set_lfb_wc_variable(uint32_t lfb_base, uint32_t lfb_size, uint32_t vcnt, uint64_t phys_mask)
{
    if (lfb_base == 0 || lfb_size == 0) return;

    struct mtrr_chunk chunks[16];
    int num_chunks = decompose_range(lfb_base, lfb_size, chunks, 16);

    mtrr_log("LFB range [0x%08lX - 0x%08lX] requires %d power-of-2 MTRR chunk(s):",
             (unsigned long)lfb_base, (unsigned long)(lfb_base + lfb_size - 1), num_chunks);
    for (int i = 0; i < num_chunks; i++) {
        mtrr_log("  Chunk %d: 0x%08llX - 0x%08llX (%llu MB)",
                 i, chunks[i].base, chunks[i].base + chunks[i].size - 1, chunks[i].size / (1024 * 1024));
    }

    /*
     * Which chunks are already satisfied by an existing WC range?
     *
     * The original looks for an MTRR whose base matches the range it is
     * about to program and, when it finds one, reuses that slot instead of
     * consuming a free one (0x3942..0x3975 in the disassembly).  Our old
     * code logged "already covers" and then programmed a duplicate range
     * into a fresh slot anyway, which wastes a variable MTRR and leaves two
     * identical entries behind.
     *
     * Resolve it per chunk instead of per range, so a partially covered LFB
     * only programs the parts that are actually missing.
     */
    bool chunk_done[16];
    for (int k = 0; k < num_chunks; k++) {
        chunk_done[k] = false;
    }

    for (uint32_t i = 0; i < vcnt; i++) {
        uint64_t base_msr = mtrr_rdmsr(MSR_IA32_MTRR_PHYSBASE0 + 2 * i);
        uint64_t mask_msr = mtrr_rdmsr(MSR_IA32_MTRR_PHYSMASK0 + 2 * i);

        if (!(mask_msr & MTRR_MASK_VALID)) {
            continue;
        }

        uint64_t base = base_msr & phys_mask;
        uint64_t size = ((~mask_msr) & phys_mask) + 0x1000;
        uint64_t end  = base + size - 1;
        uint8_t  type = base_msr & 0xFF;

        uint64_t lfb_end = (uint64_t)lfb_base + lfb_size - 1;
        if (end < lfb_base || base > lfb_end) {
            continue;                       // no overlap at all
        }

        // mark every chunk this entry fully covers with the type we want
        for (int k = 0; k < num_chunks; k++) {
            uint64_t cbase = chunks[k].base;
            uint64_t cend  = cbase + chunks[k].size - 1;
            if (type == MTRR_TYPE_WC && base <= cbase && end >= cend) {
                chunk_done[k] = true;
            }
        }

        if (type != MTRR_TYPE_WC) {
            mtrr_log("Disabling MTRR #%u (type %s): overlaps the desired LFB WC area.",
                     i, mtrr_type_name(type));
            mtrr_begin_update();
            // Clear only the valid bit, exactly as mtrrlfbe does: the
            // address mask stays readable so the entry can be identified
            // and re-enabled later.
            mtrr_wrmsr(MSR_IA32_MTRR_PHYSMASK0 + 2 * i, mask_msr & ~MTRR_MASK_VALID);
            mtrr_end_update();
        }
    }

    // Collect available free slots
    int free_slots[16];
    int num_free = 0;
    for (uint32_t i = 0; i < vcnt; i++) {
        uint64_t mask_msr = mtrr_rdmsr(MSR_IA32_MTRR_PHYSMASK0 + 2 * i);
        if (!(mask_msr & MTRR_MASK_VALID)) {
            if (num_free < 16) {
                free_slots[num_free++] = i;
            }
        }
    }

    // Build the list of chunks that still need programming
    int todo[16];
    int num_todo = 0;
    for (int k = 0; k < num_chunks; k++) {
        if (chunk_done[k]) {
            mtrr_log("Chunk %d [0x%08llX - 0x%08llX] is already WC; nothing to do.",
                     k, chunks[k].base, chunks[k].base + chunks[k].size - 1);
        } else if (num_todo < 16) {
            todo[num_todo++] = k;
        }
    }

    if (num_todo == 0) {
        mtrr_log("Every LFB chunk is already covered by write-combining MTRRs.");
        return;
    }

    mtrr_log("Available free variable MTRR slots: %d (slots needed: %d).", num_free, num_todo);

    if (num_free == 0) {
        mtrr_log("ERROR: No free variable MTRRs available to configure LFB!");
        return;
    }

    // Prioritize LFB: if free slots are insufficient, allocate them to the
    // largest chunks (the chunk list is already ordered largest-first).
    int todo_to_apply = num_todo;
    if (num_free < num_todo) {
        mtrr_log("WARNING: Insufficient variable MTRRs! Needed: %d, Available: %d.", num_todo, num_free);
        mtrr_log("Prioritizing LFB: configuring the %d largest chunk(s) starting from base framebuffer.", num_free);
        todo_to_apply = num_free;
    }

    // Program the variable MTRRs
    mtrr_begin_update();
    for (int n = 0; n < todo_to_apply; n++) {
        int k = todo[n];
        int slot = free_slots[n];
        uint64_t chunk_base = chunks[k].base;
        uint64_t chunk_size = chunks[k].size;
        uint64_t chunk_mask = (~(chunk_size - 1)) & phys_mask;

        uint64_t base_msr = (chunk_base & phys_mask) | MTRR_TYPE_WC;
        uint64_t mask_msr = (chunk_mask & phys_mask) | MTRR_MASK_VALID;

        mtrr_wrmsr(MSR_IA32_MTRR_PHYSBASE0 + 2 * slot, base_msr);
        mtrr_wrmsr(MSR_IA32_MTRR_PHYSMASK0 + 2 * slot, mask_msr);

        mtrr_log("Setting MTRR #%d: [0x%08llX - 0x%08llX] (%llu MB), mode: WC.",
                 slot, chunk_base, chunk_base + chunk_size - 1, chunk_size / (1024 * 1024));
    }
    mtrr_end_update();
}

/* -------------------------------------------------------------------------- */
/* Public Entry Point                                                         */
/* -------------------------------------------------------------------------- */

/*
 * The work is split out so that every early return inside it still goes
 * through the wrapper below, which owns the log file.  Otherwise each of the
 * five bail-out paths would have to remember to close it.
 */
static void mtrr_init_body(void)
{
    mtrr_log("=== Initializing MTRR Write-Combining (WC) ===");

    mtrr_log("checking for CPUID.");
    if (!cpu_has_cpuid()) {
        mtrr_log("CPUID instruction is not supported by CPU. Skipping MTRR setup.");
        return;
    }

    uint32_t a = 0, b = 0, c = 0, d = 0;
    char vendor[13];
    memset(vendor, 0, sizeof(vendor));
    cpuid(0, 0, &a, (uint32_t *)&vendor[0], (uint32_t *)&vendor[8], (uint32_t *)&vendor[4]);

    cpuid(1, 0, &a, &b, &c, &d);
    mtrr_log("CPUID done, EDX=0x%08lX.", (unsigned long)d);
    // EDX bit 12: MTRR support
    if (!(d & (1 << 12))) {
        mtrr_log("CPU vendor: %s, CPUID 1 EDX bit 12 is 0: MTRRs are not supported.", vendor);
        return;
    }

    uint64_t phys_mask = get_physical_address_mask();

    // Check Ring 0 / privilege escalation
    int cpl = get_cpl();
    if (cpl == 0) {
        mtrr_log("Running natively at CPL=0 (Ring 0, e.g. CWSDPR0). Direct MSR access enabled.");
    } else {
        mtrr_log("Running at CPL=%d (Ring 3, e.g. CWSDPMI). Initializing Call Gate escalation...", cpl);
        if (!setup_callgate()) {
            mtrr_log("Failed to establish Ring 0 Call Gate. Skipping MTRR modification.");
            return;
        }
    }

    // Read MTRRCAP
    uint64_t mtrrcap = mtrr_rdmsr(MSR_IA32_MTRRCAP);
    uint32_t vcnt    = (uint32_t)(mtrrcap & 0xFF);
    bool     has_fix = (mtrrcap & (1ULL << 8)) != 0;
    bool     has_wc  = (mtrrcap & (1ULL << 10)) != 0;

    mtrr_log("CPU: %s, MTRRCAP: 0x%llX (VCNT = %u, Fixed MTRR = %s, WC = %s)",
             vendor, mtrrcap, vcnt, has_fix ? "YES" : "NO", has_wc ? "YES" : "NO");

    /*
     * The original requires BOTH capabilities: it computes
     * (MTRRCAP & 0x500) and bails out unless that equals 0x500, i.e. WC
     * (bit 10) and fixed-range MTRRs (bit 8) must both be present.  A CPU
     * with WC but no fixed ranges cannot express the VGA window change, so
     * silently proceeding would only produce half the intended effect.
     */
    if ((mtrrcap & 0x500) != 0x500) {
        mtrr_log("MTRRCAP lacks WC and/or fixed-range support (need 0x500, have 0x%llX).",
                 mtrrcap & 0x500);
        if (cpl != 0) teardown_callgate();
        return;
    }

    // Log current MTRR state before modification
    mtrr_log("reading the current MTRR layout.");
    dump_all_mtrrs("Current MTRR Layout (Before)", vcnt, has_fix, phys_mask);
    mtrr_log("current layout read.");

    /*
     * Capture the state before the first write.  mtrr_shutdown() writes it
     * back when the program exits, so nothing this setup changes outlives
     * the run.
     */
    g_saved_fix_a0000 = mtrr_rdmsr(MSR_IA32_MTRR_FIX16K_A00);
    g_saved_vcnt = (vcnt > MTRR_MAX_SAVED_VAR) ? MTRR_MAX_SAVED_VAR : vcnt;
    for (uint32_t i = 0; i < g_saved_vcnt; i++) {
        g_saved_physbase[i] = mtrr_rdmsr(MSR_IA32_MTRR_PHYSBASE0 + 2 * i);
        g_saved_physmask[i] = mtrr_rdmsr(MSR_IA32_MTRR_PHYSMASK0 + 2 * i);
    }
    g_state_saved = true;



    // 1. Configure VGA buffer (0xA0000 - 0xAFFFF, 64KB)
    if (has_fix) {
        mtrr_log("setting the VGA window through fixed MTRR 0x259.");
        set_vga_wc_fixed();
        mtrr_log("VGA window done.");
    } else {
        mtrr_log("Fixed MTRRs not supported; skipping Fixed VGA MTRR.");
    }

    // 2. Query VESA and configure LFB
    uint32_t lfb_base = 0;
    uint32_t lfb_size = 0;
    mtrr_log("querying VESA for the linear framebuffer.");
    if (detect_vesa_lfb(&lfb_base, &lfb_size)) {
        mtrr_log("programming the variable MTRRs for the framebuffer.");
        set_lfb_wc_variable(lfb_base, lfb_size, vcnt, phys_mask);
        mtrr_log("framebuffer MTRRs done.");
    }

    // Log final MTRR state after modification
    dump_all_mtrrs("Updated MTRR Layout (After)", vcnt, has_fix, phys_mask);

    /*
     * The call gate is deliberately left installed: restoring the MTRR state
     * at exit needs ring 0 again, and re-creating the gate there would be a
     * lot of work for one final write.
     */
    mtrr_log("=== MTRR Write-Combining (WC) setup complete ===");
}

/* -------------------------------------------------------------------------- */
/* Entry point                                                                */
/* -------------------------------------------------------------------------- */

/*
 * Put the MTRR registers back the way we found them.  Registered with atexit,
 * so it runs on a normal exit but not when the process is killed -- there is
 * no way to be polite about that one.
 */
static void mtrr_shutdown(void)
{
    if (!g_state_saved || !g_callgate_active) {
        teardown_callgate();
        return;
    }

    mtrr_log_open("a"); // append: keep the startup record in the same file

    mtrr_begin_update();

    /*
     * Invalidate before rewriting, so the CPU never sees a half-updated
     * pair while the base and mask are momentarily inconsistent.
     */
    for (uint32_t i = 0; i < g_saved_vcnt; i++) {
        mtrr_wrmsr(MSR_IA32_MTRR_PHYSMASK0 + 2 * i, 0);
        mtrr_wrmsr(MSR_IA32_MTRR_PHYSBASE0 + 2 * i, g_saved_physbase[i]);
        mtrr_wrmsr(MSR_IA32_MTRR_PHYSMASK0 + 2 * i, g_saved_physmask[i]);
    }
    mtrr_wrmsr(MSR_IA32_MTRR_FIX16K_A00, g_saved_fix_a0000);

    mtrr_end_update();

    mtrr_log("Restored %u variable MTRR pair(s) and fixed MTRR 0x259.",
             g_saved_vcnt);
    mtrr_log("=== MTRR state restored to its value at startup ===");

    g_state_saved = false;
    teardown_callgate();
    mtrr_log_close();
}

/*
 * Ask the DPMI host to identify itself.
 *
 * DPMI 1.0 function 0401h ("Get DPMI Capabilities") fills a 128-byte block:
 * byte 0 and 1 are the host's major and minor version, and byte 2 onwards is
 * the vendor name as an ASCIIZ string.  Common hosts do implement it --
 * CWSDPMI answers "CWSDPMI" despite advertising version 0.90 -- so a missing
 * answer means an old or unusual host rather than the normal case.
 *
 * Fills vendor_out with whatever the host reports (empty when it has no
 * answer) and returns true when the name starts with "VDPMI".  The name is
 * kept even on a miss so the log shows what was actually seen.
 */
static bool dpmi_vendor_is_vdpmi(char *vendor_out, size_t vendor_size)
{
    unsigned char  buf[128];
    unsigned long  flags;
    uint32_t       eax = 0x0401;
    size_t         i;

    vendor_out[0] = '\0';
    memset(buf, 0, sizeof(buf));

    /*
     * 0401h takes ES:EDI as the buffer address.  ES is the flat data
     * selector under DJGPP, but set it from DS explicitly rather than
     * relying on nothing having changed it.
     */
    __asm__ volatile (
        "pushl %%ds\n\t"
        "popl  %%es\n\t"
        "clc\n\t"
        "int   $0x31\n\t"
        "pushfl\n\t"
        "popl  %1\n\t"
        : "+a"(eax), "=&r"(flags)
        : "D"((uint32_t)buf)
        : "memory", "cx", "dx", "bx"
    );

    if (flags & 1) {
        return false;   // carry set: no 0401h, i.e. a DPMI 0.9 host
    }

    for (i = 0; i + 1 < vendor_size && i < 126 && buf[2 + i] != '\0'; i++) {
        vendor_out[i] = (char)buf[2 + i];
    }
    vendor_out[i] = '\0';

    return strncmp(vendor_out, "VDPMI", 5) == 0;
}

void DOS_MTRR_Init(void)
{
    char vendor[128]; // up to 126 bytes of vendor name per the spec
    bool is_vdpmi;
    FILE *fp;

    /*
     * Opt-in, not opt-out.
     *
     * The call gate works on the hosts it has been tested against, but it has
     * also hung machines whose DPMI host the code has never seen, and there
     * is no way for a user to distinguish "no MTRR optimisation" from "the
     * game does not start".  So the setup stays off unless it is asked for,
     * and MTRR.ON in the working directory is that request.
     *
     * This check comes first, before the log file is created and before any
     * DPMI call, so that a machine with a known-bad host is left completely
     * untouched by the time it gets here.
     */
    fp = fopen("MTRR.ON", "r");
    if (fp == NULL) {
        return;
    }
    fclose(fp);

    mtrr_log_open("w");
    mtrr_log("MTRR.ON present in the working directory; enabling MTRR setup.");

    mtrr_log("probing the DPMI host with int 31h/0401h.");
    is_vdpmi = dpmi_vendor_is_vdpmi(vendor, sizeof(vendor));
    mtrr_log("host probe returned.");
    if (vendor[0] != '\0') {
        mtrr_log("DPMI host vendor: \"%s\".", vendor);
    } else {
        mtrr_log("DPMI host gave no vendor string (0401h unsupported); assuming a legacy host.");
    }

    /*
     * The call gate does not survive VDPMI in its current builds, so the
     * setup is skipped there.  VDPMI is closed source at the moment, which
     * leaves nothing to debug against; once it opens up this special case
     * should be replaced by real support rather than kept as a permanent
     * exclusion.
     *
     * This takes precedence over MTRR.ON: asking for the optimisation does
     * not make the call gate work on a host where it is known not to.
     */
    if (is_vdpmi) {
        mtrr_log("VDPMI detected: skipping MTRR setup -- the call gate is not "
                 "compatible with VDPMI yet (closed source, revisit when it opens).");
        mtrr_log_close();
        return;
    }

    mtrr_init_body();

    if (g_state_saved) {
        atexit(mtrr_shutdown);
    } else {
        // setup bailed out before changing anything, so there is nothing to
        // put back and the gate can go right away
        teardown_callgate();
    }

    mtrr_log_close();
}

