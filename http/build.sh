SRC_HOME=$PWD
g++ -g -m64 -std="c++11" -o main \
                        -I${SRC_HOME}/inc -I${SRC_HOME}/protocol -I${SRC_HOME}/util \
                        -I${SRC_HOME}/params -I${SRC_HOME}/message -I${SRC_HOME}/client/params \
                        -I${SRC_HOME}/net \
                        ${SRC_HOME}/util/StringUtils.cc \
                        ${SRC_HOME}/Main.cc \
                        ${SRC_HOME}/protocol/HTTP.cc \
                        ${SRC_HOME}/HttpHost.cc \
                        ${SRC_HOME}/HttpVersion.cc \
                        ${SRC_HOME}/net/URI.cc \
                        ${SRC_HOME}/net/NameResolver.cc \
                        ${SRC_HOME}/net/InetSocketAddress.cc \
                        ${SRC_HOME}/net/URLStreamHandler.cc \
                        ${SRC_HOME}/net/HttpHandler.cc \
                        ${SRC_HOME}/net/HttpsHandler.cc \
                        ${SRC_HOME}/net/FtpHandler.cc \
                        ${SRC_HOME}/net/URL.cc \
                        ${SRC_HOME}/ProtocolVersion.cc \
                        ${SRC_HOME}/util/CharArrayBuffer.cc \
                        ${SRC_HOME}/util/ByteArrayBuffer.cc \
                        ${SRC_HOME}/params/CoreProtocolPNames.cc \
                        ${SRC_HOME}/params/HttpProtocolParams.cc \
                        ${SRC_HOME}/params/AbstractHttpParams.cc \
                        ${SRC_HOME}/params/BasicHttpParams.cc \
                        ${SRC_HOME}/params/Value.cc \
                        ${SRC_HOME}/client/params/CookiePolicy.cc \
                        ${SRC_HOME}/client/params/ClientPNames.cc \
                        ${SRC_HOME}/client/params/HttpClientParams.cc \
                        ${SRC_HOME}/message/ParserCursor.cc \
                        ${SRC_HOME}/message/BasicHeader.cc \
                        ${SRC_HOME}/message/BasicHeaderValueFormatter.cc \
                        ${SRC_HOME}/message/BasicHeaderValueParser.cc \
                        ${SRC_HOME}/message/BasicHeaderElement.cc \
                        ${SRC_HOME}/message/BasicNameValuePair.cc \
                        ${SRC_HOME}/message/BasicLineFormatter.cc \
                        ${SRC_HOME}/message/BasicRequestLine.cc \
                        ${SRC_HOME}/message/AbstractHttpMessage.cc \
                        ${SRC_HOME}/message/BasicHttpRequest.cc \
                        ${SRC_HOME}/message/HeaderGroup.cc \
                        ${SRC_HOME}/message/BasicListHeaderIterator.cc \
                        ${SRC_HOME}/message/BasicHeaderIterator.cc \
                        ${SRC_HOME}/message/BasicHeaderElementIterator.cc \
                        ${SRC_HOME}/message/BufferedHeader.cc \
                        ${SRC_HOME}/message/BasicStatusLine.cc \
                        ${SRC_HOME}/message/BasicLineParser.cc \
                        ${SRC_HOME}/protocol/HttpContext.cc \
                        ${SRC_HOME}/protocol/ExecutionContext.cc \
                        ${SRC_HOME}/protocol/BasicHttpContext.cc \
                        ${SRC_HOME}/protocol/DefaultedHttpContext.cc \
                        ${SRC_HOME}/protocol/HttpDateGenerator.cc \
                        ${SRC_HOME}/protocol/UriPatternMatcher.cc \
                        ${SRC_HOME}/protocol/SyncBasicHttpContext.cc -lpthread -lnsl -lresolv
