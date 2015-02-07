

#include "config.h"
#include "ScriptEventListener.h"

#include "Attribute.h"
#include "Document.h"
#include "EventListener.h"
#include "JSNode.h"
#include "Frame.h"
#include "XSSAuditor.h"
#include <runtime/JSLock.h>

using namespace JSC;

namespace WebCore {

static const String& eventParameterName(bool isSVGEvent)
{
    DEFINE_STATIC_LOCAL(const String, eventString, ("event"));
    DEFINE_STATIC_LOCAL(const String, evtString, ("evt"));
    return isSVGEvent ? evtString : eventString;
}

PassRefPtr<JSLazyEventListener> createAttributeEventListener(Node* node, Attribute* attr)
{
    ASSERT(node);
    ASSERT(attr);
    if (attr->isNull())
        return 0;

    int lineNumber = 1;
    String sourceURL;
    JSObject* wrapper = 0;
    
    // FIXME: We should be able to provide accurate source information for frameless documents, too (e.g. for importing nodes from XMLHttpRequest.responseXML).
    if (Frame* frame = node->document()->frame()) {
        ScriptController* scriptController = frame->script();
        if (!scriptController->canExecuteScripts())
            return 0;

        if (!scriptController->xssAuditor()->canCreateInlineEventListener(attr->localName().string(), attr->value())) {
            // This script is not safe to execute.
            return 0;
        }

        lineNumber = scriptController->eventHandlerLineNumber();
        sourceURL = node->document()->url().string();

        JSC::JSLock lock(SilenceAssertionsOnly);
        JSDOMGlobalObject* globalObject = toJSDOMGlobalObject(node->document(), mainThreadNormalWorld());
        wrapper = asObject(toJS(globalObject->globalExec(), globalObject, node));
    }

    return JSLazyEventListener::create(attr->localName().string(), eventParameterName(node->isSVGElement()), attr->value(), node, sourceURL, lineNumber, wrapper, mainThreadNormalWorld());
}

PassRefPtr<JSLazyEventListener> createAttributeEventListener(Frame* frame, Attribute* attr)
{
    if (!frame)
        return 0;

    ASSERT(attr);
    if (attr->isNull())
        return 0;

    int lineNumber = 1;
    String sourceURL;
    
    ScriptController* scriptController = frame->script();
    if (!scriptController->canExecuteScripts())
        return 0;

    if (!scriptController->xssAuditor()->canCreateInlineEventListener(attr->localName().string(), attr->value())) {
        // This script is not safe to execute.
        return 0;
    }

    lineNumber = scriptController->eventHandlerLineNumber();
    sourceURL = frame->document()->url().string();
    JSObject* wrapper = toJSDOMWindow(frame, mainThreadNormalWorld());
    return JSLazyEventListener::create(attr->localName().string(), eventParameterName(frame->document()->isSVGDocument()), attr->value(), 0, sourceURL, lineNumber, wrapper, mainThreadNormalWorld());
}

String getEventListenerHandlerBody(ScriptExecutionContext* context, ScriptState* scriptState, EventListener* eventListener)
{
    const JSEventListener* jsListener = JSEventListener::cast(eventListener);
    if (!jsListener)
        return "";
    JSC::JSObject* jsFunction = jsListener->jsFunction(context);
    if (!jsFunction)
        return "";
    return jsFunction->toString(scriptState);
}

} // namespace WebCore
