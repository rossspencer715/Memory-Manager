#include <jni.h>
#include <string>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <iostream>
//#include "MemoryManager/MemoryManager.cpp"
#include "MemoryManager/MemoryManager.h"

# define OUT_PATH "mnt/ubuntu/home/reptilian/logs/"

MemoryManager* mem = new MemoryManager(2, bestFit);

// Provided example native call
extern "C"
JNIEXPORT jstring JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}


extern "C"
JNIEXPORT void JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_initMemoryManager(JNIEnv *env,jobject,jint maxAllocationSize){
    // TODO:
    
    mem->initialize(maxAllocationSize);


}

extern "C"
JNIEXPORT void JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_deleteMemoryManager(JNIEnv *env,jobject){
    // TODO:
    // Function should release all resources held by the Memory Manager
    
    delete mem;
}

extern "C"
JNIEXPORT jint JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_getFreeSize(JNIEnv *env,jobject obj){
    // TODO:
    // Function returns the word size of the free list
    
    return (jint)mem->freeMem();
    //return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_getUseSize(JNIEnv *env,jobject){
    // TODO:
    // Function returns the word size of the in use list
    
    return (jint)mem->usedMem();
    //return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_getFragSize(JNIEnv *env,jobject){
    // TODO:
    // Function returns the word size of the number of fragments within the Memory Manager

    //cancelled
    return 0;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_allocateMemory(JNIEnv *env,jobject,jint size){
    // TODO:
    // Function allocates memory in the Memory Manager and returns the address of the starting block
    // If none is available, return "RIP"

    char *val = (char*)mem->allocate((int)size);
    char idk[50];
    sprintf(idk,"%p",val);

    if(val != nullptr){
        return env->NewStringUTF(idk);
    }
    std::string addr = "RIP";
    return env->NewStringUTF(addr.c_str());
}

extern "C"
JNIEXPORT void JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_freeMemory(JNIEnv *env,jobject, jstring addr){
    // TODO:
    // Function frees block at specified address within the Memory Manager
    
    //uint16_t *val = (uint16_t*)mem->free((std::string)addr);

    /*
    const jclass stringClass = env->GetObjectClass(addr);
    const jmethodID getBytes = env->GetMethodID(stringClass, "getBytes", "(Ljava/lang/String;)[B");
    const jbyteArray stringJbytes = (jbyteArray) env->CallObjectMethod(jStr, getBytes, env->NewStringUTF("UTF-8"));

        size_t length = (size_t) env->GetArrayLength(stringJbytes);
        jbyte* pBytes = env->GetByteArrayElements(stringJbytes, NULL);

        std::string ret = std::string((char *)pBytes, length);
        env->ReleaseByteArrayElements(stringJbytes, pBytes, JNI_ABORT);

        env->DeleteLocalRef(stringJbytes);
        env->DeleteLocalRef(stringClass);
        return ret;*/

    const char *cstr = env->GetStringUTFChars(addr, NULL);
    mem->free((void*)cstr);

}

extern "C"
JNIEXPORT void JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_shutdown(JNIEnv *env,jobject){
    // TODO:
    // Function frees all in use memory from within the Memory Manager
    mem->shutdown();
    mem->initialize(800);
}

extern "C"
JNIEXPORT void JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_setAlgorithm(JNIEnv *env,jobject, jint alg){
    // TODO:
    // Functions changes the internal allocation algorithm used within your Memory Manager
    // 1 denotes Best Fit, 2 denotes Worst fit
    

    if(alg == 1){
        mem->setAllocator(bestFit);
    }
    else{
        mem->setAllocator(worstFit);
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_edu_ufl_cise_os_p2_P2Activity_writeLogs(JNIEnv *env,jobject){
    // TODO:
    // Use your POSIX calls to write logs to file at OUT_PATH that represent the holes in memory

    //cancelled :)
}




