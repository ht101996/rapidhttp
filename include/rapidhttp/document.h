#pragma once

#include <string>
#include <map>
#include <vector>
#include <stdint.h>
#include <rapidhttp/constants.h>
#include <rapidhttp/stringref.h>
#include <rapidhttp/error_code.h>

namespace rapidhttp {

// Document field
//struct Field
//{
//    enum eFlags
//    {
//        e_Update        = 0x1,
//        e_IntOrString   = 0x2,  // 0: string 1: integer.
//    };
//
//    unsigned char flags;
//    StringRef key;
//    union {
//        StringRef str;
//        uint32_t integer;
//    };
//    std::string key_storage;
//    std::string value_storage;
//
//    Field() : flags(0), key(), str() {}
//    explicit Field(StringRef const& k, int i) : flags(e_IntOrString), key(k), integer(i) {}
//    explicit Field(StringRef const& k, StringRef const& s) : flags(0), key(k), str(s) {}
//
//    bool IsUpdate() { return flags & e_Update; }
//    void SetUpdate() { flags |= e_Update; }
//
//    bool IsString() { return (flags & e_IntOrString) == 0; }
//    void SetString(const char* buf)
//    {
//        flags &= ~e_IntOrString;
//        value_storage = buf;
//        str.SetString(value_storage);
//    }
//    void SetStringRef(StringRef const& strref)
//    {
//        flags &= ~e_IntOrString;
//        str = strref;
//    }
//
//    bool IsUInt() { return flags & e_IntOrString; }
//    void SetUInt(uint32_t i)
//    {
//        flags |= e_IntOrString;
//        integer = i;
//    }
//
//    StringRef& GetStringRef() { assert(IsString()); return str; }
//    uint32_t& GetInt() { assert(IsUInt()); return integer; }
//
//    size_t ByteSize() {
//        if (IsString())
//            return str.size();
//        else {
//            return UIntegerByteSize(integer);
//        }
//    }
//
//    void OnCopy(const char* origin, const char* dest)
//    {
//        key.OnCopy(origin, dest);
//        if (IsString())
//            str.OnCopy(origin, dest);
//    }
//
//    void Storage()
//    {
//        if (IsString()) {
//            if (str.c_str() != storage_.c_str()) {
//                storage_ = str.ToString();
//                str.SetString(storage_);
//            }
//        }
//    }
//};

// Http document class.
// Copy-on-write.
class HttpHeaderDocument
{
public:
    enum DocumentType
    {
        Request,
        Response,
    };
    // 协议头
    // e.g: GET /uri HTTP/1.1
    // or
    // e.g: HTTP/1.1 200 OK
    enum eProtoFieldsIndex
    {
        e_method = 0,       // 方法 @Request
        e_response_str = 0, // URI @Request
        e_uri = 1,          // 响应字符串 @Resposne
        e_code = 1,         // 响应Code @Resposne
        e_ver_major = 2,    // 协议主版本号
        e_ver_minor = 3,    // 协议次版本号
        e_proto_fields_count = 4,
    };
    // 常用头部域
    enum eHeaderFields
    {
        e_Host = e_proto_fields_count,
        e_Accept,
        e_User_Agent,
        e_Authorization,
        e_Expect,
        e_From,
        e_Proxy_Authorization,
        e_Referer,
        e_Range,
        e_Accept_Charset,
        e_Accept_Encoding,
        e_Accept_Language,
        e_Accept_Range,
        e_Max_Forwards,
        e_header_fields_count,
    };

    enum class eParseState
    {
        init,
        method,
        uri,
        version,
        code,
        response_str,
        fields,
        done,
    };

    explicit HttpHeaderDocument(DocumentType type) : type_(type) {}
//    HttpHeaderDocument(HttpHeaderDocument const& other);
//    HttpHeaderDocument(HttpHeaderDocument && other);
//    HttpHeaderDocument& operator=(HttpHeaderDocument const& other);
//    HttpHeaderDocument& operator=(HttpHeaderDocument && other);

    /// ------------------- parse/generate ---------------------
    /// 流式解析
    // @buf_ref: 外部传入的缓冲区首地址
    // @len: 缓冲区长度
    // @returns：解析完成返回error_code=0, 解析一半返回error_code=1, 解析失败返回其他错误码.
    inline size_t PartailParse(const char* buf_ref, size_t len);
    inline size_t PartailParse(std::string const& buf);
    inline void ResetPartailParse();

    // 返回解析错误码
    inline std::error_code ParseError();

    // 保存
//    void Storage();

    inline bool IsInitialized() const;

    inline size_t ByteSize() const;

    inline bool Serialize(char *buf, size_t len);
    /// --------------------------------------------------------

    /// ------------------- fields get/set ---------------------
    inline std::string const& GetMethod();
    inline void SetMethod(const char* m);
    inline void SetMethod(std::string const& m);

    inline std::string const& GetUri();
    inline void SetUri(const char* m);
    inline void SetUri(std::string const& m);

    inline std::string const& GetResponseString();
    inline void SetResponseString(const char* m);
    inline void SetResponseString(std::string const& m);

    inline int GetCode();
    inline void SetCode(int code);

    inline int GetMajor();
    inline void SetMajor(int v);

    inline int GetMinor();
    inline void SetMinor(int v);

    inline std::string const& GetField(std::string const& k);
    inline void SetField(std::string const& k, const char* m);
    inline void SetField(std::string const& k, std::string const& m);
    /// --------------------------------------------------------

    inline bool IsRequest() const { return type_ == Request; }
    inline bool IsResponse() const { return type_ == Response; }

private:
    inline bool ParseMethod(const char* pos, const char* last);
    inline bool ParseUri(const char* pos, const char* last);
    inline bool ParseVersion(const char* pos, const char* last);
    inline bool ParseCode(const char* pos, const char* last);
    inline bool ParseResponseStr(const char* pos, const char* last);
    inline const char* ParseField(const char* pos, const char* last);

    inline bool CheckMethod() const;
    inline bool CheckUri() const;
    inline bool CheckCode() const;
    inline bool CheckResponseString() const;
    inline bool CheckVersion() const;

private:
    DocumentType type_;     // 类型

    // 默认版本号: HTTP/1.1
    uint32_t major_ = 1;
    uint32_t minor_ = 1;

    eParseState parse_state_ = eParseState::init;
    std::string parse_buffer_;
    std::error_code ec_;    // 解析错状态

    std::string request_method_;
    std::string request_uri_;

    uint32_t response_code_ = 404;
    std::string response_str_;

    std::vector<std::pair<std::string, std::string>> header_fields_;
};

} //namespace rapidhttp 

#include "document.hpp"
