#include <iostream>
#include <unistd.h>
#include <rapidhttp/document.h>
#include <gtest/gtest.h>
using namespace std;
using namespace rapidhttp;

static std::string c_http_response = 
"HTTP/1.1 200 OK\r\n"
"Accept: XAccept\r\n"
"Host: domain.com\r\n"
"Connection: Keep-Alive\r\n"
"Content-Length: 3\r\n"
"\r\nxyz";

static std::string c_http_response_2 = 
"HTTP/1.1 404 Not Found\r\n"
"Accept: XAccept\r\n"
"Host: domain.com\r\n"
"User-Agent: gtest.proxy\r\n"
"\r\n";

// 错误的协议头
static std::string c_http_response_err_1 = 
"HTTP/1.1200 OK\r\n"
"Accept: XAccept\r\n"
"Host: domain.com\r\n"
"User-Agent: gtest.proxy\r\n"
"\r\n";

// 错误的协议头
static std::string c_http_response_err_2 = 
"HTTP/1.1 200OK\r\n"
"Accept: XAccept\r\n"
"Host: domain.com\r\n"
"User-Agent: gtest.proxy\r\n"
"\r\n";

// 一部分协议头, 缺少一个\r\n
static std::string c_http_response_err_3 = 
"HTTP/1.1 200 OK\r\n"
"Accept: XAccept\r\n"
"Host: domain.com\r\n"
"Content-Length: 0\r\n"
"User-Agent: gtest.proxy\r\n";


template <typename DocType>
void test_parse_response()
{
    DocType doc(rapidhttp::Response);
    size_t bytes = doc.PartailParse(c_http_response);
    EXPECT_EQ(bytes, c_http_response.size());
    EXPECT_TRUE(!doc.ParseError());
    EXPECT_TRUE(doc.ParseDone());

    EXPECT_EQ(doc.GetStatus(), "OK");
    EXPECT_EQ(doc.GetStatusCode(), 200);
    EXPECT_EQ(doc.GetMajor(), 1);
    EXPECT_EQ(doc.GetMinor(), 1);
    EXPECT_EQ(doc.GetField("Accept"), "XAccept");
    EXPECT_EQ(doc.GetField("Host"), "domain.com");
    EXPECT_EQ(doc.GetField("Connection"), "Keep-Alive");
    EXPECT_EQ(doc.GetField("User-Agent"), "");
    EXPECT_EQ(doc.GetBody(), "xyz");

    for (int i = 0; i < 10; ++i) {
        size_t bytes = doc.PartailParse(c_http_response.c_str(), c_http_response.size());
        EXPECT_EQ(bytes, c_http_response.size());
        EXPECT_TRUE(!doc.ParseError());
        EXPECT_TRUE(doc.ParseDone());
    }

    EXPECT_EQ(doc.GetStatus(), "OK");
    EXPECT_EQ(doc.GetStatusCode(), 200);
    EXPECT_EQ(doc.GetMajor(), 1);
    EXPECT_EQ(doc.GetMinor(), 1);
    EXPECT_EQ(doc.GetField("Accept"), "XAccept");
    EXPECT_EQ(doc.GetField("Host"), "domain.com");
    EXPECT_EQ(doc.GetField("Connection"), "Keep-Alive");
    EXPECT_EQ(doc.GetField("User-Agent"), "");

    bytes = doc.PartailParse(c_http_response_2);
    EXPECT_EQ(bytes, c_http_response_2.size());
    EXPECT_TRUE(!doc.ParseError());
    EXPECT_TRUE(!doc.ParseDone());
    EXPECT_TRUE(doc.PartailParseEof());
    EXPECT_TRUE(!doc.ParseError());
    EXPECT_TRUE(doc.ParseDone());

    EXPECT_EQ(doc.GetStatus(), "Not Found");
    EXPECT_EQ(doc.GetStatusCode(), 404);
    EXPECT_EQ(doc.GetMajor(), 1);
    EXPECT_EQ(doc.GetMinor(), 1);
    EXPECT_EQ(doc.GetField("Accept"), "XAccept");
    EXPECT_EQ(doc.GetField("Host"), "domain.com");
    EXPECT_EQ(doc.GetField("Connection"), "");
    EXPECT_EQ(doc.GetField("User-Agent"), "gtest.proxy");

    bytes = doc.PartailParse(c_http_response_err_1);
    EXPECT_TRUE(doc.ParseError());

    bytes = doc.PartailParse(c_http_response_err_2);
    EXPECT_TRUE(doc.ParseError());

    // partail parse logic
    cout << "parse partail" << endl;
    bytes = doc.PartailParse(c_http_response_err_3);
    EXPECT_EQ(bytes, c_http_response_err_3.size());
    EXPECT_TRUE(!doc.ParseError());
    EXPECT_TRUE(!doc.ParseDone());

    bytes = doc.PartailParse("\r\n");
    EXPECT_EQ(bytes, 2);
    EXPECT_TRUE(!doc.ParseError());
    EXPECT_TRUE(doc.ParseDone());

    for (size_t pos = 0; pos < c_http_response.size(); ++pos)
    {
//        cout << "parse response split by " << pos << endl;
//        cout << "first partail: " << c_http_response.substr(0, pos) << endl << endl;
        std::string fp = c_http_response.substr(0, pos);
        size_t bytes = doc.PartailParse(fp);
//        EXPECT_EQ(bytes, pos);
//        EXPECT_EQ(doc.ParseError().value(), 1);
        EXPECT_FALSE(doc.ParseDone());

        std::string sp = c_http_response.substr(bytes);
        bytes += doc.PartailParse(sp);
        EXPECT_EQ(bytes, c_http_response.size());
        EXPECT_TRUE(!doc.ParseError());
        EXPECT_TRUE(doc.ParseDone());

        EXPECT_EQ(doc.GetStatus(), "OK");
        EXPECT_EQ(doc.GetStatusCode(), 200);
        EXPECT_EQ(doc.GetMajor(), 1);
        EXPECT_EQ(doc.GetMinor(), 1);
        EXPECT_EQ(doc.GetField("Accept"), "XAccept");
        EXPECT_EQ(doc.GetField("Host"), "domain.com");
        EXPECT_EQ(doc.GetField("Connection"), "Keep-Alive");
        EXPECT_EQ(doc.GetField("User-Agent"), "");
    }

    char buf[256] = {};
    bool b = doc.Serialize(buf, sizeof(buf));
    EXPECT_TRUE(b);
    bytes = doc.ByteSize();
    EXPECT_EQ(bytes, c_http_response.size());
    EXPECT_EQ(c_http_response, buf);
}

void copyto_response()
{
    std::string s = c_http_response;

    rapidhttp::HttpDocumentRef doc(rapidhttp::Response);
    size_t bytes = doc.PartailParse(s);
    EXPECT_EQ(bytes, s.size());
    EXPECT_TRUE(!doc.ParseError());

#define _CHECK_DOC(doc) \
    EXPECT_EQ(doc.GetStatus(), "OK"); \
    EXPECT_EQ(doc.GetStatusCode(), 200); \
    EXPECT_EQ(doc.GetMajor(), 1); \
    EXPECT_EQ(doc.GetMinor(), 1); \
    EXPECT_EQ(doc.GetField("Accept"), "XAccept"); \
    EXPECT_EQ(doc.GetField("Host"), "domain.com"); \
    EXPECT_EQ(doc.GetField("Connection"), "Keep-Alive"); \
    EXPECT_EQ(doc.GetField("User-Agent"), ""); \
    EXPECT_EQ(doc.GetBody(), "xyz")

    _CHECK_DOC(doc);

    rapidhttp::HttpDocumentRef doc2(rapidhttp::Response);
    doc.CopyTo(doc2);
    _CHECK_DOC(doc2);

    rapidhttp::HttpDocument doc3(rapidhttp::Response);
    doc2.CopyTo(doc3);
    _CHECK_DOC(doc3);

    rapidhttp::HttpDocument doc4(rapidhttp::Response);
    doc2.CopyTo(doc4);
    _CHECK_DOC(doc4);
    doc3.CopyTo(doc4);
    _CHECK_DOC(doc4);

    s = "xx";
    _CHECK_DOC(doc3);
    _CHECK_DOC(doc4);
}

TEST(parse, response)
{
    test_parse_response<rapidhttp::HttpDocument>();
    test_parse_response<rapidhttp::HttpDocumentRef>();
    copyto_response();
}
