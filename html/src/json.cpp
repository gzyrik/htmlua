/* #define JSON_PARENT_LINKS */
/**
 * JSON type identifier. Basic types are:
 * 	o Object
 * 	o Array
 * 	o String
 * 	o Other primitive: number, boolean (true/false) or null
 */
enum {
    JSON_PRIMITIVE      = 0,
    JSON_OBJECT         = 1,
    JSON_ARRAY          = 2,
    JSON_STRING         = 3
};

enum {
    /* Not enough tokens were provided */
    JSON_ERROR_NOMEM    = -1,
    /* Invalid character inside JSON string */
    JSON_ERROR_INVAL    = -2,
    /* The string is not a full JSON packet, more bytes expected */
    JSON_ERROR_PART     = -3
};

/**
 * JSON token description.
 * @param		type	type (JSON_OBJECT, JSON_ARRAY, JSON_STRING etc.)
 * @param		start	start position in JSON data string
 * @param		end		end position in JSON data string
 */
typedef struct {
    int         type;
    int         start;
    int         end;
    int         size;
#ifdef JSON_PARENT_LINKS
    int         parent;
#endif
} json_t;

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string
 */
typedef struct {
    /*< private >*/
    unsigned    _[3];
} json_p;

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string
 */
typedef struct {
    /*< private >*/
    unsigned int pos; /* offset in the JSON string */
    unsigned int toknext; /* next token to allocate */
    unsigned int toksuper1; /* superior token node, e.g parent object or array, start from 1, 0 is no parent */
} json_parser;

/**
 * Allocates a fresh unused token from the token pull.
 */
static json_t *json_alloc_token(json_parser *parser,
    json_t *tokens, unsigned num_tokens)
{
    json_t *tok;
    if (parser->toknext >= num_tokens) {
        return 0;
    }
    tok = &tokens[parser->toknext++];
    tok->start = tok->end = -1;
    tok->size = 0;
#ifdef JSON_PARENT_LINKS
    tok->parent = -1;
#endif
    return tok;
}

/**
 * Fills token type and boundaries.
 */
static void json_fill_token(json_t *token, int type,
    int start, int end)
{
    token->type = type;
    token->start = start;
    token->end = end;
    token->size = 0;
}

/**
 * Fills next available token with JSON primitive.
 */
static int json_parse_primitive(json_parser *parser, const char *js,
    unsigned len, json_t *tokens, unsigned num_tokens)
{
    json_t *token;
    int start;

    start = parser->pos;

    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
        switch (js[parser->pos]) {
#ifndef JSON_STRICT
            /* In strict mode primitive must be followed by "," or "}" or "]" */
        case ':':
#endif
        case '\t' : case '\r' : case '\n' : case ' ' :
        case ','  : case ']'  : case '}' :
            goto found;
        }
        if (js[parser->pos] < 32 || js[parser->pos] >= 127) {
            parser->pos = start;
            return JSON_ERROR_INVAL;
        }
    }
#ifdef JSON_STRICT
    /* In strict mode primitive must be followed by a comma/object/array */
    parser->pos = start;
    return JSON_ERROR_PART;
#endif

found:
    if (tokens == 0) {
        parser->pos--;
        return 0;
    }
    token = json_alloc_token(parser, tokens, num_tokens);
    if (token == 0) {
        parser->pos = start;
        return JSON_ERROR_NOMEM;
    }
    json_fill_token(token, JSON_PRIMITIVE, start, parser->pos);
#ifdef JSON_PARENT_LINKS
    token->parent = parser->toksuper1-1;
#endif
    parser->pos--;
    return 0;
}

/**
 * Filsl next token with JSON string.
 */
static int json_parse_string(json_parser *parser, const char *js,
    unsigned len, json_t *tokens, unsigned num_tokens)
{
    json_t *token;

    int start = parser->pos;

    parser->pos++;

    /* Skip starting quote */
    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
        char c = js[parser->pos];

        /* Quote: end of string */
        if (c == '\"') {
            if (tokens == 0) {
                return 0;
            }
            token = json_alloc_token(parser, tokens, num_tokens);
            if (token == 0) {
                parser->pos = start;
                return JSON_ERROR_NOMEM;
            }
            json_fill_token(token, JSON_STRING, start+1, parser->pos);
#ifdef JSON_PARENT_LINKS
            token->parent = parser->toksuper1-1;
#endif
            return 0;
        }

        /* Backslash: Quoted symbol expected */
        if (c == '\\' && parser->pos + 1 < len) {
            int i;
            parser->pos++;
            switch (js[parser->pos]) {
                /* Allowed escaped symbols */
            case '\"': case '/' : case '\\' : case 'b' :
            case 'f' : case 'r' : case 'n'  : case 't' :
                break;
                /* Allows escaped symbol \uXXXX */
            case 'u':
                parser->pos++;
                for(i = 0; i < 4 && parser->pos < len && js[parser->pos] != '\0'; i++) {
                    /* If it isn't a hex character we have an error */
                    if(!((js[parser->pos] >= 48 && js[parser->pos] <= 57) || /* 0-9 */
                            (js[parser->pos] >= 65 && js[parser->pos] <= 70) || /* A-F */
                            (js[parser->pos] >= 97 && js[parser->pos] <= 102))) { /* a-f */
                        parser->pos = start;
                        return JSON_ERROR_INVAL;
                    }
                    parser->pos++;
                }
                parser->pos--;
                break;
                /* Unexpected symbol */
            default:
                parser->pos = start;
                return JSON_ERROR_INVAL;
            }
        }
    }
    parser->pos = start;
    return JSON_ERROR_PART;
}

/**
 * Parse JSON string and fill tokens.
 */
static int json_parse(json_p *p, const char *js, unsigned len,
    json_t *tokens, unsigned num_tokens) {
    int r;
    int i;
    json_t *token;
    json_parser *parser = (json_parser*)p;
    int count = parser->toknext;

    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
        char c;
        int type;

        c = js[parser->pos];
        switch (c) {
        case '{': case '[':
            count++;
            if (tokens == 0) {
                break;
            }
            token = json_alloc_token(parser, tokens, num_tokens);
            if (token == 0)
                return JSON_ERROR_NOMEM;
            if (parser->toksuper1 != 0) {
                tokens[parser->toksuper1-1].size++;
#ifdef JSON_PARENT_LINKS
                token->parent = parser->toksuper1-1;
#endif
            }
            token->type = (c == '{' ? JSON_OBJECT : JSON_ARRAY);
            token->start = parser->pos;
            parser->toksuper1 = parser->toknext;
            break;
        case '}': case ']':
            if (tokens == 0)
                break;
            type = (c == '}' ? JSON_OBJECT : JSON_ARRAY);
#ifdef JSON_PARENT_LINKS
            if (parser->toknext < 1) {
                return JSON_ERROR_INVAL;
            }
            token = &tokens[parser->toknext - 1];
            for (;;) {
                if (token->start != -1 && token->end == -1) {
                    if (token->type != type) {
                        return JSON_ERROR_INVAL;
                    }
                    token->end = parser->pos + 1;
                    parser->toksuper1 = token->parent+1;
                    break;
                }
                if (token->parent == -1) {
                    break;
                }
                token = &tokens[token->parent];
            }
#else
            for (i = parser->toknext - 1; i >= 0; i--) {
                token = &tokens[i];
                if (token->start != -1 && token->end == -1) {
                    if (token->type != type) {
                        return JSON_ERROR_INVAL;
                    }
                    parser->toksuper1 = 0;
                    token->end = parser->pos + 1;
                    break;
                }
            }
            /* Error if unmatched closing bracket */
            if (i == -1) return JSON_ERROR_INVAL;
            for (; i >= 0; i--) {
                token = &tokens[i];
                if (token->start != -1 && token->end == -1) {
                    parser->toksuper1 = i+1;
                    break;
                }
            }
#endif
            break;
        case '\"':
            r = json_parse_string(parser, js, len, tokens, num_tokens);
            if (r < 0) return r;
            count++;
            if (parser->toksuper1 != 0 && tokens != 0)
                tokens[parser->toksuper1-1].size++;
            break;
        case '\t' : case '\r' : case '\n' : case ' ':
            break;
        case ':':
            parser->toksuper1 = parser->toknext;
            break;
        case ',':
            if (tokens != 0 &&
                tokens[parser->toksuper1-1].type != JSON_ARRAY &&
                tokens[parser->toksuper1-1].type != JSON_OBJECT) {
#ifdef JSON_PARENT_LINKS
                parser->toksuper1 = tokens[parser->toksuper1-1].parent+1;
#else
                for (i = parser->toknext - 1; i >= 0; i--) {
                    if (tokens[i].type == JSON_ARRAY || tokens[i].type == JSON_OBJECT) {
                        if (tokens[i].start != -1 && tokens[i].end == -1) {
                            parser->toksuper1 = i+1;
                            break;
                        }
                    }
                }
#endif
            }
            break;
#ifdef JSON_STRICT
            /* In strict mode primitives are: numbers and booleans */
        case '-': case '0': case '1' : case '2': case '3' : case '4':
        case '5': case '6': case '7' : case '8': case '9':
        case 't': case 'f': case 'n' :
            /* And they must not be keys of the object */
            if (tokens != 0) {
                json_t *t = &tokens[parser->toksuper1-1];
                if (t->type == JSON_OBJECT ||
                    (t->type == JSON_STRING && t->size != 0)) {
                    return JSON_ERROR_INVAL;
                }
            }
#else
            /* In non-strict mode every unquoted value is a primitive */
        default:
#endif
            r = json_parse_primitive(parser, js, len, tokens, num_tokens);
            if (r < 0) return r;
            count++;
            if (parser->toksuper1 != 0 && tokens != 0)
                tokens[parser->toksuper1-1].size++;
            break;

#ifdef JSON_STRICT
            /* Unexpected char in strict mode */
        default:
            return JSON_ERROR_INVAL;
#endif
        }
    }

    for (i = parser->toknext - 1; i >= 0; i--) {
        /* Unmatched opened object or array */
        if (tokens[i].start != -1 && tokens[i].end == -1) {
            return JSON_ERROR_PART;
        }
    }
    if(tokens && count != (int)parser->toknext)
        return JSON_ERROR_INVAL;
    return count;
}

/** lua binding */
#include <lua.hpp>
#include <cstring>
#include <sstream>
static const json_t*
l_obj(lua_State *L, const char *json, const json_t *t, const json_t* const end);
static const json_t* 
l_map(lua_State *L, const char *json, const json_t *t, const json_t* const end, const int size)
{
    lua_createtable(L, 0, size);
    for(int i=0; i<size && t < end; ++i){
        lua_pushlstring(L, json+t->start, t->end-t->start);
        t = l_obj(L, json, t+1, end);
        lua_settable (L, -3);
    }
    return t;
}
static const json_t*
l_vec(lua_State *L, const char *json, const json_t *t, const json_t* const end, const int size)
{
    lua_createtable(L, size, 0);
    for(int i=0;i<size && t < end;++i) {
        lua_pushinteger(L, i+1);/* Array starting with 1 */
        t = l_obj(L, json, t, end);
        lua_settable (L, -3);
    }
    return t;
}
static const json_t*
l_obj(lua_State *L, const char *json, const json_t *t, const json_t* const end)
{
    switch(t->type) {
    case JSON_OBJECT:
        return l_map(L, json, t+1, end, t->size);
    case JSON_ARRAY:
        return l_vec(L, json, t+1, end, t->size);
    case JSON_STRING:
        lua_pushlstring(L, json+t->start, t->end-t->start);
        return t+1;
    case JSON_PRIMITIVE:{
        lua_Number num;
        std::istringstream iss(std::string(json+t->start, t->end-t->start));
        if (!(iss >> num))
            lua_pushlstring(L, json+t->start, t->end-t->start);
        else{
            const lua_Integer i = (lua_Integer)num;
            if (num - i < 0.00001f && num - i > -0.00001f)
                lua_pushinteger(L, i);
            else
                lua_pushnumber(L, num);
        }
        return t+1;
    }
    default:
        return end;
    }
}
static int
l_table(lua_State *L, const char* json, size_t len, const json_t *t, const json_t* const end)
{
    if (t->type != JSON_OBJECT && end > t && t->size <= 1) {
        const char* pColon = strchr(json, ':');
        const char* pComma = strchr(json, ',');
        if (!pColon) pColon = json+len;
        if (!pComma) pComma = json+len;
        if (pColon < pComma)
            t = l_map(L, json, t, end, end - t);
        else 
            t = l_vec(L, json, t, end, end - t);
    }
    else 
        t = l_obj(L, json, t, end);
    if (t != end)
        lua_error(L);
    return 1;
}
static int l_json(lua_State *L)
{
    size_t len;
    json_t token[1024*8];
    json_p parse={0};
    const char* json = luaL_checklstring(L, 1, &len);
    int r = json_parse(&parse, json, len, token, sizeof(token)/sizeof(json_t));
    if (r <= 0)
        return luaL_error(L,"invalid json");
    return l_table(L, json, len, token, token+r);
}
int luaopen_json_decode(lua_State *L)
{
    lua_pushcfunction(L, l_json);
    return 1;
}
