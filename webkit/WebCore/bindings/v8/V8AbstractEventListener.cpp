

#include "config.h"
#include "V8AbstractEventListener.h"

#include "DateExtension.h"
#include "Document.h"
#include "Event.h"
#include "Frame.h"
#include "V8Binding.h"
#include "V8Event.h"
#include "V8EventListenerList.h"
#include "V8Proxy.h"
#include "V8Utilities.h"
#include "WorkerContext.h"
#include "WorkerContextExecutionProxy.h"

namespace WebCore {

static void weakEventListenerCallback(v8::Persistent<v8::Value>, void* parameter)
{
    V8AbstractEventListener* listener = static_cast<V8AbstractEventListener*>(parameter);
    listener->disposeListenerObject();
}

V8AbstractEventListener::V8AbstractEventListener(bool isAttribute, const WorldContextHandle& worldContext)
    : EventListener(JSEventListenerType)
    , m_isWeak(true)
    , m_isAttribute(isAttribute)
    , m_worldContext(worldContext)
{
}

V8AbstractEventListener::~V8AbstractEventListener()
{
    if (!m_listener.IsEmpty()) {
        v8::HandleScope scope;
        v8::Local<v8::Object> listener = v8::Local<v8::Object>::New(m_listener);
        V8EventListenerList::clearWrapper(listener, m_isAttribute);
    }
    disposeListenerObject();
}

void V8AbstractEventListener::handleEvent(ScriptExecutionContext* context, Event* event)
{
    // The callback function on XMLHttpRequest can clear the event listener and destroys 'this' object. Keep a local reference to it.
    // See issue 889829.
    RefPtr<V8AbstractEventListener> protect(this);

    v8::HandleScope handleScope;

    v8::Local<v8::Context> v8Context = toV8Context(context, worldContext());
    if (v8Context.IsEmpty())
        return;

    // Enter the V8 context in which to perform the event handling.
    v8::Context::Scope scope(v8Context);

    // Get the V8 wrapper for the event object.
    v8::Handle<v8::Value> jsEvent = toV8(event);

    invokeEventHandler(context, event, jsEvent);

    Document::updateStyleForAllDocuments();
}

void V8AbstractEventListener::disposeListenerObject()
{
    if (!m_listener.IsEmpty()) {
#ifndef NDEBUG
        V8GCController::unregisterGlobalHandle(this, m_listener);
#endif
        m_listener.Dispose();
        m_listener.Clear();
    }
}

void V8AbstractEventListener::setListenerObject(v8::Handle<v8::Object> listener)
{
    disposeListenerObject();
    m_listener = v8::Persistent<v8::Object>::New(listener);
#ifndef NDEBUG
    V8GCController::registerGlobalHandle(EVENT_LISTENER, this, m_listener);
#endif
    if (m_isWeak)
        m_listener.MakeWeak(this, &weakEventListenerCallback);
}

void V8AbstractEventListener::invokeEventHandler(ScriptExecutionContext* context, Event* event, v8::Handle<v8::Value> jsEvent)
{

    v8::Local<v8::Context> v8Context = toV8Context(context, worldContext());
    if (v8Context.IsEmpty())
        return;

    // We push the event being processed into the global object, so that it can be exposed by DOMWindow's bindings.
    v8::Local<v8::String> eventSymbol = v8::String::NewSymbol("event");
    v8::Local<v8::Value> returnValue;

    // In beforeunload/unload handlers, we want to avoid sleeps which do tight loops of calling Date.getTime().
    if (event->type() == "beforeunload" || event->type() == "unload")
        DateExtension::get()->setAllowSleep(false);

    {
        // Catch exceptions thrown in the event handler so they do not propagate to javascript code that caused the event to fire.
        v8::TryCatch tryCatch;
        tryCatch.SetVerbose(true);

        // Save the old 'event' property so we can restore it later.
        v8::Local<v8::Value> savedEvent = v8Context->Global()->GetHiddenValue(eventSymbol);
        tryCatch.Reset();

        // Make the event available in the global object, so DOMWindow can expose it.
        v8Context->Global()->SetHiddenValue(eventSymbol, jsEvent);
        tryCatch.Reset();

        // Call the event handler.
        returnValue = callListenerFunction(context, jsEvent, event);
        if (!tryCatch.CanContinue())
            return;

        // If an error occurs while handling the event, it should be reported.
        if (tryCatch.HasCaught()) {
            reportException(0, tryCatch);
            tryCatch.Reset();
        }

        // Restore the old event. This must be done for all exit paths through this method.
        if (savedEvent.IsEmpty())
            v8Context->Global()->SetHiddenValue(eventSymbol, v8::Undefined());
        else
            v8Context->Global()->SetHiddenValue(eventSymbol, savedEvent);
        tryCatch.Reset();
    }

    if (event->type() == "beforeunload" || event->type() == "unload")
        DateExtension::get()->setAllowSleep(true);

    ASSERT(!V8Proxy::handleOutOfMemory() || returnValue.IsEmpty());

    if (returnValue.IsEmpty())
        return;

    if (!returnValue->IsNull() && !returnValue->IsUndefined() && event->storesResultAsString())
        event->storeResult(toWebCoreString(returnValue));

    // Prevent default action if the return value is false;
    // FIXME: Add example, and reference to bug entry.
    if (m_isAttribute && returnValue->IsBoolean() && !returnValue->BooleanValue())
        event->preventDefault();
}

v8::Local<v8::Object> V8AbstractEventListener::getReceiverObject(Event* event)
{
    if (!m_listener.IsEmpty() && !m_listener->IsFunction())
        return v8::Local<v8::Object>::New(m_listener);

    EventTarget* target = event->currentTarget();
    v8::Handle<v8::Value> value = V8DOMWrapper::convertEventTargetToV8Object(target);
    if (value.IsEmpty())
        return v8::Local<v8::Object>();
    return v8::Local<v8::Object>::New(v8::Handle<v8::Object>::Cast(value));
}

} // namespace WebCore
