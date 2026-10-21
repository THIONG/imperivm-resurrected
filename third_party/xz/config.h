/* Hand-written configuration for the reduced liblzma in this folder: only the decoders (LZMA, LZMA2, Delta and the BCJ filters) that the 7z reader of libarchive uses, on Windows, with MSVC or MinGW. */
#ifndef IMPERIVM_XZ_CONFIG_H
#define IMPERIVM_XZ_CONFIG_H

#define HAVE_STDBOOL_H 1
#define HAVE_STDINT_H 1
#define HAVE_INTTYPES_H 1

#define HAVE_DECODERS 1
#define HAVE_DECODER_LZMA1 1
#define HAVE_DECODER_LZMA2 1
#define HAVE_DECODER_DELTA 1
#define HAVE_DECODER_X86 1
#define HAVE_DECODER_POWERPC 1
#define HAVE_DECODER_IA64 1
#define HAVE_DECODER_ARM 1
#define HAVE_DECODER_ARMTHUMB 1
#define HAVE_DECODER_ARM64 1
#define HAVE_DECODER_SPARC 1
#define HAVE_DECODER_RISCV 1

#define MYTHREAD_VISTA 1
#define TUKLIB_SYMBOLS_STATIC 1

#endif
