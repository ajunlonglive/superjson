
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "SuperJson.h"


using namespace SuperJson;

void iterFunc(JsonNode& node, int depth);

void test1()
{
	printf("\nstart test1\n");
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
	printf("end test1\n");
}

void test2()
{
	printf("\nstart test2\n");
	JsonNode root = JsonNode();
	JsonNode* object = root.newObjectNode();
	root.addNode(object);

	JsonNode* numberb = root.newNumberNode();
	numberb->setBool(true);
	object->addNode("bool", numberb);

	JsonNode* numberi = root.newNumberNode();
	numberi->setInteger(323);
	object->addNode("int", numberi);

	JsonNode* numberd = root.newNumberNode();
	numberd->setDouble(234.23);
	object->addNode("double", numberd);
	{
		size_t len;
		const char* buffer = numberd->write(&len);
		printf("stringd:%s \n", buffer);
	}

	JsonNode* stringd = root.newStringNode();
	const char* txt = "test txt";
	stringd->setString(txt, strlen(txt));
	object->addNode("string", stringd);

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

	size_t len;
	const char* buffer = root.write(&len);
	printf("finish:%s\n", buffer);
	printf("end test2\n");
}

void test3()
{
	printf("\nstart test3\n");
	const char* test = "{\
		\"bool1\":    true,\
		\"bool2\" :     false,\
		\"int\" :  323,\
		\"long\" :    2147483648,\
		\"double\" :    234.23\
		\"string\" : \"txt\"\
	}";
	printf("start:%s", test);

	JsonNode root = JsonNode();
	root.read(test, strlen(test));

	JsonNode* node = root.array(0);
	assert(node->isObject());

	JsonNode* bool1node = node->object("bool1");
	printf("key=%s value=%d\n", bool1node->getKey(), (int)bool1node->getBool());

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
	for (size_t i=0; i<node->count(); ++i)
	{
		//JsonNode& child = (*node)[i];
		child = node->array(i);
		if (child->getKey() == JsonNode::none) 
			printf("JsonNode type=%s, number=%lf\n", child->getTypeName(), child->getNumber());
		else 
			printf("JsonNode type=%s, key=%s, number=%lf\n", child->getTypeName(),child->getKey(), child->getNumber());
	}
	size_t len;
	const char* buffer = root.write(&len);
	printf("finish:%s", buffer);
	printf("end test3\n");
}

void test4()
{
	printf("\nstart test4\n");
	JsonNode* root = new JsonNode();
	const char* src_file = "E:\\git\\superjson\\TestCtrl.json";
	root->readFile(src_file);
	const char* dst_file = "E:\\git\\superjson\\TestCtrl123.json";
	root->writeFile(dst_file);
	printf("end test4\n");
}


int main(int argc, char* argv[])
{
	test1();
	test2();
	test3();
	test4();
	return 0;
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