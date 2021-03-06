/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class quickfix_FileStoreFactory */

#ifndef _Included_quickfix_FileStoreFactory
#define _Included_quickfix_FileStoreFactory
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     quickfix_FileStoreFactory
 * Method:    create
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_quickfix_FileStoreFactory_create__
  (JNIEnv *, jobject);

/*
 * Class:     quickfix_FileStoreFactory
 * Method:    destroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_quickfix_FileStoreFactory_destroy
  (JNIEnv *, jobject);

/*
 * Class:     quickfix_FileStoreFactory
 * Method:    create
 * Signature: (Lquickfix/SessionID;)Lquickfix/MessageStore;
 */
JNIEXPORT jobject JNICALL Java_quickfix_FileStoreFactory_create__Lquickfix_SessionID_2
  (JNIEnv *, jobject, jobject);

#ifdef __cplusplus
}
#endif
#endif
