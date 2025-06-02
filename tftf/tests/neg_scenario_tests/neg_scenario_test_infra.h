#include <string.h>
#include <firmware_image_package.h>
//#include <mbedtls_asn1.h>
#include <mbedtls/oid.h>
#include <mbedtls/platform.h>


/*
    cert:
    cert_len:
    returnPtr: ptr to pubKey in cert

    return 0 upon success, -1 on fail
*/
int get_pubKey_from_cert(void * cert, unsigned int cert_len, void ** returnPtr);