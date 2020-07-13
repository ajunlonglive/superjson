# 超级Json解析和生成器

## 一款强大简单高性能的Json解析和生成器
设计的初衷有两个。希望能设计最佳性能而又强大的Json解析器。类似芯片一样，追求更高性能的芯片。
第二个初衷是，希望Json源文件数量少，容易导入项目。希望有更丰富的错误提示，让开发简单一点。更重要的是，希望有更高效的解析效率，甚至可以轻松解析几百MB大小的Json文件。根据多年的经验，设计出这款超级Json解析和生成器

## 介绍
SuperJson是一个Json解析器和生成器，它的设计遵循着无语言特性，可以用任何编程语言实现，目前暂时实现C++版本。
+ SuperJson很迷你，代码行数不超过2000，但功能相对完善。
+ SuperJson解析是非常快速，直接复制一份json数据，在此基础上，不会进行任何字符串切割，只是标记每个值的位置pos和len, 用node-tree的方式管理这些节点node。因此，它解析大json文件非常快速。只有取值的时候，才根据标记去获得相应的值。
如果是字符串，直接返回json数据的内存所在地址，该内存片段末尾被置为0，确保是c语言字符串，当然也有字段记录数据片段的大小，以防数据缺失。如果获取的值是数值，会生成segment片段，记录此数值。
+ SuperJson使用节点node管理的，节点管理通过c语言数组实现，保证迭代json节点的高效。有四种节点node，这四种节点继承jsonnode。父类jsonnode是root节点，否则其子类就是子节点。子节点有NumberNode,StringNode,ObjectNode,ArrayNode四个，对应记录的json类型。root节点不记录任何值，只保存了树根节点。
+ SuperJson的内存管理很高效，当然还有进一步优化的空间。
注意：不能释放SuperJson返回都内存，即使是write操作返回json字符串，也不能进行释放。因为它已经绑定到root节上。root节点释放，它就会自动被释放。

## 配置
只有源文件SuperJson.h和SuperJson.cpp，无其他依赖。跑测试例子，可以使用cmake快速构建编译执行

## 运行测试
在superjson根目录下，建一个build文件节。
```shell
mkdir build
```

cd到build文件文件夹，执行cmake。
```shell
cmake ../
```
再执行make命令，就可以生成可执行程序
也可以用cmake可视化工具进行生成工程。非常简单，不在此介绍。

## demo

记录文件列表demo
```c++
	//root 对象
    JsonNode root;
    JsonNode* object = root.newObjectNode();
    root.addNode(object);
    
    //创建一个数组对象
    JsonNode* array = root.newArrayNode();
    object->addNode("data", array);
    FileInfo* fileInfo;
    JsonNode* obj;
    for (int i=0; i<files.size(); ++i) {
        fileInfo = &files[i];
        
        obj = root.newObjectNode();
        obj->addStringNode("name", fileInfo->name.c_str());
        obj->addStringNode("path", fileInfo->path.c_str());
        obj->addNumberNode("is_dir", fileInfo->is_dir);
        obj->addNumberNode("mtime", fileInfo->mtime);
        obj->addNumberNode("size", fileInfo->size);

        array->addNode(obj);
    }
    
    size_t len;
    const char* buffer = root.write(&len);
    printf("result:%s\n", buffer);
```


1.解析json字符串
```c++
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "SuperJson.h"
using namespace SuperJson;
int main(int argc, char* argv[])
{
	const char* test = "{\
		\"bool1\":    true,\
		\"bool2\" :     false,\
		\"int\" :  323,\
		\"long\" :    2147483648,\
		\"double\" :    234.23\
		\"string\" : \"txt\"\
	}";
	printf("start:%s", test);

	//创建root对象，基类JsonNode实例对象都是root对象
	JsonNode root = JsonNode();
	//输入json格式数据
	root.read(test, strlen(test));

	//root对象，只包含了一个子节点，这个子节点就是json的数据结构
	JsonNode* node = root.array(0);
	//由于json数据，是一个object，所以它必定是ObjectNode
	assert(node->isObject());

	//取出字段bool1的节点，此节点保存了数值，同时还保存了key节点
	JsonNode* bool1node = node->object("bool1");
	printf("key=%s value=%d\n", bool1node->getKey(), (int)bool1node->getBool());

	//同上
	JsonNode* bool2node = node->object("bool2");
	printf("key=%s value=%d\n", bool2node->getKey(), (int)bool2node->getBool());

	JsonNode* intnode = node->object("int");
	printf("key=%s value=%d\n", intnode->getKey(), intnode->getInteger());

	JsonNode* longnode = node->object("long");
	printf("key=%s value=%ld\n", longnode->getKey(), longnode->getLong());

	JsonNode* doublenode = node->object("double");
	printf("key=%s value=%lf\n", doublenode->getKey(), doublenode->getDouble());
	printf("for==>>\n");
	JsonNode* child;
	//Object和Array都是通过c语言数组保存，通过接口count()获得子节点的数量
	for (size_t i=0; i<node->count(); ++i)
	{
		//JsonNode& child = (*node)[i];
		child = node->array(i);
		//子节点的key节点丢弃，一般在Array节点存在
		if (child->getKey() == JsonNode::none) 
			printf("JsonNode type=%s, number=%lf\n", child->getTypeName(), child->getNumber());
		else 
			printf("JsonNode type=%s, key=%s, number=%lf\n", child->getTypeName(),child->getKey(), child->getNumber());
	}

	size_t len;
	//把解析出的json，再生成会json字符串数据
	const char* buffer = root.write(&len);
	printf("finish:%s", buffer);
	return 0;
}
```

2.解析json文件
```c++
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "SuperJson.h"
using namespace SuperJson;
int main(int argc, char* argv[])
{
	JsonNode* root = new JsonNode();
	//原json文件
	const char* src_file = "./TestCtrl.json";
	root->readFile(src_file);
	//再把它生成回去，用另一个文件保存
	const char* dst_file = "./TestCtrl123.json";
	root->writeFile(dst_file);
	return 0;
}
```

3.生成json数据
```c++
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "SuperJson.h"
using namespace SuperJson;
int main(int argc, char* argv[])
{
	//root节点
	JsonNode root = JsonNode();

	//创建一个ObjectNode节点  obj = {}
	JsonNode* object = root.newObjectNode();
	//放入到root节点，注意，root节点只能有一个节点
	root.addNode(object);

	//创建一个NumberNode节点  obj['bool'] = true
	JsonNode* numberb = root.newNumberNode();
	numberb->setBool(true);
	object->addNode("bool", numberb);

	//创建一个NumberNode节点  obj['int'] = 323
	JsonNode* numberi = root.newNumberNode();
	numberi->setInteger(323);
	object->addNode("int", numberi);

	//创建一个NumberNode节点  obj['double'] = 234.23
	JsonNode* numberd = root.newNumberNode();
	numberd->setDouble(234.23);
	object->addNode("double", numberd);
	{
		size_t len;
		//把此节点序列化成出来，打印
		const char* buffer = numberd->write(&len);
		printf("stringd:%s \n", buffer);
	}

	//创建一个NumberNode节点  obj['string'] = "test txt"
	JsonNode* stringd = root.newStringNode();
	const char* txt = "test txt";
	stringd->setString(txt, strlen(txt));
	object->addNode("string", stringd);

	//可以把每个节点序列化出来
	{
		size_t len;
		const char* buffer = stringd->write(&len);
		printf("stringd:%s\n", buffer);
	}
	{
		size_t len;
		const char* buffer = object->write(&len);
		printf("object:%s\n", buffer);
	}

	//最后生成整个json字符串
	size_t len;
	const char* buffer = root.write(&len);
	printf("finish:%s\n", buffer);
}
```
输出日志
```
stringd:"double":234.23 
stringd:"string":"test txt"
object:{"bool":1,"int":323,"double":234.23,"string":"test txt"}
finish:{"bool":1,"int":323,"double":234.23,"string":"test txt"}
```

4.迭代整个json结构
```c++
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "SuperJson.h"
using namespace SuperJson;

void iterFunc(JsonNode& node, int depth);
int main(int argc, char* argv[])
{
	const char* test = "{\
		\"bool1\":    true,\
		\"bool2\" :     false,\
		\"int\" :  323,\
		\"long\" :    2147483648,\
		\"double\" :    234.23\
		\"string\" : \"txt\"\
	}";
	JsonNode root = JsonNode();
	root.read(test, strlen(test));
	JsonNode* node = root.array(0);

	iterFunc(*node, 5);
}

void iterFunc(JsonNode& node, int depth)
{
	if (depth >= 7) return;
	switch (node.getType())
	{
	case JsonNode::STRING:
	case JsonNode::NUMBER:
		if (node.getKey() == JsonNode::none)
		{
			if (node.isNumber())
				printf("JsonNode index=%d,type=%s, number=%lf\n", node.getIndex(), node.getTypeName(), node.getNumber());
			else
				printf("JsonNode index=%d,type=%s, string=%s\n", node.getIndex(), node.getTypeName(), node.getString());
		}
		else
		{
			if (node.isNumber())
				printf("JsonNode index=%d, type=%s, key=%s, number=%lf\n", node.getIndex(), node.getTypeName(), node.getKey(), node.getNumber());
			else
				printf("JsonNode index=%d, type=%s, key=%s, string=%s\n", node.getIndex(), node.getTypeName(), node.getKey(), node.getString());
		}
		break;
	case JsonNode::ARRAY:
	case JsonNode::OBJECT:
		if (node.getKey() == JsonNode::none)
			printf("JsonNode:%d:%s\n", node.getIndex(), node.getTypeName());
		else
			printf("JsonNode:%d:%s:%s\n", node.getIndex(), node.getTypeName(), node.getKey());

		for (size_t i = 0; i < node.count(); ++i)
		{
			JsonNode& child = node[i];
			iterFunc(child, depth + 1);
		}
		break;
	case JsonNode::ROOT:
		for (size_t i = 0; i < node.count(); ++i)
		{
			JsonNode& child = node[i];
			iterFunc(child, depth + 1);
		}
		break;
	default:
		break;
	}
}
```

