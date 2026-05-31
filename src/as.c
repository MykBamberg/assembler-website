/*
Copyright (C) 2025  Mykolas Bamberg

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include <emscripten.h>

constexpr size_t MAX_ERROR_LENGTH = 8192;

static char error_buf[MAX_ERROR_LENGTH + 1] = {};

#define error(template, ...) ({ \
    size_t remaining = MAX_ERROR_LENGTH - strlen(error_buf); \
    snprintf(error_buf + strlen(error_buf), remaining, "<pre class=\"error\">" template "</pre>", ##__VA_ARGS__); \
})

#define warn(template, ...) ({ \
    size_t remaining = MAX_ERROR_LENGTH - strlen(error_buf); \
    snprintf(error_buf + strlen(error_buf), remaining,"<pre class=\"warn\">" template "</pre>", ##__VA_ARGS__); \
})

#define info(template, ...) ({ \
    size_t remaining = MAX_ERROR_LENGTH - strlen(error_buf); \
    snprintf(error_buf + strlen(error_buf), remaining,"<pre class=\"info\">" template "</pre>", ##__VA_ARGS__); \
})

constexpr uint8_t ERROR_ADDRESS = 255;
constexpr uint8_t ERROR_OPCODE = 255;
constexpr size_t MAX_LABEL_NAME_LENGTH = 64;

constexpr size_t RAM_SIZE = 16;
constexpr size_t LABEL_COUNT = RAM_SIZE;

typedef struct {
    char name[MAX_LABEL_NAME_LENGTH + 1];
    uint8_t address;
} label;

static struct { regex_t label_def; regex_t num_literal; regex_t instruction; bool success; } compile_regex() {
    static typeof(compile_regex()) result = {};
    if (result.success) return result;
    result = (typeof(result)){};

    constexpr char REGEX_LABEL_DEF[] = "^[[:space:]]*([A-Za-z0-9_-]+):[[:space:]]*$";
    constexpr char REGEX_NUM_LITERAL[] = "^[[:space:]]*(0x[0-9A-Fa-f]{1,2})[[:space:]]*$";
    constexpr char REGEX_INSTRUCTION[] = "^[[:space:]]*([A-Za-z]{3})[[:space:]]*(\\$([A-Za-z0-9_-]+))?[[:space:]]*$";

    if (regcomp(&result.label_def, REGEX_LABEL_DEF, REG_EXTENDED) != 0 ||
        regcomp(&result.num_literal, REGEX_NUM_LITERAL, REG_EXTENDED) != 0 ||
        regcomp(&result.instruction, REGEX_INSTRUCTION, REG_EXTENDED) != 0) {
        error("Error: failed to compile internal regex patterns\n");
        return result;
    }

    result.success = true;
    return result;
}

static uint8_t get_label_address(size_t label_count, label labels[static label_count], const char* name) {
    for (size_t i = 0; i < label_count; i++) {
        if (strncmp(labels[i].name, name, MAX_LABEL_NAME_LENGTH) == 0) {
            return labels[i].address;
        }
    }
    return ERROR_ADDRESS;
}

static void regex_copy_match(const char* src, regmatch_t* m, char* out, size_t max_len) {
    size_t len = (size_t)(m->rm_eo - m->rm_so);
    if (len > max_len) {
        len = max_len;
    }
    memcpy(out, src + m->rm_so, len);
    out[len] = '\0';
}

static int8_t hex_value(char c) {
    switch (c) {
        case '0' ... '9':
            return (int8_t)(c - '0');
        case 'a' ... 'f':
            return (int8_t)(c - 'a' + 10);
        case 'A' ... 'F':
            return (int8_t)(c - 'A' + 10);
        default:
            return -1;
    }
}

static uint8_t parse_num(const char* hex_str) {
    const char* digits = hex_str + 2;
    int8_t a = hex_value(digits[0]);
    if (a < 0) {
        warn("Warning: Cannot parse number literal `%s', defaulting to 0\n", hex_str);
        return 0;
    }
    int8_t b = hex_value(digits[1]);
    return (b < 0) ? (uint8_t)a : (uint8_t)(16 * a + b);
}

static uint8_t opcode_for_mnemonic(const char mnemonic[static 4]) {
    if (strcmp(mnemonic, "end") == 0) {
        return 0b00000000;
    }
    if (strcmp(mnemonic, "add") == 0) {
        return 0b00010000;
    }
    if (strcmp(mnemonic, "sub") == 0) {
        return 0b00100000;
    }
    if (strcmp(mnemonic, "str") == 0) {
        return 0b00110000;
    }
    if (strcmp(mnemonic, "lod") == 0) {
        return 0b01010000;
    }
    if (strcmp(mnemonic, "bch") == 0) {
        return 0b01100000;
    }
    if (strcmp(mnemonic, "biz") == 0) {
        return 0b01110000;
    }
    if (strcmp(mnemonic, "bnn") == 0) {
        return 0b10000000;
    }
    if (strcmp(mnemonic, "inp") == 0) {
        return 0b10010001;
    }
    if (strcmp(mnemonic, "out") == 0) {
        return 0b10010010;
    }
    return ERROR_OPCODE;
}

[[gnu::used]] char* get_error_buf() {
    return error_buf;
}

[[gnu::used]] char* assemble(const char* source) {
    static char return_text[3 * RAM_SIZE + 1] = {};
    return_text[0] = '\0';
    error_buf[0] = '\0';

    auto regex = compile_regex();
    if (!regex.success) return return_text;

    uint8_t program[RAM_SIZE] = {};
    uint8_t address = 0;

    label labels[LABEL_COUNT] = {};
    size_t label_count = 0;
    char parameters[LABEL_COUNT][MAX_LABEL_NAME_LENGTH + 1] = {};

    char* source_dup = strdup(source);
    if (!source_dup) {
        error("Error: out of memory\n");
        return return_text;
    }

    char* line = source_dup;

    while (line) {
        char* next_line = strchr(line, '\n');
        if (next_line) {
            *next_line = '\0';
            next_line++;
        }

        char* comment = strchr(line, ';');
        if (comment) {
            *comment = '\0';
        }

        regmatch_t matches[4] = {};

        if (regexec(&regex.label_def, line, 3, matches, 0) == 0) {
            if (label_count >= LABEL_COUNT) {
                error("Error: out of labels\n");
                goto abort;
            }
            regex_copy_match(line, &matches[1], labels[label_count].name, MAX_LABEL_NAME_LENGTH);
            labels[label_count].address = address;
            info("Label   %2zu: `%s' (address %u)\n", label_count, labels[label_count].name, address);
            label_count++;

        } else if (regexec(&regex.num_literal, line, 3, matches, 0) == 0) {
            if (address >= RAM_SIZE) {
                error("Error: out of RAM\n");
                goto abort;
            }
            char num_str[8] = {};
            regex_copy_match(line, &matches[1], num_str, sizeof(num_str) - 1);
            program[address] = parse_num(num_str);
            info("Address %2u: number literal: %u\n", address, program[address]);
            address++;

        } else if (regexec(&regex.instruction, line, 4, matches, 0) == 0) {
            if (address >= RAM_SIZE) {
                error("Error: out of RAM\n");
                goto abort;
            }

            char mnemonic[4] = {};
            regex_copy_match(line, &matches[1], mnemonic, 3);

            uint8_t opcode = opcode_for_mnemonic(mnemonic);
            if (opcode == ERROR_OPCODE) {
                error("Error: unknown instruction `%s'\n", mnemonic);
                goto abort;
            }

            program[address] = opcode;

            if (matches[3].rm_so != -1) {
                regex_copy_match(line, &matches[3], parameters[address], MAX_LABEL_NAME_LENGTH);
            }

            if ((opcode == 0b10010001 || opcode == 0b10010010) && parameters[address][0] != '\0') {
                warn("Warning: instruction `%s' takes no operand, ignoring `$%s'\n", mnemonic, parameters[address]);
                parameters[address][0] = '\0';
            }

            info("Address %2u: command `%s'\n", address, mnemonic);
            address++;

        } else {
            const char* p = line;
            while (*p == ' ' || *p == '\t') {
                p++;
            }
            if (*p != '\0') {
                error("Error: cannot parse line: `%s'\n", line);
                goto abort;
            }
        }

        line = next_line;
    }

    for (uint8_t i = 0; i < address; i++) {
        if (parameters[i][0] == '\0') {
            continue;
        }
        uint8_t label_address = get_label_address(label_count, labels, parameters[i]);
        if (label_address == ERROR_ADDRESS) {
            error("Error: undefined label `%s'\n", parameters[i]);
            goto abort;
        }
        program[i] += label_address;
    }

    for (size_t i = 0; i < RAM_SIZE; i++) {
        sprintf(return_text + 3 * i, "%02x ", program[i]);
    }

abort:
    free(source_dup);
    return return_text;
}
