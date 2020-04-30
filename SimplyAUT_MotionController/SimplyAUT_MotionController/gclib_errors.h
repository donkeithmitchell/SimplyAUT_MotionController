#ifndef I_D48432D9_1FA3_4C7D_B44C_05F8B9000ADF
 #define I_D48432D9_1FA3_4C7D_B44C_05F8B9000ADF

 //set library visibility for gcc and msvc
 #if BUILDING_GCLIB && HAVE_VISIBILITY
 #define GCLIB_DLL_EXPORTED __attribute__((__visibility__("default")))
 #elif BUILDING_GCLIB && defined _MSC_VER
 #define GCLIB_DLL_EXPORTED __declspec(dllexport)
 #elif defined _MSC_VER
 #define GCLIB_DLL_EXPORTED __declspec(dllimport)
 #else
 #define GCLIB_DLL_EXPORTED
 #endif

 #include "gclib_record.h" // Galil data record structs and unions.
 #include "gclib_errors.h" // GReturn error code values and strings.

 #ifdef _WIN32
 #define GCALL __stdcall
 #else
 #define GCALL
 #endif

 //#define G_USE_GCOMPOUND //!< GCompound() is not part of the standard gclib release. Contact Galil Applications for a special build, http://galil.com/contact.

 #ifdef __cplusplus
 extern "C" {
     #endif
        
           //Constants for function arguments
         #define G_DR 1
         #define G_QR 0
         #define G_BOUNDS - 1
         #define G_CR 0
         #define G_COMMA 1
        
           //Constants for GUtility()
         #define G_UTIL_TIMEOUT 1
         #define G_UTIL_TIMEOUT_OVERRIDE 2
         #define G_USE_INITIAL_TIMEOUT - 1
         #define G_UTIL_VERSION 128
         #define G_UTIL_INFO 129
         #define G_UTIL_SLEEP 130
         #define G_UTIL_ADDRESSES 131
         #define G_UTIL_IPREQUEST 132
         #define G_UTIL_ASSIGN 133
         #define G_UTIL_DEVICE_INITIALIZE 134
         #define G_UTIL_PING 135
         #define G_UTIL_ERROR_CONTEXT 136
        
         #define G_UTIL_GCAPS_HOST 256
         #define G_UTIL_GCAPS_VERSION 257
         #define G_UTIL_GCAPS_KEEPALIVE 258
         #define G_UTIL_GCAPS_ADDRESSES 259
         #define G_UTIL_GCAPS_IPREQUEST 260
         #define G_UTIL_GCAPS_ASSIGN 261
         #define G_UTIL_GCAPS_PING 262
        
        
           //Convenience constants
         #define G_SMALL_BUFFER 1024
         #define G_HUGE_BUFFER 524288
         #define G_LINE_BUFFER 80
        
           typedef int GReturn;
       typedef void* GCon;
       typedef unsigned int GSize;
       typedef int GOption;
      typedef char* GCStringOut;
      typedef const char* GCStringIn;
      typedef char* GBufOut;
      typedef const char* GBufIn;
      typedef unsigned char GStatus;
      typedef void* GMemory;
    
       GCLIB_DLL_EXPORTED GReturn GCALL GOpen(GCStringIn address, GCon * g);
       GCLIB_DLL_EXPORTED GReturn GCALL GClose(GCon g);
       GCLIB_DLL_EXPORTED GReturn GCALL GRead(GCon g, GBufOut buffer, GSize buffer_len, GSize * bytes_read);
       GCLIB_DLL_EXPORTED GReturn GCALL GWrite(GCon g, GBufIn buffer, GSize buffer_len);
       GCLIB_DLL_EXPORTED GReturn GCALL GCommand(GCon g, GCStringIn command, GBufOut buffer, GSize buffer_len, GSize * bytes_returned);
       GCLIB_DLL_EXPORTED GReturn GCALL GProgramDownload(GCon g, GCStringIn program, GCStringIn preprocessor);
       GCLIB_DLL_EXPORTED GReturn GCALL GProgramUpload(GCon g, GBufOut buffer, GSize buffer_len);
       GCLIB_DLL_EXPORTED GReturn GCALL GArrayDownload(GCon g, const GCStringIn array_name, GOption first, GOption last, GCStringIn buffer);
       GCLIB_DLL_EXPORTED GReturn GCALL GArrayUpload(GCon g, const GCStringIn array_name, GOption first, GOption last, GOption delim, GBufOut buffer, GSize buffer_len);
       GCLIB_DLL_EXPORTED GReturn GCALL GRecord(GCon g, union GDataRecord* record, GOption method);
       GCLIB_DLL_EXPORTED GReturn GCALL GMessage(GCon g, GCStringOut buffer, GSize buffer_len);
       GCLIB_DLL_EXPORTED GReturn GCALL GInterrupt(GCon g, GStatus * status_byte);
       GCLIB_DLL_EXPORTED GReturn GCALL GFirmwareDownload(GCon g, GCStringIn filepath);
       GCLIB_DLL_EXPORTED GReturn GCALL GUtility(GCon g, GOption request, GMemory memory1, GMemory memory2);
     #ifdef G_USE_GCOMPOUND
        498   GCLIB_DLL_EXPORTED GReturn GCALL GCompound(GCon g, GCStringIn buffer);
     #endif
        
         #ifdef __cplusplus
         } //extern "C"
 #endif

 #endif //I_D48432D9_1FA3_4C7D_B44C_05F8B9000ADF