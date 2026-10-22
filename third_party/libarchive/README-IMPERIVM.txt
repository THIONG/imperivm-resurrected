libarchive 3.7.7 (https://github.com/libarchive/libarchive), BSD 2-Clause (see COPYING).

Only the RAR / RAR5 reader is kept, to unpack .rar content packages without tar.exe (the one in Windows 10
does not read every RAR, and it does not exist in Wine / Proton). The files are the originals; config.h is
written by hand for this MSVC x86 build, and the sources that are not needed to read RAR are left out.
Two small parts of the library have other licenses: BLAKE2 (CC0 / Apache 2.0) and PPMd (public domain).

One change to the original code, in archive_read_support_format_rar5.c (rar5_read_data, marked "Imperivm"): the checksum of the
files that are stored without compression is verified too (libarchive 3.7.7 skips it).
