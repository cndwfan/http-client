#include "http.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <stdlib.h>
#include <ctype.h>

#pragma comment(lib, "ws2_32.lib")



// JSON 解析器实现（支持嵌套对象）

// 清除 JSON 对象
void clear_json_object(JsonObject* obj) {
	if (obj == NULL) return;
	obj->count = 0;
	memset(obj->values, 0, sizeof(obj->values));
}

// 跳过空白字符（增加安全检查）
const char* skip_whitespace(const char* str) {
	if (str == NULL) return NULL;

	while (*str && isspace((unsigned char)*str)) {
		str++;
	}
	return str;
}

// 安全的字符串复制函数
static void safe_strcpy(char* dest, size_t dest_size, const char* src, size_t src_len) {
	if (dest == NULL || src == NULL || dest_size == 0) return;

	if (src_len >= dest_size) {
		src_len = dest_size - 1;
	}
	strncpy_s(dest, dest_size, src, src_len);
	dest[src_len] = '\0';
}

// 解析 JSON 值（主要函数）
static const char* parse_json_value(const char* ptr, JsonValue* value);
// 前向声明
static const char* parse_json_array(const char* ptr, JsonArray* array);
JsonArray* create_json_array();
void print_json_array(const JsonArray* array, int indent);

// 解析嵌套对象
static const char* parse_json_object(const char* ptr, JsonObject* obj) {
	if (ptr == NULL || obj == NULL || *ptr != '{') {
		return NULL;
	}

	clear_json_object(obj);
	ptr++; // 跳过 '{'

	while (*ptr && obj->count < MAX_JSON_PAIRS) {
		ptr = skip_whitespace(ptr);
		if (ptr == NULL || *ptr == '\0') return NULL;

		if (*ptr == '}') {
			ptr++; // 跳过 '}'
			break; // 正常结束
		}

		if (*ptr == ',') {
			ptr++;
			ptr = skip_whitespace(ptr);
			if (ptr == NULL || *ptr == '\0') return NULL;
		}

		// 解析键
		if (*ptr != '"') {
			return NULL;
		}

		ptr++; // 跳过 '"'
		if (ptr == NULL || *ptr == '\0') return NULL;

		const char* key_start = ptr;
		while (*ptr && *ptr != '"') ptr++;
		if (*ptr != '"') {
			return NULL;
		}

		size_t key_len = ptr - key_start;
		safe_strcpy(obj->values[obj->count].key, MAX_KEY_LENGTH, key_start, key_len);

		ptr++; // 跳过 '"'
		ptr = skip_whitespace(ptr);
		if (ptr == NULL || *ptr == '\0') return NULL;

		if (*ptr != ':') {
			return NULL;
		}

		ptr++; // 跳过 ':'

		// 解析值
		ptr = parse_json_value(ptr, &obj->values[obj->count]);
		if (ptr == NULL) {
			return NULL;
		}

		obj->count++;
		ptr = skip_whitespace(ptr);
		if (ptr == NULL) return NULL;
	}

	return ptr;
}

// 解析 JSON 值（实现）
static const char* parse_json_value(const char* ptr, JsonValue* value) {
	if (ptr == NULL || value == NULL) return NULL;

	ptr = skip_whitespace(ptr);
	if (ptr == NULL || *ptr == '\0') return NULL;

	if (*ptr == '"') {
		// 字符串值
		value->type = JSON_STRING;
		ptr++; // 跳过 '"'
		const char* value_start = ptr;
		while (*ptr && *ptr != '"') ptr++;
		if (*ptr != '"') return NULL;

		size_t value_len = ptr - value_start;
		safe_strcpy(value->string_value, MAX_VALUE_LENGTH, value_start, value_len);
		ptr++; // 跳过 '"'
	}
	else if (*ptr == '{') {
		// 嵌套对象
		value->type = JSON_OBJECT;
		JsonObject* nested_obj = (JsonObject*)malloc(sizeof(JsonObject));
		if (nested_obj == NULL) return NULL;

		clear_json_object(nested_obj);
		ptr = parse_json_object(ptr, nested_obj);
		if (ptr == NULL) {
			free(nested_obj);
			return NULL;
		}
		value->object_value = nested_obj;
	}
	else if (*ptr == '[') {
		// 数组
		value->type = JSON_ARRAY;
		JsonArray* array = create_json_array();
		if (array == NULL) return NULL;

		ptr = parse_json_array(ptr, array);
		if (ptr == NULL) {
			free(array);
			return NULL;
		}
		value->array_value = array;
	}
	else if (isdigit((unsigned char)*ptr) || *ptr == '-' || *ptr == '.') {
		// 数字值
		value->type = JSON_NUMBER;
		const char* value_start = ptr;
		while (*ptr && (*ptr == '-' || *ptr == '.' || isdigit((unsigned char)*ptr))) {
			ptr++;
		}

		size_t value_len = ptr - value_start;
		char num_str[50];
		safe_strcpy(num_str, sizeof(num_str), value_start, value_len);
		value->number_value = atof(num_str);
	}
	else if (strncmp(ptr, "true", 4) == 0) {
		// 布尔值 true
		value->type = JSON_BOOLEAN;
		value->bool_value = 1;
		ptr += 4;
	}
	else if (strncmp(ptr, "false", 5) == 0) {
		// 布尔值 false
		value->type = JSON_BOOLEAN;
		value->bool_value = 0;
		ptr += 5;
	}
	else if (strncmp(ptr, "null", 4) == 0) {
		// null 值
		value->type = JSON_NULL;
		ptr += 4;
	}
	else {
		return NULL; // 未知类型
	}

	return ptr;
}

// 解析 JSON 字符串
int parse_json(const char* json_str, JsonObject* obj) {
	if (json_str == NULL || obj == NULL) {
		return 0;
	}

	// 使用对象解析函数
	const char* result = parse_json_object(json_str, obj);
	return (result != NULL);
}

// 获取 JSON 字符串值
const char* get_json_string(const JsonObject* obj, const char* key) {
	if (obj == NULL || key == NULL) return NULL;

	for (int i = 0; i < obj->count; i++) {
		if (strcmp(obj->values[i].key, key) == 0 && obj->values[i].type == JSON_STRING) {
			return obj->values[i].string_value;
		}
	}
	return NULL;
}

// 获取 JSON 数字值
double get_json_number(const JsonObject* obj, const char* key) {
	if (obj == NULL || key == NULL) return 0.0;

	for (int i = 0; i < obj->count; i++) {
		if (strcmp(obj->values[i].key, key) == 0 && obj->values[i].type == JSON_NUMBER) {
			return obj->values[i].number_value;
		}
	}
	return 0.0;
}

// 获取 JSON 布尔值
int get_json_bool(const JsonObject* obj, const char* key) {
	if (obj == NULL || key == NULL) return 0;

	for (int i = 0; i < obj->count; i++) {
		if (strcmp(obj->values[i].key, key) == 0 && obj->values[i].type == JSON_BOOLEAN) {
			return obj->values[i].bool_value;
		}
	}
	return 0;
}

// 获取 JSON 对象
JsonObject* get_json_object(const JsonObject* obj, const char* key) {
	if (obj == NULL || key == NULL) return NULL;

	for (int i = 0; i < obj->count; i++) {
		if (strcmp(obj->values[i].key, key) == 0 && obj->values[i].type == JSON_OBJECT) {
			return obj->values[i].object_value;
		}
	}
	return NULL;
}
// 创建 JSON 数组
JsonArray* create_json_array() {
	JsonArray* array = (JsonArray*)malloc(sizeof(JsonArray));
	if (array == NULL) return NULL;
	array->count = 0;
	return array;
}

// 解析 JSON 数组
static const char* parse_json_array(const char* ptr, JsonArray* array) {
	if (ptr == NULL || array == NULL || *ptr != '[') {
		return NULL;
	}

	array->count = 0;
	ptr++; // 跳过 '['

	while (*ptr && array->count < MAX_ARRAY_SIZE) {
		ptr = skip_whitespace(ptr);
		if (ptr == NULL || *ptr == '\0') return NULL;

		if (*ptr == ']') {
			ptr++; // 跳过 ']'
			break; // 正常结束
		}

		if (*ptr == ',') {
			ptr++;
			ptr = skip_whitespace(ptr);
			if (ptr == NULL || *ptr == '\0') return NULL;
		}

		// 根据元素类型解析
		ptr = skip_whitespace(ptr);
		if (ptr == NULL || *ptr == '\0') return NULL;

		if (*ptr == '"') {
			// 字符串元素
			array->element_types[array->count] = JSON_STRING;
			ptr++; // 跳过 '"'
			const char* value_start = ptr;
			while (*ptr && *ptr != '"') ptr++;
			if (*ptr != '"') return NULL;

			size_t value_len = ptr - value_start;
			safe_strcpy(array->string_values[array->count], MAX_VALUE_LENGTH, value_start, value_len);
			ptr++; // 跳过 '"'
		}
		else if (*ptr == '{') {
			// 对象元素
			array->element_types[array->count] = JSON_OBJECT;
			JsonObject* nested_obj = (JsonObject*)malloc(sizeof(JsonObject));
			if (nested_obj == NULL) return NULL;

			clear_json_object(nested_obj);
			ptr = parse_json_object(ptr, nested_obj);
			if (ptr == NULL) {
				free(nested_obj);
				return NULL;
			}
			array->object_values[array->count] = nested_obj;
		}
		else if (isdigit((unsigned char)*ptr) || *ptr == '-' || *ptr == '.') {
			// 数字元素
			array->element_types[array->count] = JSON_NUMBER;
			const char* value_start = ptr;
			while (*ptr && (*ptr == '-' || *ptr == '.' || isdigit((unsigned char)*ptr))) {
				ptr++;
			}

			size_t value_len = ptr - value_start;
			char num_str[50];
			safe_strcpy(num_str, sizeof(num_str), value_start, value_len);
			array->number_values[array->count] = atof(num_str);
		}
		else if (strncmp(ptr, "true", 4) == 0) {
			// 布尔值 true
			array->element_types[array->count] = JSON_BOOLEAN;
			array->bool_values[array->count] = 1;
			ptr += 4;
		}
		else if (strncmp(ptr, "false", 5) == 0) {
			// 布尔值 false
			array->element_types[array->count] = JSON_BOOLEAN;
			array->bool_values[array->count] = 0;
			ptr += 5;
		}
		else if (strncmp(ptr, "null", 4) == 0) {
			// null 值
			array->element_types[array->count] = JSON_NULL;
			ptr += 4;
		}
		else {
			return NULL; // 未知类型
		}

		array->count++;
		ptr = skip_whitespace(ptr);
		if (ptr == NULL) return NULL;
	}

	return ptr;
}
// 获取 JSON 数组
JsonArray* get_json_array(const JsonObject* obj, const char* key) {
	if (obj == NULL || key == NULL) return NULL;

	for (int i = 0; i < obj->count; i++) {
		if (strcmp(obj->values[i].key, key) == 0 && obj->values[i].type == JSON_ARRAY) {
			return obj->values[i].array_value;
		}
	}
	return NULL;
}

// 数组操作函数
int get_array_size(const JsonArray* array) {
	if (array == NULL) return 0;
	return array->count;
}

const char* get_array_string(const JsonArray* array, int index) {
	if (array == NULL || index < 0 || index >= array->count) return NULL;
	if (array->element_types[index] != JSON_STRING) return NULL;
	return array->string_values[index];
}

double get_array_number(const JsonArray* array, int index) {
	if (array == NULL || index < 0 || index >= array->count) return 0.0;
	if (array->element_types[index] != JSON_NUMBER) return 0.0;
	return array->number_values[index];
}

int get_array_bool(const JsonArray* array, int index) {
	if (array == NULL || index < 0 || index >= array->count) return 0;
	if (array->element_types[index] != JSON_BOOLEAN) return 0;
	return array->bool_values[index];
}

JsonObject* get_array_object(const JsonArray* array, int index) {
	if (array == NULL || index < 0 || index >= array->count) return NULL;
	if (array->element_types[index] != JSON_OBJECT) return NULL;
	return array->object_values[index];
}
// 更新 print_json_object 函数，添加数组支持
void print_json_array(const JsonArray* array, int indent) {
	if (array == NULL) {
		printf("JSON array is NULL\n");
		return;
	}

	char indent_str[20];
	memset(indent_str, ' ', indent * 2);
	indent_str[indent * 2] = '\0';

	printf("%sJSON Array (%d items):\n", indent_str, array->count);
	for (int i = 0; i < array->count; i++) {
		printf("%s  [%d]: ", indent_str, i);

		switch (array->element_types[i]) {
		case JSON_STRING:
			printf("\"%s\" (string)", array->string_values[i]);
			break;
		case JSON_NUMBER:
			printf("%g (number)", array->number_values[i]);
			break;
		case JSON_BOOLEAN:
			printf("%s (boolean)", array->bool_values[i] ? "true" : "false");
			break;
		case JSON_NULL:
			printf("null");
			break;
		case JSON_OBJECT:
			printf("{\n");
			print_json_object(array->object_values[i], indent + 2);
			printf("%s  }", indent_str);
			break;
		}
		printf("\n");
	}
}


// 打印 JSON 对象（支持嵌套）
void print_json_object(const JsonObject* obj, int indent) {
	if (obj == NULL) {
		printf("JSON object is NULL\n");
		return;
	}

	char indent_str[20];
	memset(indent_str, ' ', indent * 2);
	indent_str[indent * 2] = '\0';

	printf("%sJSON Object (%d items):\n", indent_str, obj->count);
	for (int i = 0; i < obj->count; i++) {
		printf("%s  %s: ", indent_str, obj->values[i].key);

		switch (obj->values[i].type) {
		case JSON_STRING:
			printf("\"%s\" (string)", obj->values[i].string_value);
			break;
		case JSON_NUMBER:
			printf("%g (number)", obj->values[i].number_value);
			break;
		case JSON_BOOLEAN:
			printf("%s (boolean)", obj->values[i].bool_value ? "true" : "false");
			break;
		case JSON_NULL:
			printf("null");
			break;
		case JSON_OBJECT:
			printf("{\n");
			print_json_object(obj->values[i].object_value, indent + 2);
			printf("%s  }", indent_str);
			break;
		case JSON_ARRAY:
			printf("[\n");
			print_json_array(obj->values[i].array_value, indent + 2);
			printf("%s  ]", indent_str);
			break;
		}
		printf("\n");
	}
}

// [其余函数保持不变：URL编码、HTTP请求等]
// ... 保持原有的 URL 编码、HTTP 请求等函数不变

// URL 编码函数
char* url_encode(const char* str) {
	if (str == NULL) return NULL;

	// 计算编码后的长度
	size_t len = strlen(str);
	size_t encoded_len = 0;

	for (size_t i = 0; i < len; i++) {
		unsigned char c = str[i];
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
			encoded_len++;
		}
		else {
			encoded_len += 3; // %XX
		}
	}

	// 分配内存
	char* encoded = (char*)malloc(encoded_len + 1);
	if (encoded == NULL) return NULL;

	// 进行编码
	size_t pos = 0;
	for (size_t i = 0; i < len; i++) {
		unsigned char c = str[i];

		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
			// 不需要编码的字符
			encoded[pos++] = c;
		}
		else {
			// 需要编码的字符
			sprintf_s(encoded + pos, encoded_len - pos + 1, "%%%02X", c);
			pos += 3;
		}
	}

	encoded[pos] = '\0';
	return encoded;
}

// URL 解码函数
char* url_decode(const char* str) {
	if (str == NULL) return NULL;

	size_t len = strlen(str);
	char* decoded = (char*)malloc(len + 1);
	if (decoded == NULL) return NULL;

	size_t pos = 0;
	for (size_t i = 0; i < len; i++) {
		if (str[i] == '%' && i + 2 < len) {
			// 处理 %XX 格式
			char hex[3] = { str[i + 1], str[i + 2], '\0' };
			int value;
			if (sscanf_s(hex, "%02x", &value) == 1) {
				decoded[pos++] = (char)value;
				i += 2;
			}
			else {
				decoded[pos++] = str[i];
			}
		}
		else if (str[i] == '+') {
			// + 号解码为空格
			decoded[pos++] = ' ';
		}
		else {
			decoded[pos++] = str[i];
		}
	}

	decoded[pos] = '\0';
	return decoded;
}

// UTF-8 转 GBK 转换函数
char* utf8_to_gbk(const char* utf8_str) {
	if (utf8_str == NULL || strlen(utf8_str) == 0) {
		return NULL;
	}

	// UTF-8 转 Unicode
	int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
	if (wlen == 0) {
		return NULL;
	}

	wchar_t* wstr = (wchar_t*)malloc(wlen * sizeof(wchar_t));
	if (wstr == NULL) {
		return NULL;
	}

	if (MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, wstr, wlen) == 0) {
		free(wstr);
		return NULL;
	}

	// Unicode 转 GBK
	int glen = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
	if (glen == 0) {
		free(wstr);
		return NULL;
	}

	char* gbk_str = (char*)malloc(glen);
	if (gbk_str == NULL) {
		free(wstr);
		return NULL;
	}

	if (WideCharToMultiByte(CP_ACP, 0, wstr, -1, gbk_str, glen, NULL, NULL) == 0) {
		free(wstr);
		free(gbk_str);
		return NULL;
	}

	free(wstr);
	return gbk_str;
}

// 直接打印 UTF-8 字符串为 GBK
void print_utf8_as_gbk(const char* utf8_str) {
	if (utf8_str == NULL) {
		return;
	}

	char* gbk_str = utf8_to_gbk(utf8_str);
	if (gbk_str != NULL) {
		printf("%s", gbk_str);
		free(gbk_str);
	}
	else {
		// 转换失败，直接输出（可能会有乱码）
		printf("%s", utf8_str);
	}
}

// 构建查询字符串 (key1=value1&key2=value2)，自动进行 URL 编码
char* build_query_string(const char** params, int param_count) {
	if (params == NULL || param_count <= 0) {
		return NULL;
	}

	// 第一遍：计算总长度
	size_t total_len = 0;
	for (int i = 0; i < param_count; i += 2) {
		if (params[i] != NULL) {
			// 键和值都需要编码
			char* encoded_key = url_encode(params[i]);
			char* encoded_value = url_encode(params[i + 1]);

			if (encoded_key) total_len += strlen(encoded_key);
			total_len += 1; // '='
			if (encoded_value) total_len += strlen(encoded_value);
			if (i + 2 < param_count) total_len += 1; // '&'

			free(encoded_key);
			free(encoded_value);
		}
	}

	if (total_len == 0) {
		return NULL;
	}

	// 分配内存并构建字符串
	char* query = (char*)malloc(total_len + 1);
	if (query == NULL) {
		return NULL;
	}

	query[0] = '\0';
	for (int i = 0; i < param_count; i += 2) {
		if (params[i] != NULL) {
			if (i > 0) {
				strcat_s(query, total_len + 1, "&");
			}

			char* encoded_key = url_encode(params[i]);
			char* encoded_value = url_encode(params[i + 1]);

			if (encoded_key) {
				strcat_s(query, total_len + 1, encoded_key);
				strcat_s(query, total_len + 1, "=");
				if (encoded_value) {
					strcat_s(query, total_len + 1, encoded_value);
				}
			}

			free(encoded_key);
			free(encoded_value);
		}
	}

	return query;
}

// 内部 HTTP 请求函数
static const char* http_request(const char* hostname, const char* port, const char* path,
	const char* method, const char* content_type, const char* data) {
	WSADATA wsa;
	SOCKET sock;
	struct addrinfo hints, *result, *ptr;
	char request[4096];
	static char response[8192];
	int bytes_received;
	int iResult;
	int total_received = 0;

	// 初始化缓冲区
	memset(response, 0, sizeof(response));

	// 初始化 Winsock
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		return "WSAStartup failed";
	}

	// 设置 addrinfo 提示
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// 解析地址和端口
	iResult = getaddrinfo(hostname, port, &hints, &result);
	if (iResult != 0) {
		WSACleanup();
		return "getaddrinfo failed";
	}

	// 尝试每个返回的地址，直到成功连接
	sock = INVALID_SOCKET;
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (sock == INVALID_SOCKET) {
			continue;
		}

		if (connect(sock, ptr->ai_addr, (int)ptr->ai_addrlen) != SOCKET_ERROR) {
			break;
		}

		closesocket(sock);
		sock = INVALID_SOCKET;
	}

	freeaddrinfo(result);

	if (sock == INVALID_SOCKET) {
		WSACleanup();
		return "Unable to connect to server";
	}

	// 构建请求
	if (data != NULL && strcmp(method, "POST") == 0) {
		// POST 请求
		snprintf(request, sizeof(request),
			"%s %s HTTP/1.1\r\n"
			"Host: %s\r\n"
			"User-Agent: C-HTTP-Client/1.0\r\n"
			"Content-Type: %s\r\n"
			"Content-Length: %d\r\n"
			"Connection: close\r\n"
			"\r\n"
			"%s",
			method, path, hostname, content_type, (int)strlen(data), data);
	}
	else {
		// GET 请求
		snprintf(request, sizeof(request),
			"%s %s HTTP/1.1\r\n"
			"Host: %s\r\n"
			"User-Agent: C-HTTP-Client/1.0\r\n"
			"Connection: close\r\n"
			"\r\n",
			method, path, hostname);
	}

	// 发送请求
	if (send(sock, request, strlen(request), 0) == SOCKET_ERROR) {
		closesocket(sock);
		WSACleanup();
		return "send failed";
	}

	// 接收响应
	while ((bytes_received = recv(sock, response + total_received,
		sizeof(response) - total_received - 1, 0)) > 0) {
		total_received += bytes_received;
		if (total_received >= sizeof(response) - 1) {
			break;
		}
	}

	response[total_received] = '\0';

	closesocket(sock);
	WSACleanup();

	return response;
}

// 简单的 HTTP GET 实现
const char* http_get(const char* hostname, const char* port, const char* path) {
	return http_request(hostname, port, path, "GET", NULL, NULL);
}

// 带参数的 GET 请求（自动 URL 编码）
const char* http_get_with_params(const char* hostname, const char* port, const char* path, const char* params) {
	char full_path[1024];
	if (params != NULL && strlen(params) > 0) {
		// 对参数进行 URL 编码
		char* encoded_params = url_encode(params);
		if (encoded_params != NULL) {
			snprintf(full_path, sizeof(full_path), "%s?%s", path, encoded_params);
			free(encoded_params);
		}
		else {
			snprintf(full_path, sizeof(full_path), "%s?%s", path, params);
		}
	}
	else {
		strncpy_s(full_path, sizeof(full_path), path, _TRUNCATE);
	}
	return http_request(hostname, port, full_path, "GET", NULL, NULL);
}

// 简单的 POST 请求
const char* http_post(const char* hostname, const char* port, const char* path, const char* data) {
	return http_request(hostname, port, path, "POST", "application/json", data);
}

// 表单 POST 请求
const char* http_post_form(const char* hostname, const char* port, const char* path, const char* form_data) {
	return http_request(hostname, port, path, "POST", "application/x-www-form-urlencoded", form_data);
}
