//
//  JsonTree.hpp
//  fileshare
//
//  Created by 林佑 on 2020/3/27.
//

#ifndef JsonTree_h
#define JsonTree_h

#include <stddef.h>

namespace SuperJson {

class JsonNumber;
class JsonString;
class JsonObject;
class JsonArray;

class JsonNode
{
public:
    enum JsonType
    {
        ROOT = 0,
        STRING,
        NUMBER,
        OBJECT,
        ARRAY,
        UNKNOWN
    };
private:
    struct JsonBox
    {
        size_t _count;
        size_t _capacity;
        JsonNode** _childs;
        JsonBox* _next;
        
        JsonBox(int cap);
        ~JsonBox();
        void moveTo(JsonBox* box);
        void clear();
        inline bool isFull(){return _count >= _capacity;}
        inline JsonNode* operator[] (size_t idx){return _childs[idx];}
        void add(JsonNode* node);
        bool remove(JsonNode* node);
    };
    
    struct JsonBuffer
    {
        char* _buffer;
        size_t _offset;
        size_t _capacity;
        size_t _len;
        JsonBuffer* _next;
    };
    
    class JsonContext
    {
        JsonBox* _boxs[64];
        int capToIdx(int cap);
        
        JsonBuffer* _bufferHead;
        JsonBuffer* _bufferTail;
        JsonBuffer* createBuffer();
    public:
        JsonBox* popIdleBox(int cap);
        void pushIdleBox(JsonBox* box);
		void clearBox();
        char* _data;
        size_t _size;
        size_t _offset;
        JsonNode* root;

        JsonContext();
        JsonContext(char* data, size_t size);
        ~JsonContext();
        
        void writeBuffer(char* data, size_t len);
		void readBuffer();
		void clearBuffer();
    };
    
    class JsonSegment
    {
    public:
        JsonSegment();
        ~JsonSegment();
		void clear();
		enum JsonSegmentType
		{
			SEGMENTBOOL = 0,
			SEGMENTINT,
			SEGMENTLONG,
			SEGMENTDOUBLE,
			SEGMENTSTRING
		};
        char _type;
        union {
            bool _bool;
            int _int;
            long _long;
            double _double;
            void* _string;
        } _value;
    };

    JsonType _type;
    JsonContext* _context;
	JsonContext* _wcontext;

    size_t _idx;
    size_t _len;

    JsonString* _key;
    JsonBox* _box;
    int _index;
    
    JsonSegment* _segment;

    void trimSpace();
    JsonNode* createNode(unsigned char code);
   
    unsigned char getCharCode();
    unsigned char getChar();

    unsigned char checkCode(unsigned char charCode);
    int searchCode(unsigned char charCode);
    virtual void __read(JsonContext* context);
    virtual void __write(JsonContext* context);
    void read_internal(const char* data, size_t size);
	const char* __getString();
    void throwError(const char* errMsg);

    double stringToNumber();
    
    JsonNode* newNumberNode();
    JsonNode* newStringNode();
    JsonNode* newObjectNode();
    JsonNode* newArrayNode();
public:
    JsonNode();
    virtual ~JsonNode();
    
    static JsonNode* null;
    static const char* none;

    inline JsonType getType(){return _type;}
    const char* getTypeName();
    inline int getIndex(){return _index;}
    const char* getKey();
    const char* getString();
	inline size_t getStringLen(){return _len;}
	void setString(const char* val, size_t len);
	void setString(const char* val);

    double getNumber();
    bool getBool();
    int getInteger();
    long getLong();
    double getDouble();

	void setNumber(double val);
	void setBool(bool val);
	void setInteger(int val);
	void setLong(long val);
	void setDouble(double val);
    
    const char* getString(const char* key);
    double getNumber(const char* key);
    bool getBool(const char* key);
    int getInteger(const char* key);
    long getLong(const char* key);
    double getDouble(const char* key);
    
    inline bool isNumber(){return _type == JsonNode::NUMBER;}
    inline bool isString(){return _type == JsonNode::STRING;}
    inline bool isObject(){return _type == JsonNode::OBJECT;}
    inline bool isArray(){return _type == JsonNode::ARRAY;}
    inline size_t count(){return _box?_box->_count:0;}

    JsonNode* newAddNumberNode();
    JsonNode* newAddStringNode();
    JsonNode* newAddObjectNode();
    JsonNode* newAddArrayNode();
    JsonNode* newAddNumberNode(const char* key);
    JsonNode* newAddStringNode(const char* key);
    JsonNode* newAddObjectNode(const char* key);
    JsonNode* newAddArrayNode(const char* key);

    JsonNode* array(size_t idx);
    JsonNode* object(const char* key);
    inline JsonNode& operator[] (size_t idx){return *array(idx);}
    inline JsonNode& operator[] (const char* key){return *object(key);}
    void addNode(JsonNode* node);
	void addNode(const char* key,JsonNode* node);
    bool removeNode(JsonNode* node);
    
    void addString(const char* key, const char* val);
    void addNumber(const char* key, double val);
    void addBool(const char* key, bool val);
    void addInteger(const char* key, int val);
    void addLong(const char* key, long val);
    void addDouble(const char* key, double val);
    
    void read(const char* data, size_t size);
    const char* write(size_t* size);
    
    void readFile(const char* filePath);
    void writeFile(const char* filePath);

private:
    friend class JsonNumber;
    friend class JsonString;
    friend class JsonObject;
    friend class JsonArray;
};

class JsonNumber:public JsonNode
{
    JsonNumber();
    virtual ~JsonNumber();
    virtual void __read(JsonContext* context);
    virtual void __write(JsonContext* context);
//    virtual double getNumber();
//    virtual bool getBool();
//    virtual int getInteger();
//    virtual long getLong();
//    virtual double getDouble();
    
    friend class JsonNode;
};

class JsonString:public JsonNode
{
    JsonString();
    virtual ~JsonString();
    virtual void __read(JsonContext* context);
    virtual void __write(JsonContext* context);
    
    friend class JsonNode;
};

class JsonObject:public JsonNode
{
    JsonObject();
    virtual ~JsonObject();
    virtual void __read(JsonContext* context);
    virtual void __write(JsonContext* context);
    
    friend class JsonNode;
};

class JsonArray:public JsonNode
{
    JsonArray();
    virtual ~JsonArray();
    virtual void __read(JsonContext* context);
    virtual void __write(JsonContext* context);
    
    friend class JsonNode;
};


};

#endif /* JsonTree_h */
