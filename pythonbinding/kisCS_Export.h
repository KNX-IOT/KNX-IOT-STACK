
#ifndef kisCS_EXPORT_H
#define kisCS_EXPORT_H

#ifdef kisCS_BUILT_AS_STATIC
#  define kisCS_EXPORT
#  define KISCS_NO_EXPORT
#else
#  ifndef kisCS_EXPORT
#    ifdef kisCS_EXPORTS
        /* We are building this library */
#      define kisCS_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define kisCS_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef KISCS_NO_EXPORT
#    define KISCS_NO_EXPORT 
#  endif
#endif

#ifndef KISCS_DEPRECATED
#  define KISCS_DEPRECATED __declspec(deprecated)
#endif

#ifndef KISCS_DEPRECATED_EXPORT
#  define KISCS_DEPRECATED_EXPORT kisCS_EXPORT KISCS_DEPRECATED
#endif

#ifndef KISCS_DEPRECATED_NO_EXPORT
#  define KISCS_DEPRECATED_NO_EXPORT KISCS_NO_EXPORT KISCS_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef KISCS_NO_DEPRECATED
#    define KISCS_NO_DEPRECATED
#  endif
#endif

#endif /* kisCS_EXPORT_H */
