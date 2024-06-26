/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_ANDROID_NATIVE_VIEW_HPP
#define XCSOAR_ANDROID_NATIVE_VIEW_HPP

#include "Java/Object.hxx"
#include "Java/Class.hxx"
#include "Java/String.hxx"

#ifndef NO_SCREEN
#include "Screen/Point.hpp"
#endif

#include <assert.h>

class NativeView {
  JNIEnv *env;
  Java::GlobalObject obj;

  unsigned width, height;
  unsigned xdpi, ydpi;
  char product[20];
  char default_language[20];

  static Java::TrivialClass cls;
  static jfieldID textureNonPowerOfTwo_field;
  static jmethodID init_surface_method, deinit_surface_method;
  static jmethodID setRequestedOrientationID;
  static jmethodID setScreenOrientationID;
  static jmethodID getRequestedOrientationID;
  static jmethodID getScreenOrientationID;
  static jmethodID getDisplayOrientationID;
  static jmethodID loadResourceBitmap_method;
  static jmethodID loadAssetsBitmap_method;
  static jmethodID loadFileBitmap_method;
  static jmethodID bitmapToTexture_method;
  static jmethodID open_file_method;
  static jmethodID getPackagePath_method;

public:
  /**
   * @see http://developer.android.com/reference/android/R.attr.html#screenOrientation
   */
  enum class ScreenOrientation {
    // API level 1
    UNSPECIFIED = -1,
    LANDSCAPE = 0,
    PORTRAIT = 1,
    USER = 2,
    BEHIND = 3,
    SENSOR = 4,
    NOSENSOR = 5,
    // API level 9
    // see http://developer.android.com/reference/android/content/pm/ActivityInfo.html#SCREEN_ORIENTATION_REVERSE_LANDSCAPE
    REVERSE_LANDSCAPE = 8,
    REVERSE_PORTRAIT = 9,
    // HACK for Galaxy Tab (FROYO = 2.2 = API level 8)
    REVERSE_LANDSCAPE_GT = 7,
    REVERSE_PORTRAIT_GT = 8,
  };

  static void Initialise(JNIEnv *env);
  static void Deinitialise(JNIEnv *env);

  NativeView(JNIEnv *_env, jobject _obj, unsigned _width, unsigned _height,
             unsigned _xdpi, unsigned _ydpi,
             jstring _product, jstring _language)
    :env(_env), obj(env, _obj),
     width(_width), height(_height),
     xdpi(_xdpi), ydpi(_ydpi) {
    Java::String::CopyTo(env, _product, product, sizeof(product));
    Java::String::CopyTo(env, _language, default_language, sizeof(default_language));
  }

#ifndef NO_SCREEN
  PixelSize GetSize() const {
    return { width, height };
  }
#endif

  unsigned GetXDPI() const {
    return xdpi;
  }

  unsigned GetYDPI() const {
    return ydpi;
  }

  void SetSize(unsigned _width, unsigned _height) {
    width = _width;
    height = _height;
  }

  const char *GetProduct() {
    return product;
  }

  const char *GetDefaultLanguage() {
    return default_language;
  }

  bool initSurface() {
    return env->CallBooleanMethod(obj, init_surface_method);
  }

  void deinitSurface() {
    env->CallVoidMethod(obj, deinit_surface_method);
  }

  bool setRequestedOrientation(ScreenOrientation so) {
    return env->CallBooleanMethod(obj, setRequestedOrientationID, (jint)so);
  }

  bool setScreenOrientation(int so) {
    return env->CallBooleanMethod(obj, setScreenOrientationID, (jint)so);
  }

  int getRequestedOrientation() {
    return env->CallIntMethod(obj, getRequestedOrientationID);
  }

  int getDisplayOrientation() {
    return env->CallIntMethod(obj, getDisplayOrientationID);
  }

  int getScreenOrientation() {
    return env->CallIntMethod(obj, getScreenOrientationID);
  }

  Java::LocalObject loadResourceBitmap(const char *name) {
    Java::String name2(env, name);
    return { env, env->CallObjectMethod(obj, loadResourceBitmap_method, name2.Get()) };
  }

  Java::LocalObject loadAssetsBitmap(const char *name) {
    Java::String name2(env, name);
    return { env, env->CallObjectMethod(obj, loadAssetsBitmap_method, name2.Get()) };
  }

  Java::LocalObject loadFileBitmap(const char *path) {
    Java::String path2(env, path);
    return { env, env->CallObjectMethod(obj, loadFileBitmap_method, path2.Get()) };
  }

  bool bitmapToTexture(jobject bmp, bool alpha, jint *result) {
    Java::LocalRef<jintArray> intArray = {env, env->NewIntArray(5) };

    bool success = env->CallBooleanMethod(obj, bitmapToTexture_method,
                                          bmp, alpha, intArray.Get());
    if (success)
      env->GetIntArrayRegion(intArray.Get(), 0, 5, result);

    return success;
  }

  void SetTexturePowerOfTwo(bool value) {
    env->SetStaticBooleanField(cls, textureNonPowerOfTwo_field, value);
  }

  void openFile(const char *pathName) {
    Java::String pathName2(env, pathName);
    env->CallVoidMethod(obj, open_file_method, pathName2.Get());
  }

  void getPackagePath(char *buffer, size_t max_size) const {

    jstring path = (jstring)env->CallObjectMethod(obj, getPackagePath_method);

    if (path != nullptr) {
      Java::String path2(env, path);
      path2.CopyTo(buffer, max_size);
    }
  }

};

#endif
