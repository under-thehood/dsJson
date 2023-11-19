#ifndef _DSJSON_JSON_PARSER_H_
#define _DSJSON_JSON_PARSER_H_

#include "./cstring.h"
#include "./json_element.h"

typedef enum
{
    // string
    JSON_TOK_STRING,
    JSON_TOK_STRING_WITH_ERROR_ESCAPE,
    JSON_TOK_STRING_WITH_INCOMPLETE,

    // object
    JSON_TOK_CURLY_OPEN,
    JSON_TOK_CURLY_CLOSE,
    JSON_TOK_COLON,
    JSON_TOK_COMMA,

    // array
    JSON_TOK_SQUARE_BOX_OPEN,
    JSON_TOK_SQUARE_BOX_CLOSE,

    // value
    JSON_TOK_TRUE,
    JSON_TOK_FALSE,
    JSON_TOK_NULL,

    // number
    JSON_TOK_NUMBER,
    JSON_TOK_BROKEN_NUMBER,
    JSON_TOK_MINUS,
    JSON_TOK_PLUS,
    JSON_TOK_DOT,

    JSON_TOK_UNKNOWN,
    JSON_TOK_EOF,

} JSONTok;

typedef struct
{
    String string;
    size_t index;
    String tokValue;
    JSONTok currentTok;
} JsonParser;

JsonParser json_parser_create(String str)
{
    JsonParser parser;

    parser.string = str;

    parser.tokValue = (String){.data = NULL, .size = 0};

    parser.index = 0;

    return parser;
}

bool is_digit(char character)
{
    return (character >= '0' && character <= '9') || character == '-';
}

bool is_alpha(char character)
{
    return (character == 'f' || character == 't' || character == 'n');
}

#define PRESENT_STATUS 0
#define NOT_PRESENT -1
#define INTERNAL_ERROR 1

JSONTok json_parse_string(JsonParser *parser)
{
    JSONTok stringStatus = JSON_TOK_STRING;

    // parser->index++;

    size_t tok_start = parser->index;

    for (; parser->index < parser->string.size; parser->index++)
    {
        char currData = parser->string.data[parser->index];
        if (currData == '\"')
        {
            parser->tokValue = (String){.data = parser->string.data + tok_start, .size = parser->index - tok_start};
            return stringStatus;
        }

        if (currData == '\\')
        {
            switch (parser->string.data[parser->index + 1])
            {
            case '\"':
            case '\\':
            case '/':
            case 'b':
            case 'f':
            case 'n':
            case 'r':
            case 't':
                // TODO:Implement it
                //  case 'u':
                //     break;
                parser->index++;
                break;
            default:
                fprintf(stderr, "[ERROR] Invalid the escape sequence\n");
                stringStatus = JSON_TOK_STRING_WITH_ERROR_ESCAPE;
                parser->index++;
            }
        }
    }

    fprintf(stderr, "[ERROR] Incomplete string\n");
    stringStatus = JSON_TOK_STRING_WITH_INCOMPLETE;

    return stringStatus;
}

void json_parser_next_token(JsonParser *parser)
{

    for (; parser->index < parser->string.size; parser->index++)
    {
        char currentCharacter = parser->string.data[parser->index];

        // Ignore the whitespaces
        if (currentCharacter == ' ' || currentCharacter == '\t' || currentCharacter == '\n')
            continue;

        // parse identifier
        if (is_alpha(currentCharacter))
        {
            size_t tok_startIndex = parser->index;
            while (currentCharacter >= 'a' && currentCharacter <= 'z')
            {
                currentCharacter = parser->string.data[++parser->index];
            }

            parser->tokValue = (String){.data = parser->string.data + tok_startIndex, .size = parser->index - tok_startIndex};

            if (string_compare_str(&parser->tokValue, "true") == true)
            {
                parser->currentTok = JSON_TOK_TRUE;
            }
            else if (string_compare_str(&parser->tokValue, "false") == true)
            {
                parser->currentTok = JSON_TOK_FALSE;
            }
            else if (string_compare_str(&parser->tokValue, "null") == true)
            {
                parser->currentTok = JSON_TOK_NULL;
            }
            else
            {
                parser->currentTok = JSON_TOK_UNKNOWN;
            }
            return;
        }

        // parse number
        if (is_digit(currentCharacter))
        {

            size_t tok_startIndex = parser->index;
            bool isValidNumber = false;
            JSONTok tokType = JSON_TOK_NUMBER;
            currentCharacter = parser->string.data[++parser->index];

            while (currentCharacter >= '0' && currentCharacter <= '9')
            {
                isValidNumber = true;
                currentCharacter = parser->string.data[++parser->index];
            }

            if (isValidNumber)
            {
                if (currentCharacter == '.') // Parsing the fractional part
                {
                    printf("Parsing decomal\n");
                    isValidNumber = false;
                    currentCharacter = parser->string.data[++parser->index];
                    while (currentCharacter >= '0' && currentCharacter <= '9')
                    {
                        isValidNumber = true;
                        currentCharacter = parser->string.data[++parser->index];
                    }
                    if (!isValidNumber)
                        tokType = JSON_TOK_BROKEN_NUMBER;
                }
                // TODO: Handle the exponent
                //  if (currentCharacter == 'e' || currentCharacter == 'E')
                //  {

                // }
            }
            else
            {
                tokType = JSON_TOK_NUMBER;
            }

            parser->tokValue = (String){.data = parser->string.data + tok_startIndex, .size = parser->index - tok_startIndex};
            parser->currentTok = tokType;
            return;
        }

        // parse symbol
        switch (currentCharacter)
        {
        case '{':
            parser->currentTok = JSON_TOK_CURLY_OPEN;
            break;
        case '}':
            parser->currentTok = JSON_TOK_CURLY_CLOSE;
            break;
        case '[':
            parser->currentTok = JSON_TOK_SQUARE_BOX_OPEN;
            break;
        case ']':
            parser->currentTok = JSON_TOK_SQUARE_BOX_CLOSE;
            break;
        case ':':
            parser->currentTok = JSON_TOK_COLON;
            break;
        case ',':
            parser->currentTok = JSON_TOK_COMMA;
            break;
        case '\"':
            parser->index++;
            parser->currentTok = json_parse_string(parser);
            break;
        default:
            parser->currentTok = JSON_TOK_UNKNOWN;
        }
        parser->index++;
        return;
    }

    parser->currentTok = JSON_TOK_EOF;
    return;
}

bool json_parser_expect(JsonParser *parser, JSONTok tok)
{
    return parser->currentTok == tok;
}

JsonElement json_parse_value(JsonParser *parser);

JsonElement json_parse_object(JsonParser *parser)
{
    bool status = true;

    JsonObject object;
    String key;
    JsonElement value;

    json_object_init(&object, 25);

    json_parser_next_token(parser);

    if (json_parser_expect(parser, JSON_TOK_CURLY_OPEN))
        return json_element_create_object(&object);

    while (parser->currentTok != JSON_TOK_CURLY_CLOSE)
    {

        if (!json_parser_expect(parser, JSON_TOK_STRING))
        {
            printf("[ERROR] Expected the property\n");
            return json_element_create_error();
        }

        key = parser->tokValue;

        json_parser_next_token(parser);
        if (!json_parser_expect(parser, JSON_TOK_COLON))
        {

            printf("[ERROR] Expected colon\n");
            return json_element_create_error();
        }

        json_parser_next_token(parser);

        value = json_parse_value(parser);

        if (value.type == JSON_TYPE_ERROR)
        {
            return value;
        }

        json_object_add(&object, key, value);

        if (json_parser_expect(parser, JSON_TOK_COMMA))
            json_parser_next_token(parser);

        else
            break;

        if (json_parser_expect(parser, JSON_TOK_CURLY_CLOSE))
        {
            printf("[ERROR]  Expected property or handle trailing comma\n");
            return json_element_create_error();
        }
    }

    if (!json_parser_expect(parser, JSON_TOK_CURLY_CLOSE))
    {
        printf("[ERROR]  Expected } or , \n");
        return json_element_create_error();
    }

    return json_element_create_object(&object);
}

JsonElement json_parse_array(JsonParser *parser)
{
    bool firstProperty = true;
    JsonArray array;
    json_array_init(&array, 8);

    do
    {
        json_parser_next_token(parser);

        if (json_parser_expect(parser, JSON_TOK_SQUARE_BOX_CLOSE))
        {
            if (!firstProperty)
            {
                printf("[ERROR] Expected the value handle trailing comma\n");
                return json_element_create_error();
            }
            else
            {
                return json_element_create_array(&array);
            }
        }

        JsonElement value = json_parse_value(parser);
        if (value.type == JSON_TYPE_ERROR)
            return json_element_create_error();

        firstProperty = false;
        json_array_add_element(&array, value);

    } while (json_parser_expect(parser, JSON_TOK_COMMA));

    if (!json_parser_expect(parser, JSON_TOK_SQUARE_BOX_CLOSE))
    {
        printf("[ERROR] Incomplete Array\n");
        return json_element_create_error();
    }
    return json_element_create_array(&array);
}

JsonElement json_parse_number(JsonParser *parser)
{

    double num = string_to_double(&parser->tokValue);

    return json_element_create_number(num);
}

JsonElement json_parse_value(JsonParser *parser)
{

    JsonElement result;
    switch (parser->currentTok)
    {
    case JSON_TOK_STRING:
        result = json_element_create_string(&parser->tokValue);
        json_parser_next_token(parser);
        break;
    case JSON_TOK_CURLY_OPEN:
        result = json_parse_object(parser);
        json_parser_next_token(parser);
        break;

    case JSON_TOK_SQUARE_BOX_OPEN:
        result = json_parse_array(parser);
        json_parser_next_token(parser);
        break;

    case JSON_TOK_FALSE:
        result = json_element_create_boolean(false);
        json_parser_next_token(parser);
        break;
    case JSON_TOK_TRUE:
        result = json_element_create_boolean(true);
        json_parser_next_token(parser);
        break;
    case JSON_TOK_NULL:
        result = json_element_create_null();
        json_parser_next_token(parser);
        break;
    case JSON_TOK_NUMBER:
        result = json_parse_number(parser);
        json_parser_next_token(parser);
        break;
    default:
        printf("[ERROR] Expected value\n");
        result = json_element_create_error();
        json_parser_next_token(parser);
        break;
    }
    return result;
}

#endif