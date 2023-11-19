#ifndef _DSJSON_JSON_ELEMENT_H_
#define _DSJSON_JSON_ELEMENT_H_

#include "./cstring.h"
#include <stdint.h>

typedef struct JsonElement JsonElement;
typedef struct Property Property;

typedef struct JsonArray
{
    JsonElement *array;
    size_t size;
    size_t capacity;
} JsonArray;

typedef struct JsonObject
{
    size_t size;
    size_t capacity;
    Property *properties;
} JsonObject;

typedef enum
{
    JSON_TYPE_STRING,
    JSON_TYPE_OBJECT,
    JSON_TYPE_ARRAY,
    JSON_TYPE_BOOLEAN,
    JSON_TYPE_NULL,
    JSON_TYPE_NUMBER,
    JSON_TYPE_ERROR,
} ValueType;

typedef struct JsonElement
{
    ValueType type;
    union
    {
        String string;
        JsonObject object;
        JsonArray array;
        bool boolean;
        double number;
    } value;
} JsonElement;

typedef struct Property
{
    String key;
    JsonElement value;
} Property;

//=========================Json Object===========================

#define TABLE_MAX_LOAD 0.75

#define GROW_CAPACITY(oldCapacity) oldCapacity * 2

uint32_t hash_string(const char *key, size_t length)
{
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < length; i++)
    {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

void json_object_init(JsonObject *object, size_t initialSize)
{
    object->size = 0;
    object->capacity = 0;

    object->properties = (Property *)calloc(initialSize, sizeof(Property));

    if (object->properties == NULL)
    {
        perror("malloc");
        return;
    }

    object->capacity = initialSize;
}

Property *json_object_find_property(Property *properties, size_t capacity, String *key)
{
    uint32_t index = hash_string(key->data, key->size) % capacity;

    for (;;)
    {
        Property *property = &properties[index];
        if (property->key.data == NULL || string_compare_string(key, &property->key) == true)
            return property;

        index = (index + 1) % capacity;
    }
}

static void adjust_table(JsonObject *object, size_t newCapacity)
{
    Property *properties = (Property *)malloc(sizeof(Property) * newCapacity);

    for (size_t i = 0; i < newCapacity; i++)
    {
        properties[i].key.data = NULL;
    }

    for (size_t i = 0; i < object->capacity; i++)
    {
        Property *property = &object->properties[i];
        if (property->key.data == NULL)
            continue;

        Property *dest = json_object_find_property(properties, newCapacity, &property->key);

        dest->key = property->key;
        dest->value = property->value;
    }

    object->properties = properties;
    object->capacity = newCapacity;
}

bool json_object_add(JsonObject *object, String key, JsonElement value)
{

    if (object->size + 1 > object->capacity * TABLE_MAX_LOAD)
    {
        int newCapacity = GROW_CAPACITY(object->capacity);

        adjust_table(object, newCapacity);
    }

    Property *property = json_object_find_property(object->properties, object->capacity, &key);

    bool isNewKey = property->key.data == NULL;
    if (isNewKey)
        object->size++;

    property->key = key;
    property->value = value;

    return isNewKey;
}

bool json_object_get(JsonObject *object, String key, JsonElement *value)
{
    Property *property = json_object_find_property(object->properties, object->capacity, &key);

    if (property->key.data == NULL)
        return false;

    *value = property->value;
    return true;
}

//==========================Json Array============================

void json_array_init(JsonArray *array, size_t size)
{
    array->size = 0;
    array->capacity = 0;
    array->array = (JsonElement *)malloc(sizeof(JsonElement) * size);
    if (array->array == NULL)
    {
        fprintf(stderr, "[ERROR] Cannot init json array\n");
        return;
    }
    array->capacity = size;
}

bool json_array_add_element(JsonArray *array, JsonElement element)
{
    if ((array->size + 2) > array->capacity)
    {
        array->array = (JsonElement *)realloc(array->array, sizeof(JsonElement) * array->capacity * 2);
        if (array->array == NULL)
        {
            fprintf(stderr, "[ERROR] Cannot realloc for json array\n");
            return false;
        }
        array->capacity = array->capacity * 2;
    }
    array->array[array->size++] = element;

    return true;
}

void json_array_destroy(JsonArray *array)
{
    array->size = 0;
    free(array->array);
    array->capacity = 0;
}

//==============================JsonElement Section=================================

JsonElement json_element_create_object(JsonObject *object)
{
    JsonElement element;
    element.type = JSON_TYPE_OBJECT;
    element.value.object = *object;

    return element;
}

JsonElement json_element_create_string(String *string)
{
    JsonElement element;
    element.type = JSON_TYPE_STRING;
    element.value.string = *string;

    return element;
}
JsonElement json_element_create_array(JsonArray *array)
{
    JsonElement element;
    element.type = JSON_TYPE_ARRAY;
    element.value.array = *array;

    return element;
}
JsonElement json_element_create_boolean(bool boolean)
{
    JsonElement element;
    element.type = JSON_TYPE_BOOLEAN;
    element.value.boolean = boolean;

    return element;
}
JsonElement json_element_create_null()
{
    JsonElement element;
    element.type = JSON_TYPE_NULL;

    return element;
}

JsonElement json_element_create_number(double number)
{
    JsonElement element;
    element.type = JSON_TYPE_NUMBER;
    element.value.number = number;

    return element;
}

JsonElement json_element_create_error()
{
    JsonElement element;
    element.type = JSON_TYPE_ERROR;

    return element;
}

void json_element_display(JsonElement *element)
{
    switch (element->type)
    {
    case JSON_TYPE_STRING:
        printf("String: ");
        string_display(&element->value.string);
        break;
    case JSON_TYPE_OBJECT:
        printf("Object: ");
        for (size_t i = 0; i < element->value.object.capacity; i++)
        {
            if (element->value.object.properties[i].key.data != NULL)
            {
                string_display(&element->value.object.properties[i].key);
                json_element_display(&element->value.object.properties[i].value);
            }
        }

        break;
    case JSON_TYPE_ARRAY:
        printf("Array: ");

        for (size_t i = 0; i < element->value.array.size; i++)
        {
            json_element_display(&element->value.array.array[i]);
        }
        break;
    case JSON_TYPE_BOOLEAN:
        printf(element->value.boolean == true ? "true" : "false");
        break;
    case JSON_TYPE_NULL:
        printf(" null ");
        break;
    case JSON_TYPE_NUMBER:
        printf("%f", element->value.number);
        break;
    case JSON_TYPE_ERROR:
        printf("[ERROR Node]");
        break;
    default:
        printf("[ERROR occured]");
        break;
    }
    printf("\n");
}

#endif