#include "src/json_parser.h"
#include <stdio.h>
#include <assert.h>

int main(int argc, char const *argv[])
{
    size_t i = 0;

    if (argc < 2)
        return 1;

    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL)
    {
        perror("fopen");
        return 1;
    }

    fseek(fp, 0, SEEK_END);

    size_t fileSize = ftell(fp);

    char *fileContent = (char *)malloc(sizeof(char) * (fileSize + 1));
    if (fileContent == NULL)
    {
        perror("malloc");
        return 1;
    }
    fseek(fp, 0, SEEK_SET);

    fread(fileContent, sizeof(*fileContent), fileSize, fp);

    fileContent[fileSize + 1] = '\0';

    JsonParser parser = json_parser_create(string_create(fileContent));

    json_parser_next_token(&parser);

    json_parse_value(&parser);

    return 0;
}
