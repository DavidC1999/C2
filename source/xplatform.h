#ifndef XPLATFORM_H
#define XPLATFORM_H

#if defined(WINDOWS)
#define INT64_FORMAT "%lld"
#define INT64_FORMAT_HEX "%llx"
#define UINT64_FORMAT "%llu"
#else  // linux
#define INT64_FORMAT "%ld"
#define INT64_FORMAT_HEX "%lx"
#define UINT64_FORMAT "%lu"
#endif

#endif  // XPLATFORM_H