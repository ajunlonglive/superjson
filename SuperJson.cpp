//
//  JsonTree.cpp
//  fileshare
//
//  Created by 林佑 on 2020/3/27.
//

#include "SuperJson.h"
#include <memory.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>

#include <cstring>

namespace SuperJson {

#if defined( _DEBUG ) || defined( DEBUG ) || defined (__DEBUG__)
#   ifndef DEBUG
#       define DEBUG
#   endif
#endif
    
#if defined(DEBUG)
    #if defined(_MSC_VER)
        #define ASSERT(x) if(!(x)){__debugbreak();} //if ( !(x)) WinDebugBreak()
    #elif defined (ANDROID_NDK)
        #include <android/log.h>
        #define ASSERT(x) if(!(x)){__android_log_assert("assert", "grinliz", "ASSERT in '%s' at %d.", __FILE__, __LINE__);}
    #else
        #include <assert.h>
        #define ASSERT assert
    #endif
#else
    #define ASSERT(x) {}
#endif


#if (defined(_MSC_VER) && (_MSC_VER >= 1400 ) && (CC_TARGET_PLATFORM != CC_PLATFORM_MARMALADE))
// Microsoft visual studio, version 2005 and higher.
/*int _snprintf_s(
   char *buffer,
   size_t sizeOfBuffer,
   size_t count,
   const char *format [,
      argument] ...
);*/
inline int SNPRINTF( char* buffer, size_t size, const char* format, ... )
{
    va_list va;
    va_start( va, format );
    int result = vsnprintf_s( buffer, size, _TRUNCATE, format, va );
    va_end( va );
    return result;
}
#define SSCANF   sscanf_s
#else
// GCC version 3 and higher
//#warning( "Using sn* functions." )
#define SNPRINTF snprintf
#define SSCANF   sscanf
#endif
#define PRINTF printf
#define MALLOC malloc
#define FREE free



static inline void ToString(int v, char* buffer, int size)
{
    SNPRINTF(buffer, size, "%d", v);
}

static inline void ToString(unsigned v, char* buffer, int size)
{
    SNPRINTF(buffer, size, "%u", v);
}

static inline void ToString(bool v, char* buffer, int size)
{
    SNPRINTF(buffer, size, "%d", v?1:0);
}

static inline void ToString(float v, char* buffer, int size)
{
    SNPRINTF(buffer, size, "%g", v);
}

static inline void ToString(double v, char* buffer, int size)
{
    SNPRINTF(buffer, size, "%g", v);
}

static inline bool StringEqual(const char* s1, const char* s2)
{
    return strcmp(s1, s2) == 0;
}

static inline bool ToInt(const char* str, int* value)
{
    if (SSCANF(str, "%d", value) == 1)
    {
        return true;
    }
    return false;
}

static inline bool ToUnsigned(const char* str, unsigned *value)
{
    if (SSCANF(str, "%u", value) == 1)
    {
        return true;
    }
    return false;
}

static inline bool ToBool(const char* str, bool* value)
{
    int ival = 0;
    if (ToInt(str, &ival))
    {
        *value = (ival==0) ? false : true;
        return true;
    }
    if (StringEqual(str, "true"))
    {
        *value = true;
        return true;
    }
    else if (StringEqual(str, "false"))
    {
        *value = false;
        return true;
    }
    return false;
}

static inline bool ToFloat(const char* str, float* value)
{
    if (SSCANF(str, "%f", value) == 1)
    {
        return true;
    }
    return false;
}

static inline bool ToDouble(const char* str, double* value)
{
    if (SSCANF(str, "%lf", value) == 1)
    {
        return true;
    }
    return false;
}


//JsonBox
JsonNode::JsonBox::JsonBox(int cap)
:_count(0),
_capacity(cap),
_childs(0),
_next(0)
{
    size_t size = sizeof(JsonNode*)*cap;
    _childs = (JsonNode**)MALLOC(size);
    memset(_childs, 0, size);
}

JsonNode::JsonBox::~JsonBox()
{
    if(_childs)
    {
        for (size_t i=0; i<_count; ++i)
        {
            delete _childs[i];
        }
        _count = 0;
        FREE(_childs);
        _childs = 0;
    }
    _capacity = 0;
}

void JsonNode::JsonBox::moveTo(JsonBox* box)
{
    ASSERT(box->_capacity >= _capacity);
    ASSERT(box->_count == 0);
    for (size_t i=0; i<_count; ++i)
    {
        box->_childs[i] = _childs[i];
    }
    box->_count = _count;
    clear();
}

void JsonNode::JsonBox::clear()
{
    size_t size = sizeof(JsonNode*)*_capacity;
    memset(_childs, 0, size);
    _count = 0;
}

void JsonNode::JsonBox::add(JsonNode* node)
{
   _childs[_count] = node;
   node->_index = _count;
   _count++;
}

bool JsonNode::JsonBox::remove(JsonNode* node)
{
    for (size_t i=0; i<_count; ++i)
    {
        if(_childs[i] == node)
        {
            for (; i<_count-1; ++i)
            {
                _childs[i] = _childs[i+1];
            }
            _count--;
            _childs[_count] = 0;
            delete node;
            return true;
        }
    }
    return false;
}

//JsonContext
JsonNode::JsonContext::JsonContext(char* data, size_t size)
:_bufferHead(0),
_bufferTail(0)
{
    _data = (char*)MALLOC(size);
    _size = size;
    memcpy(_data, data, size);
    _offset = 0;
    memset(_boxs, 0, sizeof(_boxs));
	//PRINTF("create JsonContext\n");
}

JsonNode::JsonContext::JsonContext()
:_bufferHead(0),
_bufferTail(0)
{
    _data = 0;
    _size = 0;
    _offset = 0;
    memset(_boxs, 0, sizeof(_boxs));
	//PRINTF("create JsonContext\n");
}

JsonNode::JsonContext::~JsonContext()
{
	//PRINTF("destroy ~JsonContext\n");
    if(_data)
    {
        FREE(_data);
        _data = 0;
        _size = 0;
    }
    JsonNode::JsonBox* box;
    JsonNode::JsonBox* nextBox;
    for (int i=0; i<64; ++i)
    {
        box = _boxs[i];
        while(box)
        {
            nextBox = box->_next;
            delete box;
            box = nextBox;
        }
    }
    memset(_boxs, 0, sizeof(_boxs));
    
	JsonBuffer* tmp;
	JsonBuffer* buffer = _bufferHead;
    while (buffer)
    {
		tmp = buffer->_next;
		FREE(buffer->_buffer);
		buffer->_buffer = 0;
        delete buffer;
		buffer = tmp;
    }
    _bufferHead = 0;
    _bufferTail = 0;
}

void JsonNode::JsonContext::clearBox()
{
	JsonNode::JsonBox* box;
	JsonNode::JsonBox* nextBox;
	for (int i = 0; i < 64; ++i)
	{
		box = _boxs[i];
		while (box)
		{
			nextBox = box->_next;
			delete box;
			box = nextBox;
		}
	}
	memset(_boxs, 0, sizeof(_boxs));
}

int JsonNode::JsonContext::capToIdx(int cap)
{
    int idx = 0;
    for (int i=0; i<64; ++i)
    {
        if(cap & (1<<i))
        {
            idx = i;break;
        }
    }
    if(idx == 0 && cap != 1)
    {
        PRINTF("JsonNode too many nodes");
        throw "JsonNode too many nodes";
        return 0;
    }
    return idx;
}

JsonNode::JsonBox* JsonNode::JsonContext::popIdleBox(int cap)
{
    int idx = capToIdx(cap);
    JsonNode::JsonBox* box = _boxs[idx];
    if (box)
    {
        ASSERT(box->_capacity == cap);
        JsonNode::JsonBox* nextBox = box->_next;
        _boxs[idx] = nextBox;
        return box;
    }
    box = new JsonBox(cap);
    return box;
}

void JsonNode::JsonContext::pushIdleBox(JsonBox* box)
{
    int idx = capToIdx(box->_capacity);
    JsonNode::JsonBox* nextBox = _boxs[idx];
    box->_next = nextBox;
    _boxs[idx] = box;
}

JsonNode::JsonBuffer* JsonNode::JsonContext::createBuffer()
{
    JsonBuffer* buffer = new JsonBuffer();
    buffer->_capacity = 10;
    buffer->_offset = 0;
    buffer->_next = 0;
    buffer->_len = 0;
	buffer->_buffer = (char*)MALLOC(buffer->_capacity);
	memset(buffer->_buffer, 0, buffer->_capacity);
    return buffer;
}

void JsonNode::JsonContext::clearBuffer()
{
	JsonBuffer* buffer = _bufferHead;
	JsonBuffer* tmp;
	while (buffer)
	{
		tmp = buffer->_next;
		FREE(buffer->_buffer);
		buffer->_buffer = 0;
		delete buffer;
		buffer = tmp;
	}
	_bufferHead = 0;
	_bufferTail = 0;
	_size = 0;
	if (_data != 0)
	{
		FREE(_data);
		_data = 0;
	}
}

void JsonNode::JsonContext::writeBuffer(char* data, size_t len)
{
    JsonBuffer* buffer = _bufferTail;
    if(!buffer)
    {
        buffer = createBuffer();
        _bufferHead = buffer;
        _bufferTail = buffer;
    }
    size_t leftCap;
	size_t idx = 0;
    while (len > 0)
    {
        leftCap = buffer->_capacity - buffer->_len;
        if(leftCap >= len)
        {
            memcpy(buffer->_buffer+buffer->_offset, data, len);
			buffer->_offset += len;
			buffer->_len += len;
			_size += len;
            return;
        }
		else if (leftCap > 0)
		{
			memcpy(buffer->_buffer + buffer->_offset, data, leftCap);
			len -= leftCap;
			buffer->_offset += leftCap;
			buffer->_len += leftCap;
			_size += leftCap;
			data += leftCap;
		}

        buffer = createBuffer();
        _bufferTail->_next = buffer;
		_bufferTail = buffer;
    }
}

void JsonNode::JsonContext::readBuffer()
{
	_data = (char*)MALLOC(_size+1);
	memset(_data, 0, _size + 1);
	size_t offset = 0;

	JsonBuffer* buffer = _bufferHead;
	while (buffer)
	{
		memcpy(_data + offset, buffer->_buffer, buffer->_len);
		offset += buffer->_len;
		buffer = buffer->_next;
	}
}

//JsonSegment
JsonNode::JsonSegment::JsonSegment()
:_type(SEGMENTDOUBLE)
{
	_value._double = 0;
}
JsonNode::JsonSegment::~JsonSegment()
{
	clear();
}

void JsonNode::JsonSegment::clear()
{
	if (_type == SEGMENTSTRING)
	{
		if (_value._string != 0)
		{
			FREE(_value._string);
			_value._double = 0;
			_type = SEGMENTDOUBLE;
		}
	}
}


//JsonNode
JsonNode* JsonNode::null = 0;
const char* JsonNode::none = 0;

JsonNode::JsonNode()
:_type(JsonNode::ROOT),
_key(0),
_box(0),
_index(-1),
_segment(0),
_wcontext(0),
_context(0)
{
}

JsonNode::~JsonNode()
{
	if (_segment)
	{
		delete _segment;
		_segment = 0;
	}
    if(_box)
    {
		ASSERT(_type == JsonNode::OBJECT || _type == JsonNode::ARRAY || _type == JsonNode::ROOT);
        delete _box;
        _box = 0;
    }
    if (_context->root == this)
    {
        _context->root = 0;
        delete _context;
        _context = 0;
    }
	if (_wcontext != 0)
	{
		delete _wcontext;
		_wcontext = 0;
	}
}

JsonNode* JsonNode::createNode(unsigned char code)
{
    JsonNode* node = 0;
    switch (code)
    {
        case '"':
        case 39:
            node = new JsonString();
            break;
        case '{':
            node = new JsonObject();
            break;
        case '[':
            node = new JsonArray();
            break;
        default:
            node = new JsonNumber();
            break;
    }
    return node;
}

const char* JsonNode::getKey()
{
    if (_key)  return _key->getString();
    return JsonNode::none;
}

const char* JsonNode::getTypeName()
{
    static const char* InfoTags[6] = {"ROOT", "STRING", "NUMBER", "OBJECT", "ARRAY", "UNKNOWN"};
    int len = sizeof(InfoTags)/sizeof(InfoTags[0]);
    const char* tab = InfoTags[5];
    if (_type >= 0 && _type < len)
    {
        tab = InfoTags[_type];
    }
    return tab;
}

const char* JsonNode::getString()
{
    if(_type == JsonNode::STRING)
    {
		if (_segment != 0)
		{
			return (const char*)_segment->_value._string;
		}
        return _context->_data+_idx;
    }
    else if(_type == JsonNode::NUMBER)
    {
		if (_segment != 0 || _len == 0)
		{
			return JsonNode::none;
		}
        return _context->_data+_idx;
    }
    PRINTF("SuperJson warn:JsonNode is no STRING\n");
    return JsonNode::none;
}

void JsonNode::setString(const char* val, size_t len)
{
	if (_type != JsonNode::STRING)
	{
		throwError("JsonNode is no NUMBER");
		return;
	}
	if (_segment == 0)
	{
		_segment = new JsonSegment;
	}
	_segment->clear();
	_segment->_type = JsonSegment::SEGMENTSTRING;

	char* tmp = (char*)MALLOC(len + 1);
	memset(tmp, 0, len + 1);
	memcpy(tmp, val, len);
	_segment->_value._string = tmp;
	_len = len;
}

void JsonNode::setString(const char* val)
{
	size_t len = strlen(val);
	setString(val, len);
}

const char* JsonNode::__getString()
{
	if (_type == JsonNode::STRING)
	{
		return _context->_data + _idx;
	}
	else if (_type == JsonNode::NUMBER)
	{
		return _context->_data + _idx;
	}
	PRINTF("SuperJson warn:JsonNode is no STRING\n");
	return JsonNode::none;
}

double JsonNode::stringToNumber()
{
    const char* str = __getString();
    double dval = 0;
    if (ToDouble(str, &dval))
    {
		return dval;
    }
	else if (StringEqual(str, "true"))
    {
        return 1.0;
    }
    else if (StringEqual(str, "false"))
    {
        return 0.0;
    }
	char tmp[1024] = {0};
	sprintf(tmp, "%s is no NUMBER", str);
	throwError(tmp);
	return 0.0;
}

double JsonNode::getNumber()
{
	if (_type != JsonNode::NUMBER) 
	{
		throwError("no NUMBER");
		return 0;
	}
	if (_segment == 0)
	{
		_segment = new JsonSegment;
		_segment->_type = JsonSegment::SEGMENTDOUBLE;
		_segment->_value._double = stringToNumber();
	}
	return _segment->_value._double;
}

bool JsonNode::getBool()
{
	if (_type != JsonNode::NUMBER) 
	{
		throwError("JsonNode is no NUMBER");
		return 0;
	}
	if (_segment == 0)
	{
		_segment = new JsonSegment;
		_segment->_type = JsonSegment::SEGMENTBOOL;
		_segment->_value._bool = stringToNumber() != 0 ? true : false;
	}
	return _segment->_value._bool;
}

int JsonNode::getInteger()
{
	if (_type != JsonNode::NUMBER) 
	{
		throwError("JsonNode is no NUMBER");
		return 0;
	}
	if (_segment == 0)
	{
		_segment = new JsonSegment;
		_segment->_type = JsonSegment::SEGMENTINT;
		_segment->_value._int = static_cast<int>(stringToNumber());
	}
	return _segment->_value._int;
}

long JsonNode::getLong()
{
	if (_type != JsonNode::NUMBER) 
	{
		throwError("JsonNode is no NUMBER");
		return 0;
	}
	if (_segment == 0)
	{
		_segment = new JsonSegment;
		_segment->_type = JsonSegment::SEGMENTLONG;
		_segment->_value._long = static_cast<long>(stringToNumber());
	}
	return _segment->_value._long;
}

double JsonNode::getDouble()
{
	if (_type != JsonNode::NUMBER) 
	{
		throwError("JsonNode is no NUMBER");
		return 0;
	}
    return getNumber();
}

void JsonNode::setNumber(double val)
{
	if (_type != JsonNode::NUMBER)
	{
		throwError("JsonNode is no NUMBER");
		return;
	}
	if (_segment == 0)
	{
		_segment = new JsonSegment;
	}
	_segment->clear();
	_segment->_type = JsonSegment::SEGMENTDOUBLE;
	_segment->_value._double = val;
}

void JsonNode::setBool(bool val)
{
	if (_type != JsonNode::NUMBER)
	{
		throwError("JsonNode is no NUMBER");
		return;
	}
	if (_segment == 0)
	{
		_segment = new JsonSegment;
		_segment->_type = JsonSegment::SEGMENTBOOL;
	}
	_segment->_value._bool = val;
}

void JsonNode::setInteger(int val)
{
	if (_type != JsonNode::NUMBER)
	{
		throwError("JsonNode is no NUMBER");
		return;
	}
	if (_segment == 0)
	{
		_segment = new JsonSegment;
		_segment->_type = JsonSegment::SEGMENTINT;
	}
	_segment->_value._int = val;
}

void JsonNode::setLong(long val)
{
	if (_type != JsonNode::NUMBER)
	{
		throwError("JsonNode is no NUMBER");
		return;
	}
	if (_segment == 0)
	{
		_segment = new JsonSegment;
		_segment->_type = JsonSegment::SEGMENTLONG;
	}
	_segment->_value._long = val;
}

void JsonNode::setDouble(double val)
{
	if (_type != JsonNode::NUMBER)
	{
		throwError("JsonNode is no NUMBER");
		return;
	}
	if (_segment == 0)
	{
		_segment = new JsonSegment;
		_segment->_type = JsonSegment::SEGMENTDOUBLE;
	}
	_segment->_value._double = val;
}

JsonNode* JsonNode::newNumberNode() 
{
    if (_type == JsonNode::ROOT)
    {
		if (_context == 0)
		{
			_context = new JsonContext();
			_context->root = this;
		}
    }
	ASSERT(_context);
	JsonNode* node = new JsonNumber;
	node->_context = _context;
	return node;
}

JsonNode* JsonNode::newStringNode() 
{ 
    if (_type == JsonNode::ROOT)
    {
		if (_context == 0)
		{
			_context = new JsonContext();
			_context->root = this;
		}
    }
	ASSERT(_context);
	JsonNode* node = new JsonString;
	node->_context = _context;
	return node;
}

JsonNode* JsonNode::newObjectNode() 
{ 
    if (_type == JsonNode::ROOT)
    {
		if (_context == 0)
		{
			_context = new JsonContext();
			_context->root = this;
		}
    }
	ASSERT(_context);
	JsonNode* node = new JsonObject;
	node->_context = _context;
	return node;
}

JsonNode* JsonNode::newArrayNode() 
{ 
    if (_type == JsonNode::ROOT)
    {
		if (_context == 0)
		{
			_context = new JsonContext();
			_context->root = this;
		}
    }
	ASSERT(_context);
	JsonNode* node = new JsonArray;
	node->_context = _context;
	return node;
}

JsonNode* JsonNode::array(size_t idx)
{
    if (_box->_count == 0 || idx >= _box->_count)
    {
        return JsonNode::null;
    }
    return (*_box)[idx];
}

JsonNode* JsonNode::object(const char* key)
{
    if (_box->_count > 0)
    {
        if(_type != JsonNode::OBJECT)
        {
            throwError("JsonNode must be Object");
            return JsonNode::null;;
        }
        JsonNode* child;
        for (size_t i = 0; i < _box->_count; ++i)
        {
            child = (*_box)[i];;
            if (strcmp(child->getKey(), key) == 0)
            {
                return child;
            }
        }
    }
    return JsonNode::null;
}

void JsonNode::trimSpace()
{
    size_t len = _context->_size;
    char code;
    for (size_t idx = _idx; idx < len; ++idx)
    {
        code = _context->_data[idx];
       if (code > 32)
       {
           _idx = idx;break;
       }
    }
}

unsigned char JsonNode::getCharCode()
{
    if (_idx < _context->_size)
    {
        return _context->_data[_idx];
    }
    return 0;
}

unsigned char JsonNode::getChar()
{
    unsigned char code = getCharCode();
    if (code <= 32)
    {
        trimSpace();
        code = getCharCode();
    }
    return code;
}

unsigned char JsonNode::checkCode(unsigned char charCode)
{
    unsigned char code = getCharCode();
    if (code != charCode)
    {
        trimSpace();
        code = getCharCode();
        if (code != charCode)
        {
            return 0;
        }
    }
    _idx++;
    return code;
}

int JsonNode::searchCode(unsigned char code)
{
    size_t len = _context->_size;
    char* data = _context->_data;
    for (size_t idx = _idx; idx < len; idx++)
    {
        if (data[idx] == code)
        {
            /*\  */
            if (idx > 0 && data[idx - 1] != 92)
            {
                return idx;
            }
        }
    }
    return -1;
}

void JsonNode::__read(JsonContext* context)
{
	if (_context)
	{
		delete _context;
		_context = 0;
	}
    _context = context;
    _idx = _context->_offset;
    _len = 0;
}

void JsonNode::addNode(JsonNode* node)
{
	if (_type != JsonNode::OBJECT && _type != JsonNode::ARRAY && _type != JsonNode::ROOT)
	{
		throwError("JsonNode must be OBJECT or ARRAY");
		return;
	}
    if(_box == 0)
    {
        _box = _context->popIdleBox(1);
    }
    else
    {
        if(_box->isFull())
        {
            JsonBox* newBox = _context->popIdleBox(_box->_capacity*2);
            _box->moveTo(newBox);
            _context->pushIdleBox(_box);
            _box = newBox;
        }
    }
	if (_type == JsonNode::ROOT)
	{
		if (_context == 0)
		{
			_context = new JsonContext();
			_context->root = this;
		}
	}
	node->_context = _context;
    _box->add(node);
}

void JsonNode::addNode(const char* key, JsonNode* node)
{
	JsonNode* keyNode = newStringNode();
	keyNode->setString(key);
	node->_key = (JsonString*)keyNode;
	addNode(node);
}

bool JsonNode::removeNode(JsonNode* node)
{
    if(_box == 0) return false;
    return _box->remove(node);
}

void JsonNode::throwError(const char* errMsg)
{
    const char* tab = getTypeName();
    PRINTF("SuperJson [%s] Error: %s\n", tab, errMsg);
    char tmp[1024] = {0};
    size_t len = _context->_size - _context->_offset;
    len = len>60?60:len;
    memcpy(tmp, _context->_data+_idx, len);
    PRINTF("SuperJson Error content:%s\n",tmp);
    throw errMsg;
}

void JsonNode::__write(JsonContext* context)
{
	ASSERT(0);
}

const char* JsonNode::write(size_t* size)
{
	//ASSERT(_type == JsonNode::ROOT);
	if (_wcontext == 0)
	{
		_wcontext = new JsonContext();
	}
	JsonContext* context = _wcontext;
	context->clearBuffer();

	if (_type == JsonNode::ROOT)
	{
		JsonNode* node = array(0);
		if (node != 0)
		{
			node->__write(context);
			context->readBuffer();
			*size = context->_size;
			return context->_data;
		}
	}
	else
	{
		__write(context);
		context->readBuffer();
		*size = context->_size;
		return context->_data;
	}
	*size = 0;
	return JsonNode::none;
}

void JsonNode::read_internal(const char* data, size_t size)
{
	ASSERT(_type == JsonNode::ROOT);
	if (_context == 0)
	{
		delete _context;
		_context = 0;
	}
    JsonContext* context = new JsonContext();
    context->_data = (char*)data;
    context->_size = size;
	context->_offset = 0;
    
    __read(context);
    unsigned char code = getChar();
    JsonNode* node = createNode(code);
    context->_offset = _idx;
    try {
        context->root = this;
        node->__read(context);
        addNode(node);
    } catch (const char* error) {
        delete node;
        PRINTF("SuperJson catch exception %s", error);
    }
}

void JsonNode::read(const char* data, size_t size)
{
	ASSERT(_type == JsonNode::ROOT);
	if (_context)
	{
		delete _context;
		_context = 0;
	}
	JsonContext* context = new JsonContext((char*)data, size);
	context->clearBox();
    __read(context);
    unsigned char code = getChar();
    JsonNode* node = createNode(code);
    context->_offset = _idx;
    try {
		context->root = this;
		node->__read(context);
        addNode(node);
    } catch (const char* error) {
        delete node;
        PRINTF("SuperJson catch exception %s", error);
    }
}

void JsonNode::readFile(const char* filePath)
{
	ASSERT(this->_type == JsonNode::ROOT);
    FILE * fp = fopen(filePath, "r");
    if (fp == 0)
    {
        PRINTF("SuperJson read file error:%s",strerror(errno));
        return;
    }
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    if (size > 1024*1024*1024) {
        PRINTF("SuperJson file is too big %s\n", filePath);
        return;
    }
    fseek(fp, 0, SEEK_SET);
	char* data = (char*)MALLOC(size);
	size_t idx = 0;

	char buff[4096] = { 0 };
	size_t ret;
	while (true)
	{
		ret = fread(buff, 1, 4096, fp);
		if (ret < 0)
		{
			fclose(fp);
			FREE(data);
			PRINTF("SuperJson read file error:%s", strerror(errno));
			return;
		}
		else if(ret == 0)
		{
			break;
		}
		memcpy(data+ idx, buff, ret);
		idx += ret;
	}
    fclose(fp);
    read_internal(data, size);
}

void JsonNode::writeFile(const char* filePath)
{
	ASSERT(_type == JsonNode::ROOT);
	ASSERT(this->count() == 1);
    FILE * fp = fopen(filePath, "w");
    if (fp == 0)
    {
        PRINTF("SuperJson write file error:%s",strerror(errno));
        return;
    }
    fseek(fp, 0, SEEK_SET);
	size_t len;
	const char* buffer = write(&len);
	fwrite(buffer, len, 1, fp);
	fclose(fp);
}

//JsonNumber
JsonNumber::JsonNumber()
:JsonNode()
{
    _type = JsonNode::NUMBER;
}

JsonNumber::~JsonNumber()
{
}

void JsonNumber::__read(JsonContext* context)
{
    JsonNode::__read(context);
    size_t sidx = _idx;
    size_t len =  context->_size;
    unsigned char code;
    char* data = context->_data;
    for (; _idx < len; _idx++)
    {
        code = data[_idx];
        if(code == ',' || code == '}' || code == ']')
        {
            _idx--;
            break;
        }
    }
    if(_idx < sidx)
    {
        throwError("lost number value");
        return;
    }
    _len = _idx - sidx+1;
    _idx = sidx;
}

void JsonNumber::__write(JsonContext* context)
{
	char buff[256] = { 0 };
	const char* val;
	if (_segment != 0)
	{
		switch (_segment->_type)
		{
			case JsonSegment::SEGMENTBOOL:
				ToString(_segment->_value._bool, buff, 256);
				break;
			case JsonSegment::SEGMENTINT:
				ToString(_segment->_value._int, buff, 256);
				break;
			case JsonSegment::SEGMENTLONG:
				ToString(_segment->_value._long, buff, 256);
				break;
			case JsonSegment::SEGMENTDOUBLE:
				ToString(_segment->_value._double, buff, 256);
				break;
			default:
				throwError("unkown segment");
				break;
		}
		val = buff;
	}
	else
	{
		val = getString();
	}
	if (_key)
	{
		char* tmp = (char*)MALLOC(_key->getStringLen() + strlen(val)+8);
		int ret = sprintf(tmp, "\"%s\":%s", getKey(), val);
		context->writeBuffer(tmp, ret);
		FREE(tmp);
	}
	else
	{
		context->writeBuffer((char*)val, strlen(val));
	}
}

//void JsonNumber::print()
//{
//    if (_key)
//        printf("\"%s\":%d", getKey(), (int)getNumber());
//    else
//        printf("%d",  (int)getNumber());
//}

//JsonString
JsonString::JsonString()
:JsonNode()
{
    _type = JsonNode::STRING;
}

JsonString::~JsonString()
{
}

void JsonString::__read(JsonContext* context)
{
    JsonNode::__read(context);
    unsigned char code = '"';
    if (!checkCode(code))
    {
        code = 39;
        if (!checkCode(code))
        {
            throwError("lost '\"' or '''");
            return;
        }
    }
    size_t sidx = _idx;
    int eidx = searchCode(code);
    if (eidx < 0)
    {
        throwError("lost '\"' or '''");
        return;
    }
    _idx = sidx;
    _len = eidx - sidx+1;
    context->_data[eidx] = 0;
}

void JsonString::__write(JsonContext* context)
{
	const char* val = getString();
	if (_key)
	{
		char* tmp = (char*)MALLOC(_key->getStringLen() + getStringLen() + 8);
		int ret = sprintf(tmp, "\"%s\":\"%s\"", getKey(), val);
		context->writeBuffer(tmp, ret);
		FREE(tmp);
	}
	else
	{
		char* tmp = (char*)MALLOC(getStringLen() + 4);
		int ret = sprintf(tmp, "\"%s\"", val);
		context->writeBuffer((char*)tmp, ret);
		FREE(tmp);
	}
}


//void JsonString::print()
//{
//    if (_key)
//        printf("\"%s\":\"%s\"", getKey(), getString());
//    else
//        printf("\"%s\"", getString());
//}

//JsonObject
JsonObject::JsonObject()
:JsonNode()
{
    _type = JsonNode::OBJECT;
}

JsonObject::~JsonObject()
{
}

void JsonObject::__read(JsonContext* context)
{
    JsonNode::__read(context);
    size_t oidx = _idx;
    if (!checkCode('{'))
    {
        throwError("lost '{'");
        return;
    }
    size_t len =  context->_size;
    unsigned char code;
    JsonNode* keyNode;
    JsonNode* valNode;
    while (_idx < len)
    {
        code = getChar();
        if (code == 0)
        {
            throwError("lost '}'");
            return;
        }
        if (checkCode('}')) break;
        keyNode = createNode(code);
        if(keyNode->_type != JsonNode::STRING)
        {
            throwError("key");
            return;
        }
        context->_offset = _idx;
        keyNode->__read(context);
        _idx = keyNode->_idx + keyNode->_len;
        if (!checkCode(':'))
        {
            throwError("lost ':'");
            return;
        }
        code = getChar();
        valNode = createNode(code);
        valNode->_key = (JsonString*)keyNode;
        context->_offset = _idx;
        valNode->__read(context);
        _idx = valNode->_idx + valNode->_len;
        addNode(valNode);
        
        if (checkCode('}'))
        {
            context->_data[_idx-1] = 0;
            break;
        }
        if (!checkCode(','))
        {
            throwError("lost ','");
            return;
        }
        context->_data[_idx-1] = 0;
    }
    _len = _idx - oidx;
    _idx = oidx;
}

void JsonObject::__write(JsonContext* context)
{
	char* tmp;
	if (_key)
	{
		tmp = (char*)MALLOC(_key->getStringLen()+ 8);
		int ret = sprintf(tmp, "\"%s\":{", getKey());
		context->writeBuffer(tmp, ret);
		FREE(tmp);
	}
	else
	{
		tmp = "{";
		context->writeBuffer((char*)tmp, strlen(tmp));
	}

	if (_box != 0)
	{
		size_t count = _box->_count;
		JsonNode** childs = _box->_childs;
		for (size_t i = 0; i < count; ++i)
		{
			childs[i]->__write(context);
			if (i < count - 1)
			{
				tmp = ",";
				context->writeBuffer((char*)tmp, strlen(tmp));
			}
		}
	}
	tmp = "}";
	context->writeBuffer((char*)tmp, strlen(tmp));
}

//void JsonObject::print()
//{
//    if (_key)
//        printf("\"%s\":{", getKey());
//    else
//        printf("{");
//
//    size_t count = _box->_count;
//    JsonNode** childs = _box->_childs;
//    for (size_t i=0; i<count; ++i)
//    {
//        childs[i]->print();
//        if (i < count-1) printf(",");
//    }
//    printf("}");
//}

//JsonArray
JsonArray::JsonArray()
:JsonNode()
{
    _type = JsonNode::ARRAY;
}

JsonArray::~JsonArray()
{
}

void JsonArray::__read(JsonContext* context)
{
    JsonNode::__read(context);
    size_t oidx = _idx;
    if (!checkCode('['))
    {
        throwError("lost '['");
        return;
    }
    size_t len = context->_size;
    unsigned char code;
    JsonNode* valNode;
    while (_idx < len)
    {
        code = getChar();
        if (code == 0)
        {
            throwError("lost ']'");
            return;
        }
        if (checkCode(']')) break;
        valNode = createNode(code);
        context->_offset = _idx;
        valNode->__read(context);
        _idx = valNode->_idx + valNode->_len;
        addNode(valNode);

        if (checkCode(']'))
        {
            context->_data[_idx-1] = 0;
            break;
        }
        if (!checkCode(','))
        {
            throwError("lost ','");
            return;
        }
        context->_data[_idx-1] = 0;
    }
    _len = _idx - oidx;
    _idx = oidx;
}

void JsonArray::__write(JsonContext* context)
{
	char* tmp;
	if (_key)
	{
		tmp = (char*)MALLOC(_key->getStringLen() + 8);
		int ret = sprintf(tmp, "\"%s\":[", getKey());
		context->writeBuffer(tmp, ret);
		FREE(tmp);
	}
	else
	{
		tmp = "[";
		context->writeBuffer((char*)tmp, strlen(tmp));
	}
	if (_box != 0)
	{
		size_t count = _box->_count;
		JsonNode** childs = _box->_childs;
		for (size_t i = 0; i < count; ++i)
		{
			childs[i]->__write(context);
			if (i < count - 1)
			{
				tmp = ",";
				context->writeBuffer((char*)tmp, strlen(tmp));
			}
		}
	}
	tmp = "]";
	context->writeBuffer((char*)tmp, strlen(tmp));
}

//void JsonArray::print()
//{
//    if (_key)
//        printf("\"%s\":[", getKey());
//    else
//        printf("[");
//
//    size_t count = _box->_count;
//    JsonNode** childs = _box->_childs;
//    for (size_t i=0; i<count; ++i)
//    {
//        childs[i]->print();
//        if (i < count-1) printf(",");
//    }
//    printf("]");
//}

};
