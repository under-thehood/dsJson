#ifndef _DSJSON_CSTRING_H_
#define _DSJSON_CSTRING_H_

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct
{
    char *data;
    int size;
} String;

String string_create(char *str)
{
    return (String){.data = str, .size = strlen(str)};
}

double string_to_double(String *string)
{
    char *num = (char *)malloc(string->size + 1);
    strncpy(num, string->data, string->size);

    num[string->size] = '\0';
    char *end;
    double result = strtod(num, &end);

    free(num);
    return result;
}

void string_display(String *str)
{
    // printf("=======================\n");
    for (size_t i = 0; i < str->size; i++)
    {
        putchar(str->data[i]);
    }
    // printf("\n==================================\n");
}

bool string_compare_str(String *str1, const char *str2)
{
    size_t str2Len = strlen(str2);

    if (str2Len != str1->size)
        return false;
    if (strncmp(str2, str1->data, str1->size) != 0)
        return false;
    return true;
}

bool string_compare_string(String *str1, String *str2)
{
    if (str1->size != str2->size)
        return false;

    if (strncmp(str1->data, str2->data, str1->size) != 0)
        return false;

    return true;
}

#endif