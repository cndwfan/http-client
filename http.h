#ifndef HTTP_H
#define HTTP_H

#define MAX_JSON_PAIRS 50      // 最大键值对数量
#define MAX_KEY_LENGTH 100     // 键的最大长度
#define MAX_VALUE_LENGTH 500   // 值的最大长度
#define MAX_ARRAY_SIZE 100     // 数组最大元素数量

// JSON 值类型
typedef enum {
	JSON_STRING,
	JSON_NUMBER,
	JSON_BOOLEAN,
	JSON_NULL,
	JSON_OBJECT,
	JSON_ARRAY
} JsonValueType;

// JSON 数组结构体
typedef struct JsonArray {
	JsonValueType element_types[MAX_ARRAY_SIZE];
	union {
		char string_values[MAX_ARRAY_SIZE][MAX_VALUE_LENGTH];
		double number_values[MAX_ARRAY_SIZE];
		int bool_values[MAX_ARRAY_SIZE];
		struct JsonObject* object_values[MAX_ARRAY_SIZE];
	};
	int count;
} JsonArray;

// JSON 值结构体
typedef struct JsonValue {
	char key[MAX_KEY_LENGTH];
	JsonValueType type;
	union {
		char string_value[MAX_VALUE_LENGTH];
		double number_value;
		int bool_value;
		struct JsonObject* object_value;
		struct JsonArray* array_value;
	};
} JsonValue;

// JSON 对象结构体
typedef struct JsonObject {
	JsonValue values[MAX_JSON_PAIRS];
	int count;
} JsonObject;

// 函数声明
int parse_json(const char* json_str, JsonObject* obj);
const char* get_json_string(const JsonObject* obj, const char* key);
double get_json_number(const JsonObject* obj, const char* key);
int get_json_bool(const JsonObject* obj, const char* key);
JsonObject* get_json_object(const JsonObject* obj, const char* key);
JsonArray* get_json_array(const JsonObject* obj, const char* key);

// 数组操作函数
JsonArray* create_json_array();  // 添加这行
int get_array_size(const JsonArray* array);
const char* get_array_string(const JsonArray* array, int index);
double get_array_number(const JsonArray* array, int index);
int get_array_bool(const JsonArray* array, int index);
JsonObject* get_array_object(const JsonArray* array, int index);

void print_json_object(const JsonObject* obj, int indent);
void print_json_array(const JsonArray* array, int indent);  // 添加这行
void clear_json_object(JsonObject* obj);

#endif
