#ifndef react_version_HDRINCLUDED
#define react_version_HDRINCLUDED
dnl

#define react_VERSION 0
#define react_MINOR 0
#define react_PATCHLEVEL 0

regexp(VERSION, `^v\([0-9]+\)\([^.]\)?\.\([0-9]+\)\([^.]\)?\.\([0-9]+\)[^-.]*\(-\([0-9]+\)-g\([0-9a-fA-F]+\)\)?$', ``
#undef react_VERSION
#undef react_MINOR
#undef react_PATCHLEVEL
#define react_VERSION \1
#define react_MINOR \3
#define react_PATCHLEVEL \5
'ifelse(`\7',,,``#define react_AHEAD \7
'')`'ifelse(`\8',,,``#define react_COMMIT \8
'')`'')

`#ifdef __cplusplus
extern "C" {
#endif
  extern const char *const react_date;
#ifdef __cplusplus
}
#endif
'

#endif
