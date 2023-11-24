#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */
#include <stdio.h>  /* fprintf(), fopen(), fclose() */
#include <string.h> /* strlen(), strcmp(), strcat(), etc */

#include <parg.h>   /* parg library */

/*
    Portability notes:
      - In 'fprintf()' instead of '%zu' (for 'size_t' type) used '%lu' with
        '(unsigned long)' cast - since '%zu' was added in 'C99', but for
        portability we also supports 'C89', which dont know about '%zu'.
        - Reference: https://stackoverflow.com/a/2930710/
*/

/* -------------------------------------------------------------------------- */

int is_digit(char ch)
{
    return (ch >= '0' && ch <= '9');
}

int is_alphabet(char ch)
{
    return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'));
}

/*
    Returns 0 on success, 1 on failure.

    The rules for naming variables:
    - Variable names in C++ can range from 1 to 255 characters.
    - All variable names must begin with a letter of the alphabet or an underscore(_).
    - After the first initial letter, variable names can also contain letters and numbers.
    - Variable names are case sensitive.
    - No spaces or special characters are allowed.
    - You cannot use a C++ keyword (a reserved word) as a variable name.

    Notice 1) for simplicity, this function NOT ALLOW non-ascii characters.
    Notice 2) for simplicity, this function DONT CHECK C++ keywords.
*/
int is_valid_c_variable_name(const char* str, size_t str_len)
{
    size_t i = 0;

    /* Empty name - not allowed */
    if(str_len == 0)  return 1;

    /* Too long name - not allowed */
    if(str_len > 255) return 1;

    /* First character is digit - not allowed */
    if(is_digit(str[0])) return 1;

    /* Allow only digits, alphabet characters, underscore */
    for(; i < str_len; ++i)
    {
        const char ch = str[i];

        if (!is_digit(ch) && !is_alphabet(ch) && !(ch == '_'))
        {
            return 1;
        }
    }

    /* Everything ok */
    return 0;
}

/* -------------------------------------------------------------------------- */

/*
    Return: 0 on success, non-0 on error.
    Attention: you must free the allocated buffer manually
*/
int read_file(const char* filename, char** out_bytes, size_t* out_file_size)
{
    FILE* f_input = NULL;
    size_t file_size = 0;
    char* buffer = NULL;
    size_t num_bytes_read = 0;

    f_input = fopen(filename, "rb");
    if(f_input == NULL)
    {
        fprintf(stderr, "Error: can\'t open file %s\n", filename);
        return 1;
    }

    /* Get the file length */
    fseek(f_input, 0, SEEK_END);
    file_size = ftell(f_input);
    fseek(f_input, 0, SEEK_SET);

    if(file_size == 0)
    {
        fprintf(stderr, "Error: file %s is empty\n", filename);

        fclose(f_input);
        return 1;;
    }

    buffer = (char*) malloc(file_size);
    if(buffer == NULL)
    {
        fprintf(stderr, "Error: cannot alocate memory (%lu bytes) to store file\'s %s content\n", (unsigned long)file_size, filename);

        fclose(f_input);
        return 1;
    }

    num_bytes_read = fread(buffer, 1, file_size, f_input);
    if(num_bytes_read != file_size)
    {
        fprintf(stderr, "Error: cannot read the whole file %s. (read bytes: %lu != content bytes %lu)\n", filename, (unsigned long)num_bytes_read, (unsigned long)file_size);

        fclose(f_input);
        free(buffer);
        return 1;
    }

    /* Successful read */
    fclose(f_input);
    *out_bytes     = buffer;
    *out_file_size = file_size;
    return 0;
}

/* -------------------------------------------------------------------------- */

void write_bytes(FILE* file, const char* bytes, size_t bytes_count)
{
    int need_comma = 0;

    size_t i = 0;
    for(; i < bytes_count; ++i)
    {
        if(need_comma) {
            fprintf(file, ", ");
        } else {
            need_comma = 1;
        }

        if( (i % 11) == 0 ) {
            fprintf(file, "\n\t");
        }

        fprintf(file, "0x%.2x", bytes[i] & 0xff);
    }
}

/* -------------------------------------------------------------------------- */

/* Attention: You must free allocated memory manually! */
char* str_concat(const char* str1, const char* str2)
{
    /* via: https://stackoverflow.com/a/5901241/ */

    const size_t str1_len = strlen(str1);
    const size_t str2_len = strlen(str2);

    char* new_str = malloc(str1_len + str2_len + 1);
    if(new_str != NULL)
    {
        new_str[0] = '\0'; /* Ensures the memory is an empty string */
        strcat(new_str, str1);
        strcat(new_str, str2);

        return new_str;
    }
    else
    {
        fprintf(stderr, "Error: cannot allocate memory for strings concatenation: %s and %s\n", str1, str2);
        return NULL;
    }
}

/* -------------------------------------------------------------------------- */

int write_C_header_single(
        const char* file_name, const char* var_name,
        const char* bytes, size_t bytes_count)
{
    char* header_file_name = NULL;
    FILE* header_file = NULL;

    header_file_name = str_concat(file_name, ".h");
    if(header_file_name == NULL)
    {
        return 1;
    }

    header_file = fopen(header_file_name, "w");
    if(header_file == NULL)
    {
        fprintf(stderr, "Error: can\'t open the file %s", header_file_name);

        free(header_file_name);
        return 1;
    }

    fprintf(header_file,
            "#pragma once\n"
            "\n"
            "#include <stddef.h> /* for size_t */\n"
            "\n"
            "static const unsigned char %s_bytes[%lu] = {",
            var_name, (unsigned long)bytes_count);

    write_bytes(header_file, bytes, bytes_count);

    fprintf(header_file,
            "\n"
            "};\n"
            "\n"
            "static const size_t %s_size = %lu;\n",
            var_name, (unsigned long)bytes_count
    );

    fclose(header_file);
    free(header_file_name);

    return 0;
}

int write_C_header_source_extern(
        const char* file_name, const char* var_name,
        const char* bytes, size_t bytes_count)
{
    char* header_file_name = NULL;
    char* source_file_name = NULL;

    FILE* header_file = NULL;
    FILE* source_file = NULL;

    /* ---------------------------------------------------------------------- */

    header_file_name = str_concat(file_name, ".h");
    if(header_file_name == NULL)
    {
        return 1;
    }

    source_file_name = str_concat(file_name, ".c");
    if(source_file_name == NULL)
    {
        free(header_file_name);
        return 1;
    }

    /* ---------------------------------------------------------------------- */

    {
        header_file = fopen(header_file_name, "w");
        if(header_file == NULL)
        {
            fprintf(stderr, "Error: can\'t open the file %s", header_file_name);

            free(header_file_name);
            free(source_file_name);
            return 1;
        }

        fprintf(header_file,
                "#pragma once\n"
                "\n"
                "#include <stddef.h> /* for size_t */\n"
                "\n"
                "#ifdef __cplusplus\n"
                "extern \"C\" {\n"
                "#endif\n"
                "\n");

        fprintf(header_file,
                "extern const unsigned char* %s_bytes;\n"
                "extern size_t               %s_size;\n",
                var_name, var_name);

        fprintf(header_file,
                "\n"
                "#ifdef __cplusplus\n"
                "} /* extern \"C\" */\n"
                "#endif\n");

        fclose(header_file);
    }

    /* ---------------------------------------------------------------------- */

    {
        source_file = fopen(source_file_name, "w");
        if(source_file == NULL)
        {
            fprintf(stderr, "Error: can\'t open the file %s", source_file_name);

            free(header_file_name);
            free(source_file_name);
            return 1;
        }

        fprintf(source_file, "#include \"%s\"\n", header_file_name);
        fprintf(source_file, "\n");

        fprintf(source_file, "static const unsigned char %s_bytes[%lu] = {", var_name, (unsigned long)bytes_count);
        write_bytes(source_file, bytes, bytes_count);
        fprintf(source_file,
                "\n"
                "};\n"
                "\n");

        fprintf(source_file,
                "static const size_t %s_size = %lu;\n",
                var_name, (unsigned long)bytes_count);

        fclose(source_file);
    }

    /* ---------------------------------------------------------------------- */

    free(header_file_name);
    free(source_file_name);

    return 0;
}

int write_C_header_source_funcs(
        const char* file_name, const char* var_name,
        const char* bytes, size_t bytes_count)
{
    char* header_file_name = NULL;
    char* source_file_name = NULL;

    FILE* header_file = NULL;
    FILE* source_file = NULL;

    /* ---------------------------------------------------------------------- */

    header_file_name = str_concat(file_name, ".h");
    if(header_file_name == NULL)
    {
        return 1;
    }

    source_file_name = str_concat(file_name, ".c");
    if(source_file_name == NULL)
    {
        free(header_file_name);
        return 1;
    }

    /* ---------------------------------------------------------------------- */

    {
        header_file = fopen(header_file_name, "w");
        if(header_file == NULL)
        {
            fprintf(stderr, "Error: can\'t open the file %s", header_file_name);

            free(header_file_name);
            free(source_file_name);
            return 1;
        }

        fprintf(header_file,
                "#pragma once\n"
                "\n"
                "#include <stddef.h> /* for size_t */\n"
                "\n"
                "#ifdef __cplusplus\n"
                "extern \"C\" {\n"
                "#endif\n"
                "\n");

        fprintf(header_file,
                "const unsigned char* get_%s_bytes();\n"
                "size_t               get_%s_size();\n",
                var_name, var_name);

        fprintf(header_file,
                "\n"
                "#ifdef __cplusplus\n"
                "} /* extern \"C\" */\n"
                "#endif\n");

        fclose(header_file);
    }

    /* ---------------------------------------------------------------------- */

    {
        source_file = fopen(source_file_name, "w");
        if(source_file == NULL)
        {
            fprintf(stderr, "Error: can\'t open the file %s", source_file_name);

            free(header_file_name);
            free(source_file_name);
            return 1;
        }

        fprintf(source_file,
                "#include \"%s\"\n"
                "\n", header_file_name);

        fprintf(source_file, "static const unsigned char %s_bytes[%lu] = {", var_name, (unsigned long)bytes_count);
        write_bytes(source_file, bytes, bytes_count);
        fprintf(source_file,
                "\n"
                "};\n"
                "\n");

        fprintf(source_file,
                "static const size_t %s_size = %lu;\n",
                var_name, (unsigned long)bytes_count);

        fprintf(source_file,
                "\n"
                "/* ------------------------------------------------------ */\n"
                "\n"
                "const unsigned char* get_%s_bytes() { return %s_bytes; }\n",
                var_name, var_name);
        fprintf(source_file,
                "size_t               get_%s_size()  { return %s_size; }\n",
                var_name, var_name);

        fclose(source_file);
    }

    /* ---------------------------------------------------------------------- */

    free(header_file_name);
    free(source_file_name);

    return 0;
}

int write_C_header_source_struct_extern(
        const char* file_name, const char* var_name,
        const char* bytes, size_t bytes_count)
{
    char* header_file_name = NULL;
    char* source_file_name = NULL;

    FILE* header_file = NULL;
    FILE* source_file = NULL;

    /* ---------------------------------------------------------------------- */

    header_file_name = str_concat(file_name, ".h");
    if(header_file_name == NULL)
    {
        return 1;
    }

    source_file_name = str_concat(file_name, ".c");
    if(source_file_name == NULL)
    {
        free(header_file_name);
        return 1;
    }

    /* ---------------------------------------------------------------------- */

    {
        header_file = fopen(header_file_name, "w");
        if(header_file == NULL)
        {
            fprintf(stderr, "Error: can\'t open the file %s", header_file_name);

            free(header_file_name);
            free(source_file_name);
            return 1;
        }

        fprintf(header_file,
                "#pragma once\n"
                "\n"
                "#include <stddef.h> /* for size_t */\n"
                "\n"
                "#ifdef __cplusplus\n"
                "extern \"C\" {\n"
                "#endif\n"
                "\n");

        fprintf(header_file,
                "typedef struct %s_data\n"
                "{\n"
                "    const unsigned char* bytes;\n"
                "    size_t               size;\n"
                "} %s_data;\n"
                "\n"
                "extern const %s_data %s;\n",
                var_name, var_name, var_name, var_name);

        fprintf(header_file,
                "\n"
                "#ifdef __cplusplus\n"
                "} /* extern \"C\" */\n"
                "#endif\n");

        fclose(header_file);
    }

    /* ---------------------------------------------------------------------- */

    {
        source_file = fopen(source_file_name, "w");
        if(source_file == NULL)
        {
            fprintf(stderr, "Error: can\'t open the file %s", source_file_name);

            free(header_file_name);
            free(source_file_name);
            return 1;
        }

        fprintf(source_file,
                "#include \"%s\"\n"
                "\n", header_file_name);

        fprintf(source_file, "static const unsigned char %s_bytes[%lu] = {", var_name, (unsigned long)bytes_count);
        write_bytes(source_file, bytes, bytes_count);
        fprintf(source_file,
                "\n"
                "};\n");

        fprintf(source_file,
                "\n"
                "/* ------------------------------------------------------ */\n"
                "\n"
                "static const %s_data %s = {%s_bytes, %lu};\n",
                var_name, var_name, var_name, (unsigned long)bytes_count);

        fclose(source_file);
    }

    /* ---------------------------------------------------------------------- */

    free(header_file_name);
    free(source_file_name);

    return 0;
}

int write_C_header_source_struct_func(
        const char* file_name, const char* var_name,
        const char* bytes, size_t bytes_count)
{
    char* header_file_name = NULL;
    char* source_file_name = NULL;

    FILE* header_file = NULL;
    FILE* source_file = NULL;

    /* ---------------------------------------------------------------------- */

    header_file_name = str_concat(file_name, ".h");
    if(header_file_name == NULL)
    {
        return 1;
    }

    source_file_name = str_concat(file_name, ".c");
    if(source_file_name == NULL)
    {
        free(header_file_name);
        return 1;
    }

    /* ---------------------------------------------------------------------- */

    {
        header_file = fopen(header_file_name, "w");
        if(header_file == NULL)
        {
            fprintf(stderr, "Error: can\'t open the file %s", header_file_name);

            free(header_file_name);
            free(source_file_name);
            return 1;
        }

        fprintf(header_file,
                "#pragma once\n"
                "\n"
                "#include <stddef.h> /* for size_t */\n"
                "\n"
                "#ifdef __cplusplus\n"
                "extern \"C\" {\n"
                "#endif\n"
                "\n");

        fprintf(header_file,
                "typedef struct %s_data\n"
                "{\n"
                "    const unsigned char* bytes;\n"
                "    size_t               size;\n"
                "} %s_data;\n"
                "\n"
                "const %s_data* get_%s_data();\n",
                var_name, var_name, var_name, var_name);

        fprintf(header_file,
                "\n"
                "#ifdef __cplusplus\n"
                "} /* extern \"C\" */\n"
                "#endif\n");

        fclose(header_file);
    }

    /* ---------------------------------------------------------------------- */

    {
        source_file = fopen(source_file_name, "w");
        if(source_file == NULL)
        {
            fprintf(stderr, "Error: can\'t open the file %s", source_file_name);

            free(header_file_name);
            free(source_file_name);
            return 1;
        }

        fprintf(source_file,
                "#include \"%s\"\n"
                "\n", header_file_name);

        fprintf(source_file, "static const unsigned char %s_bytes[%lu] = {", var_name, (unsigned long)bytes_count);
        write_bytes(source_file, bytes, bytes_count);
        fprintf(source_file,
                "\n"
                "};\n");

        fprintf(source_file,
                "\n"
                "/* ------------------------------------------------------ */\n"
                "\n"
                "static const %s_data %s_data_struct = {%s_bytes, %lu};\n"
                "\n",
                var_name, var_name, var_name, (unsigned long) bytes_count);

        fprintf(source_file,
                "const %s_data* get_%s_data() { return &%s_data_struct; }\n",
                var_name, var_name, var_name);

        fclose(source_file);
    }

    /* ---------------------------------------------------------------------- */

    free(header_file_name);
    free(source_file_name);

    return 0;
}

/* -------------------------------------------------------------------------- */

typedef enum {
      MODE_C_HEADER_SINGLE = 0
    , MODE_C_HEADER_SOURCE_EXTERN
    , MODE_C_HEADER_SOURCE_FUNCS
    , MODE_C_HEADER_SOURCE_STRUCT_EXTERN
    , MODE_C_HEADER_SOURCE_STRUCT_FUNC
} Mode;

typedef struct {
    Mode   mode;
    const char* mode_name;
} ModeInfo;

#define MODES_COUNT 5

static const ModeInfo MODES[MODES_COUNT] =
{
      { MODE_C_HEADER_SINGLE,        "c_header" }

    , { MODE_C_HEADER_SOURCE_EXTERN, "c_extern" }
    , { MODE_C_HEADER_SOURCE_FUNCS,  "c_funcs"  }

    , { MODE_C_HEADER_SOURCE_STRUCT_EXTERN, "c_struct_extern" }
    , { MODE_C_HEADER_SOURCE_STRUCT_FUNC,   "c_struct_func"   }
};

/* Returns -1 in case of missmatch */
Mode get_mode_from_str(const char* str)
{
    size_t i = 0;
    for(; i < MODES_COUNT; ++i)
    {
        if( strcmp(str, MODES[i].mode_name) == 0 ) /* str is equal to mode[i].name */
        {
            return MODES[i].mode;
        }
    }

    /* Undefined mode */
    return -1;
}

/* Returns 0 on success, otherwize non-0 */
int is_valid_mode(const Mode mode)
{
    size_t i = 0;
    for(; i < MODES_COUNT; ++i)
    {
        if( mode == MODES[i].mode )
        {
            return 0; /* Mode found */
        }
    }

    /* Mode not found */
    return 1;
}

void print_modes(FILE* output)
{
    size_t i = 0;
    for(; i < MODES_COUNT; ++i)
    {
        fprintf(output, "\t%s\n", MODES[i].mode_name);
    }
}

/* -------------------------------------------------------------------------- */

static const char APP_VERSION[] = "1.0.2";

int main(int argc, char* argv[])
{
    const char* app_name = (argc > 0) ? argv[0] : NULL;

    char* input_file_name  = NULL;
    char* output_file_name = NULL;
    char* var_name         = NULL;

    Mode mode = MODE_C_HEADER_SINGLE;

    /* --------------------------- */

    char*  input_file_buffer = NULL;
    size_t input_file_size   = 0;

    /* ---------------------------------------------------------------------- */
    /* Arguments parsing */
    {
        int opt = -1;
        const char* OPT_STRING = "hvi:o:n:m:";

        struct parg_state ps;
        parg_init(&ps);

        while ((opt = parg_getopt(&ps, argc, argv, OPT_STRING)) != -1)
        {
            switch (opt) {

            case 'h': { /* Help */
                fprintf(stdout, "Usage: %s -i INPUT_FILE_NAME -o OUTPUT_FILE_NAME -n VARIABLE_NAME [-m MODE]\n", app_name);
                return EXIT_SUCCESS;
            } break;

            case 'v': { /* Version */
                fprintf(stdout, "%s version: %s\n", app_name, APP_VERSION);
                fprintf(stdout, "  <parg> version: %s\n", PARG_VER_STRING);
                return EXIT_SUCCESS;
            } break;

            /* -------------------------------------------------------------- */

            case 'i': { /* [I]nput file name */
                const size_t arg_str_len = strlen(ps.optarg);
                input_file_name = (char *) malloc(arg_str_len + 1);
                strcpy(input_file_name, ps.optarg);
            } break;

            case 'o': { /* [O]utput file name */
                const size_t arg_str_len = strlen(ps.optarg);
                output_file_name = (char *) malloc(arg_str_len + 1);
                strcpy(output_file_name, ps.optarg);
            } break;

            case 'n': { /* [N]ame of the variable */
                const size_t arg_str_len = strlen(ps.optarg);
                var_name = (char *) malloc(arg_str_len + 1);
                strcpy(var_name, ps.optarg);
            } break;

            case 'm': { /* [M]ode (or output file format/style) */
                mode = get_mode_from_str(ps.optarg);
                if(mode == (Mode)-1)
                {
                    fprintf(stderr, "Error: undefined mode: %s\n", ps.optarg);

                    fprintf(stderr, "The list of known modes is:\n");
                    print_modes(stderr);

                    return EXIT_FAILURE;
                }
            } break;

            /* -------------------------------------------------------------- */

            case 1: {
                fprintf(stderr, "Error: non-option arg: %s\n", ps.optarg);
                return EXIT_FAILURE;
            } break;

            case '?': {
                fprintf(stderr, "Error: unknown option -%c\n", ps.optopt);
                return EXIT_FAILURE;
            } break;

            case ':': {
                fprintf(stderr, "Error: option needs a value: -%c\n", ps.optopt);
                return EXIT_FAILURE;
            } break;

            default: {
                fprintf(stderr, "Error: unhandled option -%c\n", opt);
                return EXIT_FAILURE;
            } break;
            }
        }
    }

    /* ---------------------------------------------------------------------- */

    /* Arguments validation */
    {
        if( (input_file_name == NULL) || (strlen(input_file_name) == 0) )
        {
            fprintf(stderr, "Error: input file name is empty\n");
            return EXIT_FAILURE;
        }

        if( (output_file_name == NULL) || (strlen(output_file_name) == 0) )
        {
            fprintf(stderr, "Error: output file name is empty\n");
            return EXIT_FAILURE;
        }

        if( (var_name == NULL) || (is_valid_c_variable_name(var_name, strlen(var_name)) != 0) )
        {
            fprintf(stderr, "Error: invalid var name %s\n", var_name);
            return EXIT_FAILURE;
        }

        if( is_valid_mode(mode) != 0)
        {
            fprintf(stderr, "Error: invalid mode %i\n", mode);
            return EXIT_FAILURE;
        }
    }

    /* ---------------------------------------------------------------------- */

    if(read_file(input_file_name, &input_file_buffer, &input_file_size) == 0)
    {
        int result = -1;

        switch (mode) {
        case MODE_C_HEADER_SINGLE: {
            result = write_C_header_single(output_file_name, var_name, input_file_buffer, input_file_size);
        } break;

        case MODE_C_HEADER_SOURCE_EXTERN: {
            result = write_C_header_source_extern(output_file_name, var_name, input_file_buffer, input_file_size);
        } break;

        case MODE_C_HEADER_SOURCE_FUNCS: {
            result = write_C_header_source_funcs(output_file_name, var_name, input_file_buffer, input_file_size);
        } break;

        case MODE_C_HEADER_SOURCE_STRUCT_EXTERN: {
            result = write_C_header_source_struct_extern(output_file_name, var_name, input_file_buffer, input_file_size);
        } break;

        case MODE_C_HEADER_SOURCE_STRUCT_FUNC: {
            result = write_C_header_source_struct_func(output_file_name, var_name, input_file_buffer, input_file_size);
        } break;

        default: { /* Unreachable: mode validated previously */ } break;
        }

        free(input_file_buffer);

        /* ------------------------------------------------------------------ */

        if(result != 0)
        {
            fprintf(stderr, "Error during writing output into file\n");
            return EXIT_FAILURE;
        }
    }

    free(input_file_name);
    free(output_file_name);

    return EXIT_SUCCESS;
}
