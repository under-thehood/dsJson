#include "./cstring.h"

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
    JSON_TOK_NUMBER,
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

bool json_parse_value(JsonParser *parser);

JsonParser json_parser_create(String str)
{
    JsonParser parser;

    parser.string = str;

    parser.index = 0;

    return parser;
}

bool is_digit(char character)
{
    return (character >= '0' && character <= '9');
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

    parser->index++;

    for (; parser->index < parser->string.size; parser->index++)
    {
        char currData = parser->string.data[parser->index];
        if (currData == '\"')
            return stringStatus;

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
            while (currentCharacter >= '0' && currentCharacter <= '9')
            {
                currentCharacter = parser->string.data[++parser->index];
            }

            parser->tokValue = (String){.data = parser->string.data + tok_startIndex, .size = parser->index - tok_startIndex};
            parser->currentTok = JSON_TOK_NUMBER;
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
        case '-':
            parser->currentTok = JSON_TOK_MINUS;
            break;
        case '+':
            parser->currentTok = JSON_TOK_PLUS;
            break;
        case '.':
            parser->currentTok = JSON_TOK_DOT;
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

bool json_parse_object(JsonParser *parser)
{
    bool status = true;

    json_parser_next_token(parser);

    if (json_parser_expect(parser, JSON_TOK_CURLY_OPEN))
        return true;

    while (parser->currentTok != JSON_TOK_CURLY_CLOSE)
    {

        if (!json_parser_expect(parser, JSON_TOK_STRING))
        {
            printf("[ERROR] Expected the property\n");
            status = false;
        }

        json_parser_next_token(parser);
        if (!json_parser_expect(parser, JSON_TOK_COLON))
        {

            printf("[ERROR] Expected colon\n");
            return false;
        }

        json_parser_next_token(parser);

        if (!json_parse_value(parser))
        {
            return false;
        }

        if (json_parser_expect(parser, JSON_TOK_COMMA))
            json_parser_next_token(parser);

        else
            break;

        if (json_parser_expect(parser, JSON_TOK_CURLY_CLOSE))
        {
            printf("[ERROR]  Expected property or handle trailing comma\n");
            return false;
        }
    }

    if (!json_parser_expect(parser, JSON_TOK_CURLY_CLOSE))
    {
        printf("[ERROR]  Expected } or , \n");
        status = false;
    }
    return status;
}

bool json_parse_array(JsonParser *parser)
{
    bool status = true;
    bool firstProperty = true;

    do
    {
        json_parser_next_token(parser);

        if (json_parser_expect(parser, JSON_TOK_SQUARE_BOX_CLOSE))
        {
            if (!firstProperty)
            {
                printf("[ERROR] Expected the value\n");
                return false;
            }
            else
            {
                return status;
            }
        }

        if (!json_parse_value(parser))
            return false;

    } while (json_parser_expect(parser, JSON_TOK_COMMA));

    if (!json_parser_expect(parser, JSON_TOK_SQUARE_BOX_CLOSE))
    {
        printf("[ERROR] Incomplete Array\n");
        status = false;
    }
    return status;
}

bool json_parse_number(JsonParser *parser)
{
    if (json_parser_expect(parser, JSON_TOK_MINUS))
        json_parser_next_token(parser);

    if (!json_parser_expect(parser, JSON_TOK_NUMBER))
        return false;

    json_parser_next_token(parser);
    if (!json_parser_expect(parser, JSON_TOK_DOT))
        return true;

    json_parser_next_token(parser);
    if (!json_parser_expect(parser, JSON_TOK_NUMBER))
        return false;
    return true;
}

bool json_parse_value(JsonParser *parser)
{

    bool status;
    switch (parser->currentTok)
    {
    case JSON_TOK_STRING:
        status = true;
        json_parser_next_token(parser);
        break;

    case JSON_TOK_CURLY_OPEN:
        status = json_parse_object(parser);
        json_parser_next_token(parser);
        break;

    case JSON_TOK_SQUARE_BOX_OPEN:
        status = json_parse_array(parser);
        json_parser_next_token(parser);
        break;

    case JSON_TOK_FALSE:
    case JSON_TOK_TRUE:
    case JSON_TOK_NULL:
        status = true;
        json_parser_next_token(parser);
        break;
    case JSON_TOK_MINUS:
    case JSON_TOK_NUMBER:
        status = json_parse_number(parser);
        break;
    default:
        printf("[ERROR] Expected value\n");
        status = false;
        json_parser_next_token(parser);
        break;
    }
    return status;
}
