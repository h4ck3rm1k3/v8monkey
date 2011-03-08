
#include "v8.h"

#include "string.h"

#include <list>
#include <set>

using namespace std;

#define JSCONTEXT Context::GetCurrent()->_get_context()
#define STACK_SIZE 8192
#define MAXIMUM_SIZE (1024 * 1024 * 2)

namespace v8 {

int V8::amount_of_external_allocated_memory_ = 0;
JSRuntime* V8::_runtime = NULL;
Local<Context>* Context::_ctx = NULL;

static uintN PropertyAttributeToJSAttrs(PropertyAttribute attribute) {
  uintN jsattrs = JSPROP_ENUMERATE;
  if (attribute & ReadOnly) {
    jsattrs |= JSPROP_READONLY;
  }
  if (attribute & DontEnum) {
    jsattrs &= ~JSPROP_ENUMERATE;
  }
  if (attribute & DontDelete) {
    jsattrs |= JSPROP_PERMANENT;
  }
  return jsattrs;
}

static list<jsid> JSProperties(JSContext* cx, JSObject *obj) {
  list<jsid> ids;
  JSObject* it = JS_NewPropertyIterator(cx, obj);
  if (it != NULL) {
    jsid idp;
    while(JS_NextProperty(cx, it, &idp) == JS_TRUE) {
      if (JSID_IS_VOID(idp)) {
        break;
      }
      ids.push_back(idp);
    }
  }
  return ids;
}

internal::Object::~Object() {
  if (_is_type(_kTypeObject)) {
    PrivateData* p = (PrivateData *) JS_GetPrivate(_get_context(),
        _get_object());
    delete p;
  }
  _remove_from_root();
}

jsval internal::Object::_get_value() const {
  if (_is_type(_kTypeSymbol)) {
    jsval val;
    JS_IdToValue(_get_context(), (jsid) _data, &val);
    return val;
  }
  if (_is_type(_kTypeString)) {
    return STRING_TO_JSVAL((JSString *)_data);
  }
  if (_is_type(_kTypeObject)) {
    return OBJECT_TO_JSVAL((JSObject *)_data);
  }
  return (jsval) _data;
}

jsval* internal::Object::_get_pvalue() const {
  return (jsval *) _data;
}

void internal::Object::_set_value(jsval value) {
  _remove_from_root();
  _data_type = _data_type & ~_kTypeSymbol;
  if (_is_type(_kTypeString)) {
    if (JSVAL_IS_STRING(value)) {
      _data = (void *) JSVAL_TO_STRING(value);
    } else {
      _data = (void *) JS_ValueToString(_get_context(), value);
    }
  } else if (_is_type(_kTypeObject)) {
    if (JSVAL_IS_OBJECT(value)) {
      _data = (void *) JSVAL_TO_OBJECT(value);
    } else {
      JS_ValueToObject(_get_context(), value, (JSObject **) &_data);
    }
  } else {
    _data = (void *) value;
  }
  _add_to_root();
}

JSObject* internal::Object::_get_object() const {
  JSObject* obj;
  if (_is_type(_kTypeSymbol)) {
    jsval val;
    JS_IdToValue(_get_context(), (jsid) _data, &val);
    JS_ValueToObject(_get_context(), val, &obj);
  } else if (_is_type(_kTypeObject)) {
    return (JSObject *) _data;
  } else {
    JS_ValueToObject(_get_context(), _get_value(), &obj);
  }
  return obj;
}

void internal::Object::_set_object(JSObject* value) {
  _remove_from_root();
  if (_is_type(_kTypeSymbol)) {
    _data_type = _data_type & ~_kTypeSymbol;
  }
  if (_is_type(_kTypeString)) {
    _data = (void *) JS_ValueToString(_get_context(), OBJECT_TO_JSVAL(value));
  } else if (_is_type(_kTypeObject)) {
    _data = (void *) value;
  } else {
    _data = (void *) OBJECT_TO_JSVAL(value);
  }
  _add_to_root();
}

void internal::Object::_set_string(JSString* value) {
  _remove_from_root();
  if (_is_type(_kTypeSymbol)) {
    JS_ValueToId(_get_context(), STRING_TO_JSVAL(value), (jsid *) &_data);
  } else if (_is_type(_kTypeString)) {
    _data = (void *) value;
  } else if (_is_type(_kTypeObject)) {
    JS_ValueToObject(_get_context(), STRING_TO_JSVAL(value),
        (JSObject **) &_data);
  } else {
    _data = (void *) STRING_TO_JSVAL(value);
  }
  _add_to_root();
}

JSString* internal::Object::_get_string() const {
  if (_is_type(_kTypeSymbol)) {
    jsval val;
    JS_IdToValue(_get_context(), (jsid) _data, &val);
    return JS_ValueToString(_get_context(), val);
  } else if (_is_type(_kTypeString)) {
    return (JSString *) _data;
  }
  return JS_ValueToString(_get_context(), _get_value());
}

jsid internal::Object::_get_jsid() const {
  if (_is_type(_kTypeSymbol)) {
    return (jsid) _data;
  }
  jsid id;
  JS_ValueToId(_get_context(), _get_value(), &id);
  return id;
}

void internal::Object::_set_jsid(jsid value) {
  _remove_from_root();
  _persistent = false;
  _data_type = _data_type | _kTypeSymbol;
  _data = (void *) value;
}

internal::PrivateData* internal::Object::_get_private() {
  if (_is_type(_kTypeObject)) {
    PrivateData* p = (PrivateData *) JS_GetPrivate(_get_context(),
        _get_object());
    if (p) {
      p = new PrivateData();
      JS_SetPrivate(_get_context(), _get_object(), (void *) p);
    }
    return p;
  }
  return NULL;
}

void internal::Object::_add_to_root() {
  if (_is_type(_kTypeSymbol) || !_data) {
    return;
  }
  if (_is_type(_kTypeString)) {
    JS_AddStringRoot(_get_context(), (JSString **) &_data);
  } else if (_is_type(_kTypeObject)) {
    JS_AddObjectRoot(_get_context(), (JSObject **) &_data);
  } else {
    JS_AddValueRoot(_get_context(), (jsval *) &_data);
  }
}

void internal::Object::_remove_from_root() {
  if (_is_type(_kTypeSymbol) || !_data) {
    return;
  }
  if (_is_type(_kTypeString)) {
    JS_RemoveStringRoot(_get_context(), (JSString **) &_data);
  } else if (_is_type(_kTypeObject)) {
    JS_RemoveObjectRoot(_get_context(), (JSObject **) &_data);
  } else {
    JS_RemoveValueRoot(_get_context(), (jsval *) &_data);
  }
}

Handle<Boolean> Boolean::New(bool value) {
  Local<Boolean> p(new Boolean());
  p->_set_value(BOOLEAN_TO_JSVAL(value? JS_TRUE : JS_FALSE));
  return p;
}

Handle<Boolean> False() {
  static Handle<Boolean>* false_value = NULL;
  if (!false_value) {
    false_value = new Handle<Boolean> ();
    (*false_value)->_set_value(JSVAL_FALSE);
  }
  return *false_value;
}

Handle<Boolean> True() {
  static Handle<Boolean>* true_value = NULL;
  if (!true_value) {
    true_value = new Handle<Boolean> ();
    (*true_value)->_set_value(JSVAL_TRUE);
  }
  return *true_value;
}

Handle<Primitive> Null() {
  static Handle<Primitive>* null_value = NULL;
  if (!null_value) {
    null_value = new Handle<Primitive> ();
    (*null_value)->_set_value(JSVAL_NULL);
  }
  return *null_value;
}

Handle<Primitive> Undefined() {
  static Handle<Primitive>* undef_value = NULL;
  if (!undef_value) {
    undef_value = new Handle<Primitive> ();
    (*undef_value)->_set_value(JSVAL_VOID);
  }
  return *undef_value;
}

bool SetResourceConstraints(ResourceConstraints* constraints) {
  // TODO: fix me
  return true;
}

Handle<Value> ThrowException(Handle<Value> exception) {
  // TODO: fix me
  JS_ReportError(exception->_get_context(), "%s", "exception");
  return exception;
}

Arguments::Arguments(Local<Value> data, Local<Object> holder,
    Local<Function> callee, bool is_construct_call, void** values, int length) {
}

Local<Array> Array::New(int length) {
  Local<Array> p(new Array());
  length = length < 0 ? 0 : length;
  p->_set_object(JS_NewArrayObject(p->_get_context(), length, NULL));
  return p;
}

uint32_t Array::Length() const {
  jsuint length = 0;
  if (JS_IsArrayObject(_get_context(), _get_object()) == JS_TRUE) {
    JS_GetArrayLength(_get_context(), _get_object(), &length);
  }
  return (uint32_t) length;
}

ExtensionConfiguration::ExtensionConfiguration(int name_count,
    const char* names[]) {
}

String::AsciiValue::AsciiValue(Handle<Value> obj) {
  JSString* jsstr = obj->_get_string();
  length_ = JS_GetStringLength(jsstr);
  ctx_ = obj->_get_context();
  str_ = JS_EncodeString(ctx_, jsstr);
}

String::AsciiValue::~AsciiValue() {
  if (str_) {
    JS_free(ctx_, str_);
    str_ = NULL;
  }
}

String::Utf8Value::Utf8Value(Handle<Value> obj) {
  JSString* jsstr = obj->_get_string();
  length_ = 0;
  ctx_ = obj->_get_context();
  JS_EncodeCharacters(ctx_, JS_GetStringChars(jsstr),
      JS_GetStringLength(jsstr), NULL, &length_);
  str_ = (char *) JS_malloc(ctx_, length_ * sizeof(char));
  if (str_) {
    JS_EncodeCharacters(ctx_, JS_GetStringChars(jsstr), JS_GetStringLength(
        jsstr), str_, &length_);
  }
}

String::Utf8Value::~Utf8Value() {
  if (str_) {
    JS_free(ctx_, str_);
    str_ = NULL;
  }
}

String::UCValue::UCValue(Handle<Value> obj) {
  JSString* jsstr = obj->_get_string();
  length_ = JS_GetStringLength(jsstr);
  ctx_ = obj->_get_context();
  str_ = (jschar *) JS_malloc(ctx_, length_ * sizeof(jschar));
  memcpy(str_, JS_GetStringChars(jsstr), length_ * sizeof(jschar));
}

String::UCValue::~UCValue() {
  if (str_) {
    JS_free(ctx_, str_);
    str_ = NULL;
  }
}

Local<String> String::Concat(Handle<String> left, Handle<String> right) {
  JSContext* ctx = left->_get_context();
  Local<String> p(new String(ctx));
  p->_set_string(JS_ConcatStrings(ctx, left->_get_string(),
      right->_get_string()));
  return p;
}

Local<String> String::Empty(JSContext* ctx) {
  Local<String> p(new String());
  if (!ctx) {
    ctx = p->_get_context();
  }
  p->_set_value(JS_GetEmptyStringValue(ctx));
  return p;
}

int String::Length() const {
  size_t len;
  JS_EncodeCharacters(_get_context(), JS_GetStringChars(_get_string()),
      Utf8Length(), NULL, &len);
  return (int) len;
}

int String::_strlen16(const uint16_t* buffer) {
  int length = 0;
  if (buffer) {
    while (*buffer++) {
      ++length;
    }
  }
  return length;
}

Local<String> String::New(const char* data, int length) {
  Local<String> p(new String());
  length = length < 0 ? strlen(data) : length;
  p->_set_string(JS_NewStringCopyN(p->_get_context(), data, length));
  return p;
}

Local<String> String::New(const uint16_t* data, int length) {
  Local<String> p(new String());
  length = length < 0 ? _strlen16(data) : length;
  p->_set_string(JS_NewUCStringCopyN(p->_get_context(), data, length));
  return p;
}

Local<String> String::NewSymbol(const char* data, int length) {
  Local<String> p = String::New(data, length);
  return p;
}

int String::Utf8Length() const {
  return JS_GetStringLength(_get_string());
}

int String::Write(uint16_t *buffer, int start, int length, WriteHints hints) {
  if (!buffer || !length) {
    return 0;
  }
  start = (start < 0) ? 0 : start;
  int buffer_length = _strlen16(buffer) - start;
  if (buffer_length < 1) {
    return 0;
  }
  length = (length < 0) ? buffer_length : length;
  jschar* nb = (jschar *) JS_malloc(_get_context(), length * sizeof(jschar));
  if (!nb) {
    return 0;
  }
  memcpy(nb, buffer + start, buffer_length * sizeof(jschar));
  uint16_t *pb = nb + buffer_length;
  while (length > buffer_length++) {
    *pb++ = (uint16_t) ' ';
  }
  _set_string(JS_NewUCString(_get_context(), nb, length));
  return length;
}

int String::WriteAscii(char* buffer, int start, int length, WriteHints hints) {
  if (!buffer || !length) {
    return 0;
  }
  start = (start < 0) ? 0 : start;
  int buffer_length = strlen(buffer) - start;
  if (buffer_length < 1) {
    return 0;
  }
  length = (length < 0) ? buffer_length : length;
  char* nb = (char *) JS_malloc(_get_context(), length * sizeof(char));
  if (!nb) {
    return 0;
  }
  memcpy(nb, buffer + start, buffer_length * sizeof(char));
  if (length > buffer_length) {
    memset(nb + buffer_length, ' ', (length - buffer_length) * sizeof(char));
  }
  _set_string(JS_NewString(_get_context(), nb, length));
  return length;
}

int String::WriteUtf8(char* buffer, int length, int* nchars_ref,
    WriteHints hints) {
  length = length < 0 ? strlen(buffer) : length;
  jschar* ucStr = (jschar *) JS_malloc(_get_context(), length * sizeof(jschar));
  size_t dstlen = length;
  if (JS_DecodeBytes(_get_context(), buffer, length, ucStr, &dstlen) == JS_TRUE) {
    if (nchars_ref) {
      *nchars_ref = dstlen;
    }
  }
  return dstlen;
}

v8::Object::Object(JSContext* ctx, uint32_t type, JSObject* obj) :
  Value(ctx, type) {
  _set_object((obj == NULL)? JS_NewObject(_get_context(), NULL, NULL, NULL) : obj);
}

JSObject* v8::Object::_clone(JSObject *obj) {
  JSContext* cx = _get_context();
  JSObject* cloned = NULL;
  if (JS_ObjectIsFunction(cx, obj) == JS_TRUE) {
    return JS_CloneFunctionObject(cx, obj, JS_GetParent(cx, obj));
  }
  if (JS_IsArrayObject(cx, obj) == JS_TRUE) {
    JSIdArray* it = JS_Enumerate(cx, obj);
    if (it == NULL) {
      return NULL;
    }
    JSObject* cloned = JS_NewArrayObject(cx, it->length, NULL);
    if (cloned == NULL) {
      JS_DestroyIdArray(cx, it);
      return NULL;
    }
    jsval vp;
    for(jsint i = 0; i < it->length; ++i) {
      if (JS_GetElement(cx, obj, it->vector[i], &vp) == JS_TRUE) {
        if (JSVAL_IS_OBJECT(vp) && !JSVAL_IS_NULL(vp)) {
          jsval value = OBJECT_TO_JSVAL(_clone(JSVAL_TO_OBJECT(vp)));
          JS_SetElement(cx, obj, it->vector[i], &value);
        } else {
          JS_SetElement(cx, obj, it->vector[i], &vp);
        }
      }
    }
    JS_DestroyIdArray(cx, it);
  } else {
    cloned = JS_ConstructObject(cx, JS_GetClass(cx, obj), JS_GetPrototype(cx, obj), JS_GetParent(cx, obj));
    if (cloned == NULL) {
      return NULL;
    }
    list<jsid> properties = JSProperties(_get_context(), _get_object());
    jsval vp;
    list<jsid>::iterator it;
    for(it = properties.begin(); it != properties.end(); ++it) {
      if (JS_GetPropertyById(cx, obj, *it, &vp) == JS_TRUE) {
        if (JSVAL_IS_OBJECT(vp) && !JSVAL_IS_NULL(vp)) {
          jsval value =  OBJECT_TO_JSVAL(_clone(JSVAL_TO_OBJECT(vp)));
          JS_SetPropertyById(cx, obj, *it, &value);
        } else {
          JS_SetPropertyById(cx, obj, *it, &vp);
        }
      }
    }
  }
  return cloned;
}

Local<v8::Object> v8::Object::Clone() {
  Local<v8::Object> p;
  p->_set_object(_clone(_get_object()));
  return p;
}

Local<Value> v8::Object::Get(uint32_t index) {
  Handle<Value> key;
  key->_set_value(INT_TO_JSVAL(index));
  return Get(key);
}

Local<Value> v8::Object::Get(Handle<Value> key) {
  jsval vp;
  if (JS_GetPropertyById(_get_context(), _get_object(), key->_get_jsid(), &vp) == JS_TRUE) {
    Local<Value> value;
    value->_set_value(vp);
    return value;
  }
  Local<Value> p;
  p->_set_value(Undefined()->_get_value());
  return p;
}

uint8_t* v8::Object::GetIndexedPropertiesPixelData() {
  return (uint8_t *)_get_private()->get_indexedProperties();
}

int v8::Object::GetIndexedPropertiesPixelDataLength() {
  return _get_private()->get_numberOfElements();
}

Local<Array> v8::Object::GetPropertyNames() {
  list<jsid> ids = JSProperties(_get_context(), _get_object());;
  JSObject *proto = JS_GetPrototype(_get_context(), _get_object());
  if (proto != NULL) {
    list<jsid> pproto = JSProperties(_get_context(), proto);
    ids.insert(ids.end(), pproto.begin(), pproto.end());
  }
  Local<Array> properties = Array::New(ids.size());
  JSObject* array = properties->_get_object();
  JSContext* cx = _get_context();
  list<jsid>::iterator it;
  jsint index = 0;
  jsval vp;
  JSString* propName;
  set<string> props;
  char *prop;
  for(it = ids.begin(); it != ids.end(); ++it) {
    if (JS_IdToValue(cx, *it, &vp) == JS_TRUE) {
      if ((propName = JS_ValueToString(cx, vp)) != NULL) {
        prop = JS_EncodeString(cx, propName);
        if (prop != NULL && props.insert(prop).second) {
          vp = STRING_TO_JSVAL(propName);
          JS_SetElement(cx, array, index, &vp);
          index++;
        }
        if (prop) {
          JS_free(cx, prop);
        }
      }
    }
  }
  return Local<Array> (properties);
}

bool v8::Object::Has(Handle<String> key) {
  JSBool foundp;

  if (JS_HasPropertyById(_get_context(), _get_object(), key->_get_jsid(), &foundp) == JS_TRUE) {
    return (foundp == JS_TRUE);
  }
  return false;
}

bool v8::Object::HasIndexedPropertiesInPixelData() {
  return (_get_private()->get_indexedProperties() != NULL);
}

int v8::Object::InternalFieldCount() {
  return 0;
}

Local<v8::Object> v8::Object::New(JSObject* obj) {
  return new v8::Object::Object(NULL, _kTypeObject, obj);
}

bool v8::Object::Set(uint32_t index, Handle<Value> value) {
  Handle<Value> key;
  key->_set_value(INT_TO_JSVAL(index));
  return (JS_SetPropertyById(_get_context(), _get_object(), key->_get_jsid(), value->_get_pvalue()) == JS_TRUE);
}

bool v8::Object::Set(Handle<Value> key, Handle<Value> value,
    PropertyAttribute attribs) {
  JSBool foundp;

  String::UCValue property(key);
  if (JS_SetUCPropertyAttributes(_get_context(), _get_object(), *property, property.length(), PropertyAttributeToJSAttrs(attribs), &foundp) == JS_TRUE) {
    if (foundp == JS_TRUE) {
      return (JS_SetUCProperty(_get_context(), _get_object(), *property, property.length(), value->_get_pvalue()) == JS_TRUE);
    }
  }
  return false;
}

bool v8::Object::SetAccessor(Handle<String> name, AccessorGetter getter,
    AccessorSetter setter, Handle<Value> data, AccessControl settings,
    PropertyAttribute attribute) {
  return false;
}

void v8::Object::SetIndexedPropertiesToExternalArrayData(void* data,
    ExternalArrayType array_type, int number_of_elements) {
  internal::PrivateData *privData = _get_private();

  privData->set_indexedProperties(data);
  privData->set_array_type(array_type);
  privData->set_numberOfElements(number_of_elements);
}

void v8::Object::SetIndexedPropertiesToPixelData(uint8_t* data, int length) {
  SetIndexedPropertiesToExternalArrayData(data, kExternalByteArray, length);
}

void v8::Object::SetInternalField(int index, Handle<Value> value) {
}

void* v8::Object::GetPointerFromInternalField(int index) {
  internal::PrivateData* privData = _get_private();
  if (index < 0 || index >= privData->get_numberOfElements()) {
    return NULL;
  }
  switch(privData->get_array_type()) {
    case kExternalByteArray:
    case kExternalUnsignedByteArray:
      return (void *)(reinterpret_cast<uint8_t *>(privData->get_indexedProperties()) + index);
    case kExternalShortArray:
    case kExternalUnsignedShortArray:
      return (void *)(reinterpret_cast<short *>(privData->get_indexedProperties()) + index);
    case kExternalIntArray:
    case kExternalUnsignedIntArray:
      return (void *)(reinterpret_cast<int *>(privData->get_indexedProperties()) + index);
    case kExternalFloatArray:
      return (void *)(reinterpret_cast<float *>(privData->get_indexedProperties()) + index);
  }
  return NULL;
}

JSBool prop_setter(JSContext* cx, JSObject* obj, jsid idval, jsval* vp) {
  Local<String> property = String::Empty(cx);
  property->_set_jsid(idval);
  Local<Value> d(new Value(cx));
  d->_set_value(*vp);
  Local<Object> t(new Object(cx));
  t->_set_object(obj);
  AccessorInfo info(d, t, t);
  internal::PrivateData* p = (internal::PrivateData *) JS_GetPrivate(cx, obj);
  AccessorSetter setter = p->get_setter(*String::Utf8Value(property));
  if (setter) {
    Local<Value> value(new Value(cx));
    value->_set_value(*vp);
    (*setter)(property, value, info);
    return JS_TRUE;
  }
  return JS_FALSE;
}

JSBool prop_getter(JSContext* cx, JSObject* obj, jsid idval, jsval* vp) {
  Local<String> property = String::Empty(cx);
  property->_set_jsid(idval);
  Local<Value> d(new Value(cx));
  d->_set_value(*vp);
  Local<Object> t(new Object(cx));
  t->_set_object(obj);
  AccessorInfo info(d, t, t);
  internal::PrivateData* p = (internal::PrivateData *) JS_GetPrivate(cx, obj);
  AccessorGetter getter = p->get_getter(*String::Utf8Value(property));
  if (getter) {
    Handle<Value> rc = (*getter)(property, info);
    *vp = rc->_get_value();
    return JS_TRUE;
  }
  return JS_FALSE;
}

Local<ObjectTemplate> ObjectTemplate::New() {
  return Local<ObjectTemplate> (new ObjectTemplate());
}

void ObjectTemplate::SetAccessor(Handle<String> name, AccessorGetter getter,
    AccessorSetter setter, Handle<Value> data, AccessControl setting,
    PropertyAttribute attribute) {
  if (getter) {
    this->_get_private()->set_getter(*String::Utf8Value(name), getter);
  }
  if (setter) {
    this->_get_private()->set_setter(*String::Utf8Value(name), setter);
  }
  JS_DefinePropertyById(_get_context(), _get_object(), name->_get_jsid(),
      data->_get_value(), (getter ? prop_getter : NULL), (setter ? prop_setter
          : NULL), PropertyAttributeToJSAttrs(attribute));
}

Context::Context() :
  _js_ctx(NULL), _prev_ctx(NULL) {
  _js_ctx = JS_NewContext(V8::_get_runtime(), STACK_SIZE);
}

Context::~Context() {
  if (_js_ctx) {
    JS_DestroyContext(_js_ctx);
  }
}

void Context::DetachGlobal() {
  if (_js_ctx) {
    JS_SetGlobalObject(_js_ctx, NULL);
  }
}

void Context::ReattachGlobal(Handle<Object> global_object) {
  if (_js_ctx && !global_object.IsEmpty()) {
    JS_SetGlobalObject(_js_ctx, global_object->_get_object());
  }
}

void Context::Enter() {
  _prev_ctx = _ctx;
  _ctx = new Local<Context> (this);
}

void Context::Exit() {
  delete _ctx;
  _ctx = _prev_ctx;
}

Local<Context> Context::GetCurrent() {
  if (!_ctx) {
    Context::New()->Enter();
  }
  return *_ctx;
}

Local<v8::Object> Context::Global() {
  Local<Object> p(new Object());
  if (_js_ctx) {
    p->_set_object(JS_GetGlobalObject(_js_ctx));
  }
  return p;
}

Persistent<Context> Context::New(ExtensionConfiguration* extension, Handle<
    ObjectTemplate> global_template, Handle<Value> global_object) {
  // TODO: ignoring extension. Fix me
  Persistent<Context> p(Handle<Context> (new Context()));
  p->_global_template = global_template;
  p->_global_object = global_object;
  return p;
}

Local<Value> Date::New(double time) {
  Local<Value> p(new Date());
  JS_NewNumberValue(p->_get_context(), (jsdouble) time, p->_get_pvalue());
  return p;
}

bool Debug::EnableAgent(const char* name, int port, bool wait_for_connection) {
  return false;
}

void Debug::ProcessDebugMessages() {
}

void Debug::SetDebugMessageDispatchHandler(DebugMessageDispatchHandler handler,
    bool provide_locker) {
}

void Debug::SetMessageHandler2(MessageHandler2 handler) {
}

Local<Value> Exception::Error(Handle<String> message) {
  return Local<Value> ();
}

Local<Value> Exception::TypeError(Handle<String> message) {
  return Local<Value> ();
}

Local<Value> Function::Call(Handle<Object> recv, int argc, Handle<Value> argv[]) {
  return Local<Value> ();
}

Local<Object> Function::NewInstance() const {
  return Local<Object> ();
}

Local<Object> Function::NewInstance(int argc, Handle<Value> argv[]) const {
  return Local<Object> ();
}

Local<Signature> Signature::New(Handle<FunctionTemplate> receiver, int argc,
    Handle<FunctionTemplate> argv[]) {
  return Local<Signature> (new Signature());
}

Local<Function> FunctionTemplate::GetFunction() {
  return Local<Function> ();
}

bool FunctionTemplate::HasInstance(Handle<Value> object) {
  return false;
}

void FunctionTemplate::Inherit(Handle<FunctionTemplate> parent) {
}

Local<ObjectTemplate> FunctionTemplate::InstanceTemplate() {
  return Local<ObjectTemplate> ();
}

Local<FunctionTemplate> FunctionTemplate::New(InvocationCallback callback,
    Handle<Value> data, Handle<Signature> signature) {
  return Local<FunctionTemplate> (new FunctionTemplate());
}

Local<ObjectTemplate> FunctionTemplate::PrototypeTemplate() {
  return Local<ObjectTemplate> ();
}

void FunctionTemplate::SetClassName(Handle<String> name) {
}

HandleScope::HandleScope() {
  _success = (JS_EnterLocalRootScope(JSCONTEXT) == JS_TRUE);
}

HandleScope::~HandleScope() {
  if (_success) {
    _success = false;
    JS_LeaveLocalRootScope(JSCONTEXT);
  }
}

template<class T>
Local<T> HandleScope::Close(Handle<T> value) {
  if (_success) {
    _success = false;
    JS_LeaveLocalRootScopeWithResult(value->_get_context(), value->_get_value());
  }
}

Local<Integer> Integer::New(int32_t value) {
  Local<Integer> p(new Integer());
  JS_NewNumberValue(p->_get_context(), (jsdouble) value, p->_get_pvalue());
  return p;
}

Local<Integer> Integer::NewFromUnsigned(uint32_t value) {
  Local<Integer> p(new Integer());
  JS_NewNumberValue(p->_get_context(), (jsdouble) value, p->_get_pvalue());
  return p;
}

int64_t Integer::Value() const {
  return JSVAL_TO_INT(_get_value());
}

Message::Message() {
  if (JS_GetPendingException(JSCONTEXT, (jsval *) _err) != JS_TRUE) {
    _err = NULL;
  }
}

int Message::GetEndColumn() const {
  return GetStartColumn();
}

int Message::GetLineNumber() const {
  if (_err == NULL) {
    return 0;
  }
  return _err->lineno;
}

Handle<Value> Message::GetScriptResourceName() const {
  if (_err == NULL) {
    return Handle<Value> ();
  }
  return String::New(_err->filename);
}

Local<String> Message::GetSourceLine() const {
  if (_err == NULL) {
    return String::Empty();
  }
  return String::New(_err->uclinebuf);
}

int Message::GetStartColumn() const {
  if (_err == NULL || !_err->uctokenptr || !_err->uclinebuf) {
    return 0;
  }
  return (_err->uctokenptr - _err->uclinebuf);
}

Local<Number> Number::New(double value) {
  Local<Number> p(new Number());
  JS_NewNumberValue(p->_get_context(), (jsdouble) value, p->_get_pvalue());
  return p;
}

ResourceConstraints::ResourceConstraints() {
}

Local<Script> Script::Compile(Handle<String> source, Handle<Value> file_name,
    Handle<String> script_data) {
  Local<Script> p;
  String::UCValue code(source);
  String::AsciiValue file(file_name);
  p->_script = JS_CompileUCScript(JSCONTEXT, NULL, *code, code.length(), *file,
      1);
  return p;
}

Local<Script> Script::New(Handle<String> source, Handle<Value> file_name) {
  return Compile(source, file_name);
}

Local<Value> Script::Run() {
  Local<Value> p;
  JS_ExecuteScript(JSCONTEXT, Context::_get_global_template(), _script,
      p->_get_pvalue());
  return p;
}

void Template::Set(Handle<String> name, Handle<Data> value,
    PropertyAttribute attributes) {
}

Local<Value> TryCatch::Exception() const {
  return Local<Value> ();
}

bool TryCatch::HasCaught() const {
  return false;
}

Local<v8::Message> TryCatch::Message() const {
  return Local<v8::Message> ();
}

Handle<Value> TryCatch::ReThrow() {
  return Handle<Value> ();
}

Local<Value> TryCatch::StackTrace() const {
  return Local<Value> ();
}

TryCatch::TryCatch() {
}

TryCatch::~TryCatch() {
}

HeapStatistics::HeapStatistics() {
  used_heap_size_ = 0;
#ifdef __linux__
  long pages = sysconf(_SC_PHYS_PAGES);
  long page_size = sysconf(_SC_PAGE_SIZE);
  total_heap_size_ = pages * page_size;
#else
  total_heap_size_ = MAXIMUM_SIZE;
#endif
}

int V8::AdjustAmountOfExternalAllocatedMemory(int change_in_bytes) {
  int amount = amount_of_external_allocated_memory_ + change_in_bytes;
  if (change_in_bytes >= 0) {
    if (amount > amount_of_external_allocated_memory_) {
      amount_of_external_allocated_memory_ = amount;
    }
  } else {
    if (amount >= 0) {
      amount_of_external_allocated_memory_ = amount;
    }
  }
  return amount_of_external_allocated_memory_;
}

void V8::Dispose() {
  if (_runtime) {
    JS_DestroyRuntime(_runtime);
    _runtime = NULL;
  }
  JS_ShutDown();
}

void V8::GetHeapStatistics(HeapStatistics* heap_statistics) {
  heap_statistics->used_heap_size_ = amount_of_external_allocated_memory_;
}

const char* V8::GetVersion() {
  return JS_GetImplementationVersion();
}

bool V8::IdleNotification() {
  return false;
}

bool V8::Initialize() {
  return true;
}

void V8::LowMemoryNotification() {
}

void V8::SetFatalErrorHandler(FatalErrorCallback that) {
}

void V8::SetFlagsFromCommandLine(int* argc, char** argv, bool remove_flags) {
}

void V8::ClearWeak(internal::Object** global_handle) {
}

void V8::DisposeGlobal(internal::Object** global_handle) {
}

bool V8::IsGlobalNearDeath(internal::Object** global_handle) {
  return false;
}

bool V8::IsGlobalWeak(internal::Object** global_handle) {
  return false;
}

void V8::MakeWeak(internal::Object** global_handle, void* data,
    WeakReferenceCallback) {
}

JSRuntime* V8::_get_runtime() {
  if (!V8::_runtime) {
    JS_SetCStringsAreUTF8();
    V8::_runtime = JS_NewRuntime(MAXIMUM_SIZE);
  }
  return V8::_runtime;
}

Data::Data(JSContext* ctx, uint32_t type) :
  internal::Object(ctx, type) {
  if (!_get_context()) {
    _set_context(JSCONTEXT);
  }
  _set_value(JSVAL_VOID);
}

Value::Value(JSContext* ctx, uint32_t type) :
  Data(ctx, type) {

}

bool Value::BooleanValue() const {
  if (JSVAL_IS_BOOLEAN(_get_value())) {
    return (JSVAL_TO_BOOLEAN(_get_value()) == JS_TRUE) ? true : false;
  }
  JSBool bp;
  if (JS_ValueToBoolean(_get_context(), _get_value(), &bp) == JS_TRUE) {
    return (bp == JS_TRUE) ? true : false;
  }
  return false;
}

int32_t Value::Int32Value() const {
  if (JSVAL_IS_INT(_get_value())) {
    return JSVAL_TO_INT(_get_value());
  }
  int32_t ip;
  if (JS_ValueToECMAInt32(_get_context(), _get_value(), &ip) == JS_TRUE) {
    return ip;
  }
  return 0;
}

uint32_t Value::Uint32Value() const {
  if (JSVAL_IS_INT(_get_value())) {
    return JSVAL_TO_INT(_get_value());
  }
  uint32_t ip;
  if (JS_ValueToECMAUint32(_get_context(), _get_value(), &ip) == JS_TRUE) {
    return ip;
  }
  return 0;
}

int64_t Value::IntegerValue() const {
  // TODO: no 64bits??
  return Uint32Value();
}

bool Value::IsArray() const {
  return _is_type(_kTypeArray);
}

bool Value::IsBoolean() const {
  return _is_type(_kTypeBoolean);
}

bool Value::IsFalse() const {
  return !BooleanValue();
}

bool Value::IsFunction() const {
  return _is_type(_kTypeFunction);
}

bool Value::IsInt32() const {
  jsdouble dp;
  if (JS_ValueToNumber(_get_context(), _get_value(), &dp) == JS_TRUE) {
    return Int32Value() == (int32_t) dp;
  }
  return false;
}

bool Value::IsNumber() const {
  return _is_type(_kTypeNumber);
}

bool Value::IsObject() const {
  return _is_type(_kTypeObject);
}

bool Value::IsTrue() const {
  return BooleanValue();
}

bool Value::IsUint32() const {
  jsdouble dp;
  if (JS_ValueToNumber(_get_context(), _get_value(), &dp) == JS_TRUE) {
    return Uint32Value() == (uint32_t) dp;
  }
  return false;
}

bool Value::IsUndefined() const {
  return JSVAL_IS_VOID(_get_value());
}

Local<Integer> Value::ToInteger() const {
  return Integer::New(IntegerValue());
}

Local<v8::Object> Value::ToObject() const {
  Local<v8::Object> p = v8::Object::New(_get_object());
  return p;
}

Local<String> Value::ToString() const {
  JSString* str;
  str = JS_ValueToString(_get_context(), _get_value());
  if (str != NULL) {
    return String::New(JS_GetStringBytes(str));
  }
  return String::Empty();
}

}
