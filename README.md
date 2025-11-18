# HTTP 操作库完整使用教程

本教程将详细介绍如何使用附件中的 HTTP 操作库，包括 JSON 解析、URL 编码/解码、HTTP 请求等功能。

## 目录

1. [JSON 解析功能](#json-解析功能)
2. [URL 编码/解码](#url-编码解码)
3. [HTTP 请求](#http-请求)
4. [字符编码转换](#字符编码转换)
5. [完整示例](#完整示例)

## JSON 解析功能

### 基本 JSON 对象解析

```c
#include "http.h"
#include <stdio.h>

int main() {
    const char* json_str = "{\"name\":\"张三\",\"age\":25,\"isStudent\":true}";
    JsonObject obj;
    
    // 解析 JSON
    if (parse_json(json_str, &obj)) {
        printf("解析成功！\n");
        
        // 获取字符串值
        const char* name = get_json_string(&obj, "name");
        if (name) {
            printf("姓名: %s\n", name);
        }
        
        // 获取数字值
        double age = get_json_number(&obj, "age");
        printf("年龄: %.0f\n", age);
        
        // 获取布尔值
        int isStudent = get_json_bool(&obj, "isStudent");
        printf("是否学生: %s\n", isStudent ? "是" : "否");
        
        // 打印整个 JSON 对象
        printf("\n完整 JSON 结构:\n");
        print_json_object(&obj, 0);
    } else {
        printf("解析失败！\n");
    }
    
    return 0;
}
```

### 嵌套 JSON 对象解析

```c
#include "http.h"
#include <stdio.h>

int main() {
    const char* json_str = "{\"person\":{\"name\":\"李四\",\"address\":{\"city\":\"北京\",\"street\":\"长安街\"}},\"scores\":[95,87,92]}";
    JsonObject obj;
    
    if (parse_json(json_str, &obj)) {
        printf("解析成功！\n");
        
        // 获取嵌套对象
        JsonObject* person = get_json_object(&obj, "person");
        if (person) {
            const char* name = get_json_string(person, "name");
            printf("姓名: %s\n", name);
            
            // 获取更深层的嵌套对象
            JsonObject* address = get_json_object(person, "address");
            if (address) {
                const char* city = get_json_string(address, "city");
                const char* street = get_json_string(address, "street");
                printf("城市: %s, 街道: %s\n", city, street);
            }
        }
        
        // 获取数组
        JsonArray* scores = get_json_array(&obj, "scores");
        if (scores) {
            printf("分数: ");
            for (int i = 0; i < get_array_size(scores); i++) {
                double score = get_array_number(scores, i);
                printf("%.0f ", score);
            }
            printf("\n");
        }
        
        printf("\n完整 JSON 结构:\n");
        print_json_object(&obj, 0);
    }
    
    return 0;
}
```

### JSON 数组操作

```c
#include "http.h"
#include <stdio.h>

int main() {
    const char* json_str = "{\"users\":[{\"name\":\"王五\",\"age\":30},{\"name\":\"赵六\",\"age\":28}],\"tags\":[\"C语言\",\"编程\",\"教程\"]}";
    JsonObject obj;
    
    if (parse_json(json_str, &obj)) {
        // 获取对象数组
        JsonArray* users = get_json_array(&obj, "users");
        if (users) {
            printf("用户列表:\n");
            for (int i = 0; i < get_array_size(users); i++) {
                JsonObject* user = get_array_object(users, i);
                if (user) {
                    const char* name = get_json_string(user, "name");
                    double age = get_json_number(user, "age");
                    printf("  用户%d: %s, %d岁\n", i+1, name, (int)age);
                }
            }
        }
        
        // 获取字符串数组
        JsonArray* tags = get_json_array(&obj, "tags");
        if (tags) {
            printf("标签: ");
            for (int i = 0; i < get_array_size(tags); i++) {
                const char* tag = get_array_string(tags, i);
                printf("%s ", tag);
            }
            printf("\n");
        }
        
        printf("\n完整 JSON 结构:\n");
        print_json_object(&obj, 0);
    }
    
    return 0;
}
```

## URL 编码/解码

### URL 编码示例

```c
#include "http.h"
#include <stdio.h>

int main() {
    // 需要编码的字符串
    const char* original = "Hello 世界! @#$%";
    
    // URL 编码
    char* encoded = url_encode(original);
    if (encoded) {
        printf("原始字符串: %s\n", original);
        printf("编码后: %s\n", encoded);
        
        // 记得释放内存
        free(encoded);
    }
    
    return 0;
}
```

### URL 解码示例

```c
#include "http.h"
#include <stdio.h>

int main() {
    // 编码后的 URL
    const char* encoded = "Hello%20%E4%B8%96%E7%95%8C%21%20%40%23%24%25";
    
    // URL 解码
    char* decoded = url_decode(encoded);
    if (decoded) {
        printf("编码字符串: %s\n", encoded);
        printf("解码后: %s\n", decoded);
        
        free(decoded);
    }
    
    return 0;
}
```

### 构建查询字符串

```c
#include "http.h"
#include <stdio.h>

int main() {
    // 构建查询参数
    const char* params[] = {
        "name", "张三",
        "age", "25",
        "city", "北京",
        "interests", "编程 音乐",  // 包含空格的参数会自动编码
        NULL  // 结束标记
    };
    
    // 构建查询字符串
    char* query = build_query_string(params, 8);  // 8 是参数数量（键值对总数）
    if (query) {
        printf("生成的查询字符串: %s\n", query);
        
        // 使用示例
        printf("完整URL: http://api.example.com/user?%s\n", query);
        
        free(query);
    }
    
    return 0;
}
```

## HTTP 请求

### GET 请求示例

```c
#include "http.h"
#include <stdio.h>

int main() {
    printf("发送 GET 请求...\n");
    
    // 简单 GET 请求
    const char* response = http_get("httpbin.org", "80", "/get");
    if (response) {
        printf("服务器响应:\n%s\n", response);
        
        // 解析 JSON 响应
        JsonObject obj;
        if (parse_json(response, &obj)) {
            const char* url = get_json_string(&obj, "url");
            if (url) {
                printf("请求的URL: %s\n", url);
            }
        }
    }
    
    return 0;
}
```

### 带参数的 GET 请求

```c
#include "http.h"
#include <stdio.h>

int main() {
    printf("发送带参数的 GET 请求...\n");
    
    // 方法1：手动构建查询字符串
    const char* response1 = http_get_with_params("httpbin.org", "80", "/get", "name=张三&age=25&city=北京");
    if (response1) {
        printf("响应1:\n%s\n", response1);
    }
    
    // 方法2：使用 build_query_string
    const char* params[] = {"name", "李四", "age", "30", "language", "C/C++"};
    char* query = build_query_string(params, 6);
    if (query) {
        const char* response2 = http_get_with_params("httpbin.org", "80", "/get", query);
        if (response2) {
            printf("响应2:\n%s\n", response2);
        }
        free(query);
    }
    
    return 0;
}
```

### POST 请求示例

```c
#include "http.h"
#include <stdio.h>

int main() {
    printf("发送 POST 请求...\n");
    
    // JSON POST 请求
    const char* json_data = "{\"title\":\"测试文章\",\"content\":\"这是一篇测试文章的内容\",\"author\":\"张三\"}";
    const char* response = http_post("httpbin.org", "80", "/post", json_data);
    
    if (response) {
        printf("服务器响应:\n%s\n", response);
        
        // 解析响应
        JsonObject obj;
        if (parse_json(response, &obj)) {
            JsonObject* data_obj = get_json_object(&obj, "json");
            if (data_obj) {
                const char* title = get_json_string(data_obj, "title");
                const char* author = get_json_string(data_obj, "author");
                printf("提交的数据 - 标题: %s, 作者: %s\n", title, author);
            }
        }
    }
    
    return 0;
}
```

### 表单 POST 请求

```c
#include "http.h"
#include <stdio.h>

int main() {
    printf("发送表单 POST 请求...\n");
    
    // 构建表单数据
    const char* form_params[] = {
        "username", "testuser",
        "password", "testpass123",
        "email", "test@example.com"
    };
    
    char* form_data = build_query_string(form_params, 6);
    if (form_data) {
        const char* response = http_post_form("httpbin.org", "80", "/post", form_data);
        
        if (response) {
            printf("服务器响应:\n%s\n", response);
            
            // 解析响应
            JsonObject obj;
            if (parse_json(response, &obj)) {
                JsonObject* form_obj = get_json_object(&obj, "form");
                if (form_obj) {
                    const char* username = get_json_string(form_obj, "username");
                    const char* email = get_json_string(form_obj, "email");
                    printf("表单数据 - 用户名: %s, 邮箱: %s\n", username, email);
                }
            }
        }
        
        free(form_data);
    }
    
    return 0;
}
```

## 字符编码转换

### UTF-8 转 GBK

```c
#include "http.h"
#include <stdio.h>

int main() {
    const char* utf8_text = "Hello 世界！这是一段UTF-8编码的中文文本。";
    
    printf("原始UTF-8文本: %s\n", utf8_text);
    
    // 转换为GBK
    char* gbk_text = utf8_to_gbk(utf8_text);
    if (gbk_text) {
        printf("转换为GBK: %s\n", gbk_text);
        free(gbk_text);
    }
    
    // 直接打印UTF-8为GBK（适用于中文Windows控制台）
    printf("直接打印(GBK): ");
    print_utf8_as_gbk(utf8_text);
    printf("\n");
    
    return 0;
}
```

## 完整示例

### 综合示例：调用天气API

```c
#include "http.h"
#include <stdio.h>
#include <string.h>

// 模拟天气API响应处理
void process_weather_response(const char* response) {
    JsonObject obj;
    
    if (parse_json(response, &obj)) {
        // 获取城市信息
        JsonObject* location = get_json_object(&obj, "location");
        if (location) {
            const char* city = get_json_string(location, "name");
            const char* country = get_json_string(location, "country");
            printf("城市: %s, 国家: %s\n", city, country);
        }
        
        // 获取当前天气
        JsonObject* current = get_json_object(&obj, "current");
        if (current) {
            double temp = get_json_number(current, "temp_c");
            const char* condition = get_json_string(current, "condition");
            if (condition) {
                JsonObject* condition_obj = get_json_object(current, "condition");
                if (condition_obj) {
                    const char* text = get_json_string(condition_obj, "text");
                    printf("当前温度: %.1f°C, 天气状况: %s\n", temp, text);
                }
            }
        }
        
        // 打印完整响应结构（调试用）
        printf("\n完整API响应结构:\n");
        print_json_object(&obj, 0);
    } else {
        printf("解析天气数据失败！\n");
        printf("原始响应: %s\n", response);
    }
}

int main() {
    printf("=== 天气查询系统 ===\n");
    
    // 构建请求参数
    const char* params[] = {
        "key", "your_api_key_here",  // 替换为实际的API密钥
        "q", "Beijing",
        "aqi", "no"
    };
    
    char* query = build_query_string(params, 6);
    if (query) {
        printf("发送天气API请求...\n");
        
        // 发送GET请求（这里使用模拟数据，实际使用时需要有效的API端点）
        const char* response = http_get_with_params("api.weatherapi.com", "80", "/v1/current.json", query);
        
        if (response && strstr(response, "HTTP") != response) {
            // 成功收到JSON响应
            process_weather_response(response);
        } else {
            printf("请求失败或返回错误: %s\n", response);
            
            // 使用模拟数据进行演示
            printf("\n使用模拟数据进行演示:\n");
            const char* mock_response = 
                "{\"location\":{\"name\":\"北京\",\"region\":\"\",\"country\":\"中国\"},"
                "\"current\":{\"temp_c\":22.0,\"condition\":{\"text\":\"晴朗\"}}}";
            process_weather_response(mock_response);
        }
        
        free(query);
    }
    
    return 0;
}
```

### 错误处理示例

```c
#include "http.h"
#include <stdio.h>

void handle_http_error(const char* response) {
    if (strstr(response, "failed") != NULL || 
        strstr(response, "Unable") != NULL ||
        strstr(response, "error") != NULL) {
        printf("HTTP请求错误: %s\n", response);
        return;
    }
    
    // 检查HTTP状态码
    if (strstr(response, "HTTP/1.1 200") == NULL) {
        printf("服务器返回非200状态码\n");
    }
}

int main() {
    // 测试错误情况
    printf("测试错误处理...\n");
    
    // 1. 无效的主机名
    const char* response1 = http_get("invalid-hostname-xyz", "80", "/");
    handle_http_error(response1);
    
    // 2. 无效的JSON
    const char* invalid_json = "{\"name\":\"张三\",\"age\":}"; // 无效的JSON
    JsonObject obj;
    if (!parse_json(invalid_json, &obj)) {
        printf("JSON解析失败（符合预期）\n");
    }
    
    // 3. 不存在的键
    const char* valid_json = "{\"name\":\"张三\",\"age\":25}";
    if (parse_json(valid_json, &obj)) {
        const char* nonexistent = get_json_string(&obj, "nonexistent_key");
        if (nonexistent == NULL) {
            printf("获取不存在的键返回NULL（符合预期）\n");
        }
    }
    
    return 0;
}
```

## 使用注意事项

1. **内存管理**：
   - `url_encode()`、`url_decode()`、`utf8_to_gbk()`、`build_query_string()` 返回的字符串需要手动调用 `free()` 释放
   - JSON 对象中的嵌套对象和数组会在 `clear_json_object()` 时自动释放

2. **错误处理**：
   - 所有函数都返回 NULL 或 0 表示失败
   - HTTP 请求函数在失败时返回错误消息字符串

3. **编码问题**：
   - 在中文 Windows 环境下，使用 `print_utf8_as_gbk()` 正确显示中文
   - 所有字符串参数都应该是 UTF-8 编码

4. **性能考虑**：
   - 适用于简单的 HTTP 请求和 JSON 解析
   - 对于大量数据或高性能需求，建议使用专门的库

这个库提供了完整的 HTTP 客户端功能和 JSON 解析能力，适合嵌入式系统、学习用途或简单的应用程序开发。



