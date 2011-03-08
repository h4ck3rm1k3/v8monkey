#ifndef V8_H
#define	V8_H

#include <stack>
#include <map>
#include <iostream>

#include <stdlib.h>
#include <stdint.h>

#define XP_UNIX
#include "jsapi.h"

namespace v8 {

class AccessorInfo;
class Arguments;
class Array;
class Boolean;
class Context;
class Function;
class FunctionTemplate;
class Template;
class String;
class Integer;
class Primitive;
class ResourceConstraints;
class Value;
class Object;

template<class T> class Handle;
template<class T> class Local;
template<class T> class Persistent;

typedef Handle<Value> (*AccessorGetter)(Local<String> property,
    const AccessorInfo& info);

typedef void (*AccessorSetter)(Local<String> property, Local<Value> value,
    const AccessorInfo& info);

typedef Handle<Value> (*InvocationCallback)(const Arguments& args);

typedef void (*FatalErrorCallback)(const char* location, const char* message);

typedef void
(*WeakReferenceCallback)(Persistent<Value> object, void* parameter);

enum AccessControl {
  DEFAULT = 0,
  ALL_CAN_READ = 1 << 0,
  ALL_CAN_WRITE = 1 << 1,
  PHOHIBITS_OVERWRITING = 1 << 2
};

enum PropertyAttribute {
  None = 0,
  ReadOnly = 1 << 0,
  DontEnum = 1 << 1,
  DontDelete = 1 << 2
};

enum ExternalArrayType {
  kExternalByteArray = 1,
  kExternalUnsignedByteArray,
  kExternalShortArray,
  kExternalUnsignedShortArray,
  kExternalIntArray,
  kExternalUnsignedIntArray,
  kExternalFloatArray
};

enum _DataType {
  _kTypeData = 1,
  _kTypeSignature,
  _kTypeTemplate,
  _kTypeFunctionTemplate,
  _kTypeObjectTemplate,
  _kTypeValue,
  _kTypeDate,
  _kTypeObject,
  _kTypeArray,
  _kTypeFunction,
  _kTypePrimitive,
  _kTypeBoolean,
  _kTypeString,
  _kTypeNumber,
  _kTypeInteger,
  _kTypeRegExp,
  _kTypeSymbol
};

namespace internal {

class PrivateData {
public:
  PrivateData(): _indexedProperties(NULL), _numberOfElements(0), _array_type(kExternalByteArray) { }
  inline void set_setter(char* name, v8::AccessorSetter cb) {
    _setters[name] = cb;
  }
  inline v8::AccessorSetter get_setter(char* name) {
    return _setters[name];
  }
  inline void set_getter(char* name, v8::AccessorGetter cb) {
    _getters[name] = cb;
  }
  inline v8::AccessorGetter get_getter(char* name) {
    return _getters[name];
  }
  inline void* get_indexedProperties() { return _indexedProperties; }
  inline void set_indexedProperties(void* data) { _indexedProperties = data; }
  inline int get_numberOfElements() { return _numberOfElements; }
  inline void set_numberOfElements(int number_of_elements) { _numberOfElements = number_of_elements; }
  inline ExternalArrayType get_array_type() { return _array_type; }
  inline void set_array_type(ExternalArrayType type) { _array_type = type; }
private:
  std::map<char *, v8::AccessorSetter> _setters;
  std::map<char *, v8::AccessorGetter> _getters;
  void* _indexedProperties;
  int _numberOfElements;
  ExternalArrayType _array_type;
};

class Object {
public:
  jsval _get_value() const;
  jsval* _get_pvalue() const;
  void _set_value(jsval value);
  JSObject* _get_object() const;
  void _set_object(JSObject* value);
  void _set_string(JSString* value);
  JSString* _get_string() const;
  jsid _get_jsid() const;
  void _set_jsid(jsid value);
  PrivateData* _get_private();
  void _add_to_root();
  void _remove_from_root();
  JSContext* _get_context() const {
    return _ctx;
  }
  void _set_context(JSContext* ctx) {
    _ctx = ctx;
  }
  void _set_persistency(bool persistent) {
    _persistent = persistent;
  }
  bool _get_persistency() {
    return _persistent;
  }
  inline uint32_t _get_type() {
    return _data_type;
  }
  inline bool _is_type(uint32_t type) const {
    return _data_type == type;
  }
  inline void _inc_ref() {
    ++_ref_counter;
  }
  inline void _dec_ref() {
    --_ref_counter;
  }
  inline uint32_t _get_ref_counter() {
    return _ref_counter;
  }
  Object(JSContext* ctx = NULL, uint32_t type = 0) :
    _data_type(type), _ctx(ctx), _persistent(false), _ref_counter(0), _data(NULL) {
  }
  virtual ~Object();
private:
  uint32_t _data_type;
  JSContext* _ctx;
  bool _persistent;
  uint32_t _ref_counter;
  void* _data;
};

}

Handle<Boolean> False();
Handle<Boolean> True();
Handle<Primitive> Null();
Handle<Primitive> Undefined();
Handle<Value> ThrowException(Handle<Value> exception);
bool SetResourceConstraints(ResourceConstraints* constraints);

template<class T>
class Handle {
public:
  inline Handle();
  inline explicit Handle(T* val) :
    val_(val) {
    val_->_inc_ref();
  }
  template<class S> inline Handle(Handle<S> that) :
    val_(reinterpret_cast<T*> (*that)) {
    val_->_inc_ref();
  }
  inline Handle(const Handle<T>& p) {
    val_ = p.val_;
    val_->_inc_ref();
  }
  inline bool IsEmpty() const {
    return val_ == 0;
  }
  inline T* operator->() const {
    return val_;
  }
  inline T* operator*() const {
    return val_;
  }
  template<class S> static inline Handle<T> Cast(Handle<S> that) {
    return Handle<T> (T::Cast(*that));
  }
  inline virtual ~Handle() {
    if (!IsEmpty()) {
      val_->_dec_ref();
      if (val_->_get_ref_counter() == 0 && !val_->_get_persistency()) {
        delete val_;
      }
      val_ = 0;
    }
  }
private:
  T* val_;
};

template<class T>
class Local: public Handle<T> {
public:
  inline Local();
  template<class S> inline Local(Local<S> that) :
    Handle<T> (reinterpret_cast<T*> (*that)) {
  }
  template<class S> inline Local(S* that) :
    Handle<T> (that) {
  }
  template<class S> static inline Local<T> Cast(Local<S> that) {
    return Local<T> (T::Cast(*that));
  }
  inline static Local<T> New(Handle<T> that);
};

template<class T>
class Persistent: public Handle<T> {
public:
  inline Persistent() :
    Handle<T> () {
    this->_set_persistency(true);
  }
  template<class S> inline Persistent(Persistent<S> that) :
    Handle<T> (reinterpret_cast<T*> (*that)) {
    (*that)->_set_persistency(true);
  }
  template<class S> inline Persistent(S* that) :
    Handle<T> (that) {
    (*that)->_set_persistency(true);
  }
  template<class S> explicit inline Persistent(Handle<S> that) :
    Handle<T> (*that) {
    (*that)->_set_persistency(true);
  }
  inline static Persistent<T> New(Handle<T> that);
  inline void Dispose();
  template<class S> static inline Persistent<T> Cast(Persistent<S> that) {
    return Persistent<T> (T::Cast(*that));
  }
};

class ExtensionConfiguration {
public:
  ExtensionConfiguration(int name_count, const char* names[]);
};

class Data: public internal::Object {
public:
  virtual ~Data() {
  }
protected:
  Data(JSContext* ctx = NULL, uint32_t type = _kTypeData);
};

class Value: public Data {
public:
  bool BooleanValue() const;
  int32_t Int32Value() const;
  int64_t IntegerValue() const;
  uint32_t Uint32Value() const;
  bool IsArray() const;
  bool IsBoolean() const;
  bool IsFalse() const;
  bool IsFunction() const;
  bool IsInt32() const;
  bool IsNumber() const;
  bool IsObject() const;
  bool IsTrue() const;
  bool IsUint32() const;
  bool IsUndefined() const;
  Local<Integer> ToInteger() const;
  Local<v8::Object> ToObject() const;
  Local<String> ToString() const;
  virtual ~Value() {
  }
protected:
  Value(JSContext* ctx = NULL, uint32_t type = _kTypeValue);
  friend JSBool prop_setter(JSContext* cx, JSObject* obj, jsid idval, jsval* vp);
  friend JSBool prop_getter(JSContext* cx, JSObject* obj, jsid idval, jsval* vp);
};

class Object: public Value {
public:
  static inline Object* Cast(Value* obj);
  Local<Object> Clone();
  Local<Value> Get(uint32_t index);
  Local<Value> Get(Handle<Value> key);
  uint8_t* GetIndexedPropertiesPixelData();
  int GetIndexedPropertiesPixelDataLength();
  Local<Array> GetPropertyNames();
  bool Has(Handle<String> key);
  bool HasIndexedPropertiesInPixelData();
  int InternalFieldCount();
  static Local<Object> New(JSObject* obj = NULL);
  bool Set(uint32_t index, Handle<Value> value);
  bool Set(Handle<Value> key, Handle<Value> value, PropertyAttribute attribs =
      None);
  bool SetAccessor(Handle<String> name, AccessorGetter getter,
      AccessorSetter setter = 0, Handle<Value> data = Handle<Value> (),
      AccessControl settings = DEFAULT, PropertyAttribute attribute = None);
  void SetIndexedPropertiesToExternalArrayData(void* data,
      ExternalArrayType array_type, int number_of_elements);
  void SetIndexedPropertiesToPixelData(uint8_t* data, int length);
  void SetInternalField(int index, Handle<Value> value);
  void* GetPointerFromInternalField(int index);
  virtual ~Object() {
  }
protected:
  Object(JSContext* ctx = NULL, uint32_t type = _kTypeObject, JSObject* obj = NULL);
private:
  JSObject* _clone(JSObject *obj);
  friend class Context;
  friend class ObjectTemplate;
  friend JSBool prop_setter(JSContext* cx, JSObject* obj, jsid idval, jsval* vp);
  friend JSBool prop_getter(JSContext* cx, JSObject* obj, jsid idval, jsval* vp);
};

class AccessorInfo {
public:
  AccessorInfo(Local<Value> d, Local<Object> t, Local<Object> h): _data(d), _this(t), _holder(h) { };
  inline Local<Value> Data() const { return _data; }
  inline Local<Object> This() const { return _this; }
  inline Local<Object> Holder() const { return _holder; }
private:
  Local<Value> _data;
  Local<Object> _this;
  Local<Object> _holder;
};

class Primitive: public Value {
public:
  virtual ~Primitive() {
  }
protected:
  Primitive(JSContext* ctx = NULL, uint32_t type = _kTypePrimitive) :
    Value(ctx, type) {
  }
};

class String: public Primitive {
public:

  enum WriteHints {
    NO_HINTS = 0, HINT_MANY_WRITES_EXPECTED = 1
  };

  class AsciiValue {
  public:
    explicit AsciiValue(Handle<Value> obj);
    virtual ~AsciiValue();
    char* operator*() {
      return str_;
    }
    const char* operator*() const {
      return str_;
    }
    int length() const {
      return (int) length_;
    }
  private:
    char* str_;
    size_t length_;
    JSContext* ctx_;
    AsciiValue(const AsciiValue&);
    void operator=(const AsciiValue&);
  };

  class Utf8Value {
  public:
    explicit Utf8Value(Handle<Value> obj);
    virtual ~Utf8Value();
    char* operator*() {
      return str_;
    }
    const char* operator*() const {
      return str_;
    }
    int length() const {
      return (int) length_;
    }
  private:
    char* str_;
    size_t length_;
    JSContext* ctx_;
    Utf8Value(const Utf8Value&);
    void operator=(const Utf8Value&);
  };

  class UCValue {
  public:
    explicit UCValue(Handle<Value> obj);
    virtual ~UCValue();
    jschar* operator*() {
      return str_;
    }
    const jschar* operator*() const {
      return str_;
    }
    int length() const {
      return (int) length_;
    }
  private:
    jschar* str_;
    size_t length_;
    JSContext* ctx_;
    UCValue(const UCValue&);
    void operator=(const UCValue&);
  };

  static inline String* Cast(Value* obj);
  static Local<String> Concat(Handle<String> left, Handle<String> right);
  static Local<String> Empty(JSContext* ctx = NULL);
  int Length() const;
  static Local<String> New(const char* data, int length = -1);
  static Local<String> New(const uint16_t* data, int length = -1);
  static Local<String> NewSymbol(const char* data, int length = -1);
  int Utf8Length() const;
  int Write(uint16_t *buffer, int start = 0, int length = -1, WriteHints hints =
      NO_HINTS);
  int WriteAscii(char* buffer, int start = 0, int length = -1,
      WriteHints hints = NO_HINTS);
  int WriteUtf8(char* buffer, int length = -1, int* nchars_ref = NULL,
      WriteHints hints = NO_HINTS);
  virtual ~String() {
  }
protected:
  String(JSContext* ctx = NULL, uint32_t type = _kTypeString) :
    Primitive(ctx, type) {
  }
private:
  static int _strlen16(const uint16_t* buffer);
};

class Arguments {
public:
  inline Local<Object> Holder() const;
private:
  Arguments(Local<Value> data, Local<Object> holder, Local<Function> callee,
      bool is_construct_call, void** values, int length);
};

class Array: public Object {
public:
  static inline Array* Cast(Value* obj);
  uint32_t Length() const;
  static Local<Array> New(int length = 0);
  virtual ~Array() {
  }
protected:
  Array(JSContext* ctx = NULL, uint32_t type = _kTypeArray) :
    Object(ctx, type) {
  }
private:
  friend class Object;
};

class Template: public Data {
public:
  void Set(Handle<String> name, Handle<Data> value,
      PropertyAttribute attributes = None);
  virtual ~Template() {
  }
protected:
  Template(JSContext* ctx = NULL, uint32_t type = _kTypeTemplate) :
    Data(ctx, type) {
  }
};

class ObjectTemplate: public Template {
public:
  static Local<ObjectTemplate> New();
  void
  SetAccessor(Handle<String> name, AccessorGetter getter,
      AccessorSetter setter = 0, Handle<Value> data = Handle<Value> (),
      AccessControl setting = DEFAULT, PropertyAttribute attribute = None);
  inline void SetInternalFieldCount(int value) {
    _fieldCount = value;
  }
  virtual ~ObjectTemplate() {
  }
protected:
  ObjectTemplate(JSContext* ctx = NULL, uint32_t type = _kTypeObjectTemplate) :
    Template(ctx, type), _fieldCount(0) {
  }
private:
  int _fieldCount;
};

class Context: public internal::Object {
public:
  void DetachGlobal();
  void ReattachGlobal(Handle<Object> global_object);
  void Enter();
  void Exit();
  static Local<Context> GetCurrent();
  Local<v8::Object> Global();
  static Persistent<Context> New(ExtensionConfiguration* extension = NULL,
      Handle<ObjectTemplate> global_template = Handle<ObjectTemplate> (),
      Handle<Value> global_object = Handle<Value> ());
  class Scope {
  public:
    inline Scope(Handle<Context> context) :
      context_(context) {
      context_->Enter();
    }
    inline ~Scope() {
      context_->Exit();
    }
  private:
    Handle<Context> context_;
  };
  static inline JSContext* _get_context() {
    return (*_ctx)->_js_ctx;
  }
  static inline JSObject* _get_global_template() {
    return (*_ctx)->_global_template->_get_object();
  }
  static inline JSObject* _get_global_object() {
    return (*_ctx)->_global_object->_get_object();
  }
  inline bool IsEmpty() {
    return false;
  }
  virtual ~Context();
private:
  Context();
  JSContext* _js_ctx;
  Handle<ObjectTemplate> _global_template;
  Handle<Value> _global_object;
  Local<Context> *_prev_ctx;
  static Local<Context> *_ctx;
};

class Date: public Value {
public:
  static inline Date* Cast(Value* obj);
  static Local<Value> New(double time);
  virtual ~Date() {
  }
protected:
  Date(JSContext* ctx = NULL, uint32_t type = _kTypeDate) :
    Value(ctx, type) {
  }
};

class Debug {
public:

  class Message {
  public:
  };

  typedef void (*DebugMessageDispatchHandler)();
  typedef void (*MessageHandler2)(const Debug::Message& message);

  static bool EnableAgent(const char* name, int port, bool wait_for_connection =
      false);
  static void ProcessDebugMessages();
  static void SetDebugMessageDispatchHandler(
      DebugMessageDispatchHandler handler, bool provide_locker = false);
  static void SetMessageHandler2(MessageHandler2 handler);
};

class Boolean: public Primitive {
public:
  virtual ~Boolean() {
  }
protected:
  Boolean(JSContext* ctx = NULL, uint32_t type = _kTypeBoolean) :
    Primitive(ctx, type) {
  }
  static Handle<Boolean> New(bool value);
};

class Exception {
public:
  static Local<Value> Error(Handle<String> message);
  static Local<Value> TypeError(Handle<String> message);
};

class Function: public Object {
public:
  Local<Value> Call(Handle<Object> recv, int argc, Handle<Value> argv[]);
  static inline Function* Cast(Value* obj);
  Local<Object> NewInstance() const;
  Local<Object> NewInstance(int argc, Handle<Value> argv[]) const;
  virtual ~Function() {
  }
protected:
  Function(JSContext* ctx = NULL, uint32_t type = _kTypeFunction) :
    Object(ctx, type) {
  }
};

class Signature: public Data {
public:
  static Local<Signature> New(Handle<FunctionTemplate> receiver = Handle<
      FunctionTemplate> (), int argc = 0, Handle<FunctionTemplate> argv[] = 0);
  virtual ~Signature() {
  }
protected:
  Signature(JSContext* ctx = NULL, uint32_t type = _kTypeSignature) :
    Data(ctx, type) {
  }
};

class FunctionTemplate: public Template {
public:
  Local<Function> GetFunction();
  bool HasInstance(Handle<Value> object);
  void Inherit(Handle<FunctionTemplate> parent);
  Local<ObjectTemplate> InstanceTemplate();
  static Local<FunctionTemplate> New(InvocationCallback callback = 0, Handle<
      Value> data = Handle<Value> (), Handle<Signature> signature = Handle<
      Signature> ());
  Local<ObjectTemplate> PrototypeTemplate();
  void SetClassName(Handle<String> name);
  virtual ~FunctionTemplate() {
  }
protected:
  FunctionTemplate(JSContext* ctx = NULL, uint32_t type =
      _kTypeFunctionTemplate) :
    Template(ctx, type) {
  }
};

class HandleScope {
public:
  // static internal::Object** CreateHandle(internal::Object* value);
  HandleScope();
  virtual ~HandleScope();
  template<class T> Local<T> Close(Handle<T> value);
private:
  bool _success;
  // internal::Object** RawClose(internal::Object** value);
};

class HeapStatistics {
public:
  HeapStatistics();
  size_t total_heap_size() {
    return total_heap_size_;
  }
  size_t used_heap_size() {
    return used_heap_size_;
  }
private:
  size_t total_heap_size_;
  size_t used_heap_size_;
  friend class V8;
};

class Number: public Primitive {
public:
  static Local<Number> New(double);
  static inline Number* Cast(Value* obj);
  virtual ~Number() {
  }
protected:
  Number(JSContext* ctx = NULL, uint32_t type = _kTypeNumber) :
    Primitive(ctx, type) {
  }
};

class Integer: public Number {
public:
  static inline Integer* Cast(Value* obj);
  static Local<Integer> New(int32_t value);
  static Local<Integer> NewFromUnsigned(uint32_t value);
  int64_t Value() const;
  virtual ~Integer() {
  }
protected:
  Integer(JSContext* ctx = NULL, uint32_t type = _kTypeInteger) :
    Number(ctx, type) {
  }
};

class Message: public internal::Object {
public:
  int GetEndColumn() const;
  int GetLineNumber() const;
  Handle<Value> GetScriptResourceName() const;
  Local<String> GetSourceLine() const;
  int GetStartColumn() const;
  Message();
private:
  JSErrorReport* _err;
};

class ResourceConstraints {
public:
  ResourceConstraints();
  int max_young_space_size() const {
    return max_young_space_size_;
  }
  void set_max_young_space_size(int value) {
    max_young_space_size_ = value;
  }
  int max_old_space_size() const {
    return max_old_space_size_;
  }
  void set_max_old_space_size(int value) {
    max_old_space_size_ = value;
  }
  uint32_t* stack_limit() const {
    return stack_limit_;
  }
  void set_stack_limit(uint32_t* value) {
    stack_limit_ = value;
  }
private:
  int max_young_space_size_;
  int max_old_space_size_;
  uint32_t* stack_limit_;
};

class Script: public internal::Object {
public:
  static Local<Script> Compile(Handle<String> source, Handle<Value> file_name,
      Handle<String> script_data = Handle<String> ());
  static Local<Script>
  New(Handle<String> source, Handle<Value> file_name);
  Local<Value> Run();
private:
  JSScript* _script;
};

class TryCatch {
public:
  Local<Value> Exception() const;
  bool HasCaught() const;
  Local<v8::Message> Message() const;
  Handle<Value> ReThrow();
  Local<Value> StackTrace() const;
  TryCatch();
  virtual ~TryCatch();
};

class V8 {
public:
  static int AdjustAmountOfExternalAllocatedMemory(int change_in_bytes);
  static void Dispose();
  static void GetHeapStatistics(HeapStatistics* heap_statistics);
  static const char* GetVersion();
  static bool IdleNotification();
  static bool Initialize();
  static void LowMemoryNotification();
  static void SetFatalErrorHandler(FatalErrorCallback that);
  static void
  SetFlagsFromCommandLine(int* argc, char** argv, bool remove_flags);
  static JSRuntime* _get_runtime();
private:
  V8() {
  }
  static void ClearWeak(internal::Object** global_handle);
  static void DisposeGlobal(internal::Object** global_handle);
  // static internal::Object** GlobalizeReference(internal::Object** handle);
  static bool IsGlobalNearDeath(internal::Object** global_handle);
  static bool IsGlobalWeak(internal::Object** global_handle);
  static void MakeWeak(internal::Object** global_handle, void* data,
      WeakReferenceCallback);
  static int amount_of_external_allocated_memory_;
  static JSRuntime* _runtime;
};

Number* Number::Cast(v8::Value* value) {
  return static_cast<Number*> (value);
}

Integer* Integer::Cast(v8::Value* value) {
  return static_cast<Integer*> (value);
}

Date* Date::Cast(v8::Value* value) {
  return static_cast<Date*> (value);
}

Object* Object::Cast(v8::Value* value) {
  return static_cast<Object*> (value);
}

Array* Array::Cast(v8::Value* value) {
  return static_cast<Array*> (value);
}

Function* Function::Cast(v8::Value* value) {
  return static_cast<Function*> (value);
}

template<class T>
Handle<T>::Handle() :
  val_(0) {
}

template<class T>
Local<T>::Local() :
  Handle<T> () {
}

template<class T>
Local<T> Local<T>::New(Handle<T> that) {
  return Local<T> (that);
}

template<class T>
Persistent<T> Persistent<T>::New(Handle<T> that) {
  return Persistent<T> (that);
}

template<class T>
void Persistent<T>::Dispose() {
  (*this)->_set_persistency(false);
}

}

#endif	/* V8_H */
