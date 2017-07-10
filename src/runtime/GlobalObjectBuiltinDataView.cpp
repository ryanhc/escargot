/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#if ESCARGOT_ENABLE_TYPEDARRAY

#include "Escargot.h"
#include "GlobalObject.h"
#include "Context.h"
#include "TypedArrayObject.h"
#include "DataViewObject.h"

namespace Escargot {

#define FOR_EACH_DATAVIEW_TYPES(F) \
    F(Float32)                     \
    F(Float64)                     \
    F(Int8)                        \
    F(Int16)                       \
    F(Int32)                       \
    F(Uint8)                       \
    F(Uint16)                      \
    F(Uint32)

Value builtinDataViewConstructor(ExecutionState& state, Value thisValue, size_t argc, Value* argv, bool isNewExpression)
{
    if (!isNewExpression) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, state.context()->staticStrings().DataView.string(), false, String::emptyString, errorMessage_GlobalObject_NotExistNewInDataViewConstructor);
    }
    ArrayBufferView* obj = thisValue.asObject()->asArrayBufferView();
    if (!(argv[0].isObject() && argv[0].asPointerValue()->isArrayBufferObject())) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, state.context()->staticStrings().DataView.string(), false, String::emptyString, errorMessage_GlobalObject_ThisNotArrayBufferObject);
    }

    ArrayBufferObject* buffer = argv[0].asObject()->asArrayBufferObject();

    double byteOffset = 0;
    if (argc >= 2) {
        Value& val = argv[1];
        double numberOffset = val.toNumber(state);
        byteOffset = Value(numberOffset).toInteger(state);
        if (numberOffset != byteOffset || byteOffset < 0) {
            ErrorObject::throwBuiltinError(state, ErrorObject::RangeError, state.context()->staticStrings().DataView.string(), false, String::emptyString, errorMessage_GlobalObject_InvalidArrayBufferOffset);
        }
    }

    if (buffer->isDetachedBuffer()) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, state.context()->staticStrings().DataView.string(), false, String::emptyString, "%s: ArrayBuffer is detached buffer");
    }

    double bufferByteLength = buffer->bytelength();

    if (byteOffset > bufferByteLength) {
        ErrorObject::throwBuiltinError(state, ErrorObject::RangeError, state.context()->staticStrings().DataView.string(), false, String::emptyString, errorMessage_GlobalObject_InvalidArrayBufferOffset);
    }

    double byteLength = bufferByteLength - byteOffset;

    if (argc >= 3) {
        Value& val = argv[2];
        if (!val.isUndefined()) {
            byteLength = val.toLength(state);
            if (byteOffset + byteLength > bufferByteLength) {
                ErrorObject::throwBuiltinError(state, ErrorObject::RangeError, state.context()->staticStrings().DataView.string(), false, String::emptyString, errorMessage_GlobalObject_InvalidArrayBufferOffset);
            }
        }
    }

    obj->setBuffer(buffer, byteOffset, byteLength);

    return obj;
}

#define DECLARE_DATAVIEW_GETTER(Name)                                                                                             \
    static Value builtinDataViewGet##Name(ExecutionState& state, Value thisValue, size_t argc, Value* argv, bool isNewExpression) \
    {                                                                                                                             \
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, DataView, get##Name);                                                          \
        if (!(thisObject->isDataViewObject())) {                                                                                  \
            ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, state.context()->staticStrings().DataView.string(),     \
                                           true, state.context()->staticStrings().get##Name.string(),                             \
                                           errorMessage_GlobalObject_ThisNotDataViewObject);                                      \
        }                                                                                                                         \
        if (argc < 2) {                                                                                                           \
            return thisObject->asDataViewObject()->getViewValue(state, argv[0], Value(false), TypedArrayType::Name);              \
        } else {                                                                                                                  \
            return thisObject->asDataViewObject()->getViewValue(state, argv[0], argv[1], TypedArrayType::Name);                   \
        }                                                                                                                         \
    }

#define DECLARE_DATAVIEW_SETTER(Name)                                                                                             \
    static Value builtinDataViewSet##Name(ExecutionState& state, Value thisValue, size_t argc, Value* argv, bool isNewExpression) \
    {                                                                                                                             \
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, DataView, get##Name);                                                          \
        if (!(thisObject->isDataViewObject())) {                                                                                  \
            ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, state.context()->staticStrings().DataView.string(),     \
                                           true, state.context()->staticStrings().set##Name.string(),                             \
                                           errorMessage_GlobalObject_ThisNotDataViewObject);                                      \
        }                                                                                                                         \
        if (argc < 3) {                                                                                                           \
            return thisObject->asDataViewObject()->setViewValue(state, argv[0], Value(false), TypedArrayType::Name, argv[1]);     \
        } else {                                                                                                                  \
            return thisObject->asDataViewObject()->setViewValue(state, argv[0], argv[2], TypedArrayType::Name, argv[1]);          \
        }                                                                                                                         \
    }

FOR_EACH_DATAVIEW_TYPES(DECLARE_DATAVIEW_GETTER);
FOR_EACH_DATAVIEW_TYPES(DECLARE_DATAVIEW_SETTER);

static Value builtinDataViewBufferGetter(ExecutionState& state, Value thisValue, size_t argc, Value* argv, bool isNewExpression)
{
    if (LIKELY(thisValue.isPointerValue() && thisValue.asPointerValue()->isDataViewObject())) {
        return Value(thisValue.asObject()->asArrayBufferView()->buffer());
    }
    ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, "get DataView.prototype.buffer called on incompatible receiver");
    RELEASE_ASSERT_NOT_REACHED();
}

static Value builtinDataViewByteLengthGetter(ExecutionState& state, Value thisValue, size_t argc, Value* argv, bool isNewExpression)
{
    if (LIKELY(thisValue.isPointerValue() && thisValue.asPointerValue()->isDataViewObject())) {
        return Value(thisValue.asObject()->asArrayBufferView()->bytelength());
    }
    ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, "get DataView.prototype.byteLength called on incompatible receiver");
    RELEASE_ASSERT_NOT_REACHED();
}

static Value builtinDataViewByteOffsetGetter(ExecutionState& state, Value thisValue, size_t argc, Value* argv, bool isNewExpression)
{
    if (LIKELY(thisValue.isPointerValue() && thisValue.asPointerValue()->isDataViewObject())) {
        return Value(thisValue.asObject()->asArrayBufferView()->byteoffset());
    }
    ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, "get DataView.prototype.byteOffset called on incompatible receiver");
    RELEASE_ASSERT_NOT_REACHED();
}

void GlobalObject::installDataView(ExecutionState& state)
{
    m_dataView = new FunctionObject(state, NativeFunctionInfo(state.context()->staticStrings().DataView, builtinDataViewConstructor, 3, [](ExecutionState& state, CodeBlock* codeBlock, size_t argc, Value* argv) -> Object* {
                                        return new DataViewObject(state);
                                    }),
                                    FunctionObject::__ForBuiltin__);
    m_dataView->markThisObjectDontNeedStructureTransitionTable(state);
    m_dataView->setPrototype(state, m_functionPrototype);
    m_dataViewPrototype = new DataViewObject(state);
    m_dataViewPrototype->markThisObjectDontNeedStructureTransitionTable(state);
    m_dataViewPrototype->setPrototype(state, m_objectPrototype);
    m_dataView->setFunctionPrototype(state, m_dataViewPrototype);
    m_dataViewPrototype->defineOwnProperty(state, ObjectPropertyName(state.context()->staticStrings().constructor), ObjectPropertyDescriptor(m_dataView, (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));

#define DATAVIEW_DEFINE_GETTER(Name)                                                                                                                                                                                             \
    m_dataViewPrototype->defineOwnProperty(state, ObjectPropertyName(state.context()->staticStrings().get##Name),                                                                                                                \
                                           ObjectPropertyDescriptor(new FunctionObject(state, NativeFunctionInfo(state.context()->staticStrings().get##Name, builtinDataViewGet##Name, 1, nullptr, NativeFunctionInfo::Strict)), \
                                                                    (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));

#define DATAVIEW_DEFINE_SETTER(Name)                                                                                                                                                                                             \
    m_dataViewPrototype->defineOwnProperty(state, ObjectPropertyName(state.context()->staticStrings().set##Name),                                                                                                                \
                                           ObjectPropertyDescriptor(new FunctionObject(state, NativeFunctionInfo(state.context()->staticStrings().set##Name, builtinDataViewSet##Name, 2, nullptr, NativeFunctionInfo::Strict)), \
                                                                    (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));

    FOR_EACH_DATAVIEW_TYPES(DATAVIEW_DEFINE_GETTER);
    FOR_EACH_DATAVIEW_TYPES(DATAVIEW_DEFINE_SETTER);

    const StaticStrings* strings = &state.context()->staticStrings();

    {
        JSGetterSetter gs(
            new FunctionObject(state, NativeFunctionInfo(strings->getBuffer, builtinDataViewBufferGetter, 0, nullptr, NativeFunctionInfo::Strict)),
            Value(Value::EmptyValue));
        ObjectPropertyDescriptor bufferDesc(gs, ObjectPropertyDescriptor::ConfigurablePresent);
        m_dataViewPrototype->defineOwnProperty(state, ObjectPropertyName(strings->buffer), bufferDesc);
    }

    {
        JSGetterSetter gs(
            new FunctionObject(state, NativeFunctionInfo(strings->getbyteLength, builtinDataViewByteLengthGetter, 0, nullptr, NativeFunctionInfo::Strict)),
            Value(Value::EmptyValue));
        ObjectPropertyDescriptor byteLengthDesc(gs, ObjectPropertyDescriptor::ConfigurablePresent);
        m_dataViewPrototype->defineOwnProperty(state, ObjectPropertyName(strings->byteLength), byteLengthDesc);
    }

    {
        JSGetterSetter gs(
            new FunctionObject(state, NativeFunctionInfo(strings->getbyteOffset, builtinDataViewByteOffsetGetter, 0, nullptr, NativeFunctionInfo::Strict)),
            Value(Value::EmptyValue));
        ObjectPropertyDescriptor byteOffsetDesc(gs, ObjectPropertyDescriptor::ConfigurablePresent);
        m_dataViewPrototype->defineOwnProperty(state, ObjectPropertyName(strings->byteOffset), byteOffsetDesc);
    }

    defineOwnProperty(state, ObjectPropertyName(state.context()->staticStrings().DataView),
                      ObjectPropertyDescriptor(m_dataView, (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));
}
}
#endif