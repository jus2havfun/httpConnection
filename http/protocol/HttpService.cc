#include "HttpException.h"
#include "ByteArrayBuffer.h"
#include "CharArrayBuffer.h"
#include "Integer.h"
#include "StringUtils.h"
#include "HTTP.h"
#include "ProtocolVersion.h"
#include "HttpVersion.h"
#include "HttpHost.h"
#include "HttpStatus.h"
#include "Value.h"
#include "HttpParams.h"
#include "AbstractHttpParams.h"
#include "BasicHttpParams.h"
#include "CoreProtocolPNames.h"
#include "HttpProtocolParams.h"
#include "DefaultedHttpParams.h"
#include "RequestLine.h"
#include "NameValuePair.h"
#include "HeaderElement.h"
#include "Header.h"
#include "HeaderIterator.h"
#include "InputStream.h"
#include "OutputStream.h"
#include "StatusLine.h"
#include "HttpEntity.h"
#include "AbstractHttpEntity.h"
#include "ByteArrayEntity.h"
#include "ExecutionContext.h"
#include "HttpMessage.h"
#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpRequestHandler.h"
#include "HttpResponseFactory.h"
#include "HttpExpectationVerifier.h"
#include "HttpResponseInterceptor.h"
#include "HttpRequestInterceptor.h"
#include "HttpEntityEnclosingRequest.h"
#include "HttpRequestHandlerResolver.h"
#include "HttpProcessor.h"
#include "ConnectionReuseStrategy.h"
#include "HttpConnection.h"
#include "HttpServerConnection.h"
#include "CoreProtocolPNames.h"
#ifndef HTTPSERVICE_H
#include "HttpService.h"
#endif
void HttpService::doService(HttpRequest *request, HttpResponse *response, HttpContext *context) throw(HttpException, IOException) {
    HttpRequestHandler *handler = NULL;
    if (handlerResolver != NULL) {
        RequestLine *requestline = request->getRequestLine();
        std::string requestUri = requestline->getUri();
        requestline->unref();
        handler = handlerResolver->lookup(requestUri);
    }
    if (handler != NULL) {
        handler->handle(request, response, context);
    } else response->setStatusCode(HttpStatus::SC_NOT_IMPLEMENTED);
}
HttpService::HttpService(HttpProcessor *p, ConnectionReuseStrategy *c, HttpResponseFactory *r) {
    setHttpProcessor(p);
    setConnReuseStrategy(c);
    setResponseFactory(r);
}
void HttpService::setHttpProcessor(HttpProcessor *p) {
    if (p == NULL) throw IllegalArgumentException("HTTP processor may not be null.");
    processor = p;
}
void HttpService::setConnReuseStrategy(ConnectionReuseStrategy *c) {
    if (c == NULL) throw IllegalArgumentException("Connection reuse strategy may not be null");
    connStrategy = c;
}
void HttpService::setResponseFactory(HttpResponseFactory *r) {
    if (r == NULL) throw IllegalArgumentException("Response factory may not be null");
    responseFactory = r;
}
void HttpService::setHandlerResolver(HttpRequestHandlerResolver *h) {
    handlerResolver = h;
}
void HttpService::setExpectationVerifier(HttpExpectationVerifier *e) {
    expectationVerifier = e;
}
HttpParams *HttpService::getParams() {
    return params;
}
void HttpService::setParams(HttpParams *p) {
    params = p;
}
void HttpService::handleRequest(HttpServerConnection *conn, HttpContext *context) throw(IOException, HttpException) {
    context->setAttribute(ExecutionContext::HTTP_CONNECTION, new Value<HttpServerConnection *>(conn));
    HttpResponse* response = NULL;
    try {
        HttpRequest *request = conn->receiveRequestHeader();

        DefaultedHttpParams *_params = new DefaultedHttpParams(request->getParams(), params);
        request->setParams(_params);
        _params->unref();

        RequestLine *requestline = request->getRequestLine();
        ProtocolVersion *ver = requestline->getProtocolVersion();
        requestline->unref();
        if (!ver->lessEquals(*HttpVersion::HTTP_1_1)) {
            ver->unref();
            ver = HttpVersion::HTTP_1_1;
            ver->ref();
        }

        HttpEntityEnclosingRequest* httpreq = dynamic_cast<HttpEntityEnclosingRequest *>(request);
        if (httpreq != NULL) {
            if (httpreq->expectContinue()) {
                response = responseFactory->newHttpResponse(ver, HttpStatus::SC_CONTINUE, context);
                DefaultedHttpParams *_params = new DefaultedHttpParams(response->getParams(), params);
                response->setParams(_params);
                _params->unref();
                if (expectationVerifier != NULL) {
                    try {
                        expectationVerifier->verify(request, response, context);
                    } catch (const HttpException &ex) {
                        response = responseFactory->newHttpResponse(HttpVersion::HTTP_1_0, HttpStatus::SC_INTERNAL_SERVER_ERROR, context);
                        DefaultedHttpParams *_params =new DefaultedHttpParams(response->getParams(), params);
                        response->setParams(_params);
                        _params->unref();
                        handleException(const_cast<HttpException *>(&ex), response);
                    }
                }
                if (response->getStatusLine()->getStatusCode() < 200) {
                    conn->sendResponseHeader(response);
                    conn->flush();
                    response = NULL;
                    conn->receiveRequestEntity(httpreq);
                }
            } else {
                conn->receiveRequestEntity(httpreq);
            }
        }
        if (response == NULL) {
            response = responseFactory->newHttpResponse(ver, HttpStatus::SC_OK, context);
            DefaultedHttpParams *_params = new DefaultedHttpParams(response->getParams(), params);
            response->setParams(_params);
            _params->unref();
            context->setAttribute(ExecutionContext::HTTP_REQUEST, new Value<HttpRequest *>(request));
            context->setAttribute(ExecutionContext::HTTP_RESPONSE, new Value<HttpResponse *>(response));
            processor->process(request, context);
            doService(request, response, context);
        }
        httpreq = dynamic_cast<HttpEntityEnclosingRequest *>(request);
        if (httpreq != NULL) {
            HttpEntity *entity = httpreq->getEntity();
            if (entity != NULL) entity->consumeContent();
        }
    } catch (const HttpException &ex) {
        response = responseFactory->newHttpResponse(HttpVersion::HTTP_1_0, HttpStatus::SC_INTERNAL_SERVER_ERROR, context);
        DefaultedHttpParams *_params = new DefaultedHttpParams(response->getParams(), params);
        response->setParams(_params);
        _params->unref();
        handleException(const_cast<HttpException *>(&ex), response);
    }
    processor->process(response, context);
    conn->sendResponseHeader(response);
    conn->sendResponseEntity(response);
    conn->flush();
    if (!connStrategy->keepAlive(response, context)) {
        conn->close();
    }
    request->unref();
    response->unref();
    ver->unref();
}
void HttpService::handleException(HttpException *ex, HttpResponse *response) {
    MethodNotSupportedException *mnse = dynamic_cast<MethodNotSupportedException *>(ex);
    UnsupportedHttpVersionException *uhve = dynamic_cast<UnsupportedHttpVersionException *>(ex);
    ProtocolException *pe = dynamic_cast<ProtocolException *>(ex);
    if (mnse != NULL) response->setStatusCode(HttpStatus::SC_NOT_IMPLEMENTED);
    else if (uhve != NULL) response->setStatusCode(HttpStatus::SC_HTTP_VERSION_NOT_SUPPORTED);
    else if (pe != NULL) response->setStatusCode(HttpStatus::SC_BAD_REQUEST);
    else response->setStatusCode(HttpStatus::SC_INTERNAL_SERVER_ERROR);
    char msg[512];
    ex->describe(msg, 512);
    ByteArrayEntity *entity = new ByteArrayEntity((unsigned char *)msg, 512);
    entity->setContentType("text/plain; charset=US-ASCII");
    response->setEntity(entity);
}
