// Copyright (c) 2012 javacef Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "jni_tools.h"
#include "include/internal/cef_types.h"
#include "chromium_settings.h"

#if defined(OS_LINUX)
#include <stdlib.h>
#endif

JavaVM* jvm;
JNIEnv* envs;
jobject jobjs;

void set_jvm(JNIEnv* env, jobject jobj)
{
  jint rs = env->GetJavaVM(&jvm);
  if (rs != JNI_OK) {
    printf("Can not get jvm");
    exit(1);
  }
  envs = env;
  jobjs = jobj;
}

void setup_env() {
  envs = NULL;
  if (jvm->GetEnv((void**)&envs, JNI_VERSION_1_6) == JNI_EDETACHED &&
      jvm->AttachCurrentThread((void**)&envs, NULL) != JNI_OK) {
    printf("Can not get jni env");
    exit(1);
  }
  // TODO: DetachCurrentThread may be needed. jvm can only be attached
  // to one thread.
}

void send_handler(JNIEnv* env, jobject jobj, jlong gh)
{
  jclass cls = env->FindClass("org/embedded/browser/Chromium");
  jmethodID mid = env->GetMethodID(cls, "loadfinish", "(J)V");
  env->CallObjectMethod(jobj, mid, gh);
}

void set_title(const char* title, int id)
{
  //setup_env();
  jclass cls = envs->FindClass("org/embedded/browser/Chromium");
  jmethodID mid = envs->GetStaticMethodID(cls, "title_change", "(Ljava/lang/String;I)V");
  envs->CallStaticObjectMethod(cls, mid, stringtojstring(envs, title), id);
}

jobject get_download_window(const char* fn, long long size, const char* mime)
{
  jclass cls = envs->FindClass("org/embedded/browser/DownloadWindow");
  jmethodID mid = envs->GetMethodID(cls, "<init>", "(Ljava/lang/String;JLjava/lang/String;)V");
  return envs->NewObject(cls, mid, stringtojstring(envs, fn), size, stringtojstring(envs, mime));
}

std::string get_download_path_init(jobject dw)
{
  jclass cls = envs->FindClass("org/embedded/browser/DownloadWindow");
  jmethodID mid = envs->GetMethodID(cls, "getPathAndInit", "()Ljava/lang/String;");
  jstring jpath = (jstring)envs->CallObjectMethod(dw, mid);
  const char* chr = envs->GetStringUTFChars(jpath, 0);
  std::string path(chr);
  envs->ReleaseStringUTFChars(jpath, chr);
  return path;
}

void send_download_handler(jobject dw, int dh)
{
  jclass cls = envs->FindClass("org/embedded/browser/DownloadWindow");
  jmethodID mid = envs->GetMethodID(cls, "set_dhptr", "(I)V");
  envs->CallObjectMethod(dw, mid, dh);
}

void send_download_status(jobject dw, int ds)
{
  jclass cls = envs->FindClass("org/embedded/browser/DownloadWindow");
  jmethodID msid = envs->GetMethodID(cls, "set_status", "(I)V");
  envs->CallObjectMethod(dw, msid, ds);
}

void new_tab(int id, std::string url)
{
  jclass cls = envs->FindClass("org/embedded/browser/Chromium");
  jmethodID mid = envs->GetMethodID(cls, "new_window", "(ILjava/lang/String;)V");//(I)Lorg/embedded/browser/Chromium;
  envs->CallObjectMethod(jobjs, mid, id, stringtojstring(envs, url.c_str()));
  //jfieldID fidc = envs->GetFieldID(cls, "chptr", "I");
  //envs->SetIntField(bobj, fidc, gh);
  //jfieldID fidh = envs->GetFieldID(cls, "hwnd", "I");
  //hwnd = envs->GetIntField(bobj, fidh);
  //return bobj;get_chromium
}

void close_tab(int id)
{
  jclass cls = envs->FindClass("org/embedded/browser/Chromium");
  jmethodID mid = envs->GetMethodID(cls, "close_window", "(I)V");
  envs->CallObjectMethod(jobjs, mid, id);
}

void send_load(int id, bool loading)
{
  //setup_env();
  jclass cls = envs->FindClass("org/embedded/browser/Chromium");
  jmethodID mid = envs->GetStaticMethodID(cls, "load_change", "(IZ)V");
  envs->CallStaticObjectMethod(cls, mid, id, loading);
}

void send_navstate(int id, bool canGoBack, bool canGoForward)
{
  //setup_env();
  jclass cls = envs->FindClass("org/embedded/browser/Chromium");
  jmethodID mid = envs->GetStaticMethodID(cls, "navstate_change", "(IZZ)V");
  envs->CallStaticObjectMethod(cls, mid, id, canGoBack, canGoForward);
}

void get_browser_settings(JNIEnv* env, jobject jcset, ChromiumSettings& cset)
{
  jclass cls = env->FindClass("org/embedded/browser/ChromeSettings");
  jfieldID allow_right_button = env->GetFieldID(cls, "allow_right_button", "Z");
  cset.allow_right_button = (bool)env->GetBooleanField(jcset, allow_right_button);

  jfieldID keyid = env->GetFieldID(cls, "keys", "[Ljava/lang/String;");
  jobjectArray keys = (jobjectArray)env->GetObjectField(jcset, keyid);
  jint klen = env->GetArrayLength(keys);
  jfieldID valueid = env->GetFieldID(cls, "values", "[Ljava/lang/String;");
  jobjectArray values = (jobjectArray)env->GetObjectField(jcset, valueid);
  //jint vlen = env->GetArrayLength(values);
  for (int i = 0; i < klen; i++)
  {
    jstring kstr = (jstring)env->GetObjectArrayElement(keys, (jsize)i);
    jstring vstr = (jstring)env->GetObjectArrayElement(values, (jsize)i);
    const char* kchr = env->GetStringUTFChars(kstr, 0);
    const char* vchr = env->GetStringUTFChars(vstr, 0);
    cset.cookies[std::string(kchr)] = std::string(vchr);
    env->ReleaseStringUTFChars(kstr, kchr);
    env->ReleaseStringUTFChars(vstr, vchr);
  }
}

jstring stringtojstring(JNIEnv* env, const char* pat)
{
  jclass strClass = env->FindClass("Ljava/lang/String;");
  jmethodID ctorID = env->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V");
  jbyteArray bytes = env->NewByteArray((jsize)strlen(pat));
  env->SetByteArrayRegion(bytes, 0, (jsize)strlen(pat), (jbyte*)pat);
  jstring encoding = env->NewStringUTF("utf-8");
  return (jstring)env->NewObject(strClass, ctorID, bytes, encoding);
}
