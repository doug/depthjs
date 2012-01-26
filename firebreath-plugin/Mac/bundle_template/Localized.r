#include <CoreServices/CoreServices.r>

resource 'STR#' (126) 
{ {
    "${FBSTRING_LegalCopyright}",
    "${FBSTRING_ProductName}"
} };

resource 'STR#' (127) 
{ {
    "",
} };

resource 'STR#' (128) 
{ {
@foreach (FBSTRING_MIMEType CUR_MIMETYPE FBSTRING_FileExtents CUR_EXTENT)
    "${CUR_MIMETYPE}",
    "${CUR_EXTENT}",
@endforeach
} };
