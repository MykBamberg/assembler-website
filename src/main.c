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

constexpr uint8_t ERROR_ADDRESS = 255;
constexpr size_t MAX_LABEL_NAME_LENGTH = 64;

constexpr size_t RAM_SIZE = 16;
constexpr size_t LABEL_COUNT = RAM_SIZE;

typedef struct {
    char name[MAX_LABEL_NAME_LENGTH + 1];
    uint8_t address;
} label;

uint8_t get_label_address(size_t label_count, label labels[static label_count], const char* name) {
    for (size_t i = 0; i < label_count; i++) {
        if (strncmp(labels[i].name, name, MAX_LABEL_NAME_LENGTH) == 0) {
            return labels[i].address;
        }
    }

    if (*name != '\0') {
        fprintf(stderr, "Cannot find label, defaulting to address `0'\n");
    }
    return ERROR_ADDRESS;
}

void get_label(char* str, char label[static MAX_LABEL_NAME_LENGTH]) {
    /* Remove leading white-space */
    while (*str == ' ') str++;
    *strchr(str, ':') = '\0';
    /* Truncate string to first space */
    *strchrnul(str, ' ') = '\0';
    strncpy(label, str, MAX_LABEL_NAME_LENGTH);
}

bool string_empty(const char* str) {
    while (*str != '\0' && *str != '\n') {
        if (*str != ' ') {
            return false;
        }
        str++;
    }
    return true;
}

bool is_num(const char* str) {
    while (*str != '\0' && *str != '\n') {
        if (*str != ' ') {
            if (*str == '0' && *(str + 1) == 'x') {
                return true;
            }
            break;
        }
        str++;
    }
    return false;
}

int8_t hex_value(char c) {
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

uint8_t parse_num(const char* str) {
    const char* num_start = strchr(str, 'x') + 1;
    int8_t a = hex_value(*num_start);

    if (a < 0) {
        fprintf(stderr, "Cannot parse number literal, defaulting to 0\n");
        return 0;
    }

    int8_t b = hex_value(*(num_start + 1));
    if (b < 0) {
        return (uint8_t)a;
    }

    return (uint8_t)(16 * a + b);
}

uint8_t get_cmd(char* str, char label[static MAX_LABEL_NAME_LENGTH]) {
    /* Remove leading white-space */
    while (*str == ' ') str++;
    /* Remove newline */
    *strchrnul(str, '\n') = '\0';
    if (strlen(str) < 3) {
        goto parse_error;
    }

    if (strchr(str, '$') != NULL) {
        /* Truncate string to first space */
        *strchrnul(strchr(str, '$'), ' ') = '\0';
        strncpy(label, strchr(str, '$') + 1, MAX_LABEL_NAME_LENGTH);
    }

    if (strncmp(str, "end", 3) == 0) {
        label[0] = '\0';
        return 0b00000000;
    }
    if (strncmp(str, "add", 3) == 0) {
        return 0b00010000;
    }
    if (strncmp(str, "sub", 3) == 0) {
        return 0b00100000;
    }
    if (strncmp(str, "str", 3) == 0) {
        return 0b00110000;
    }
    if (strncmp(str, "lod", 3) == 0) {
        return 0b01010000;
    }
    if (strncmp(str, "bch", 3) == 0) {
        return 0b01100000;
    }
    if (strncmp(str, "biz", 3) == 0) {
        return 0b01110000;
    }
    if (strncmp(str, "bnn", 3) == 0) {
        return 0b10000000;
    }
    if (strncmp(str, "inp", 3) == 0) {
        label[0] = '\0';
        return 0b10010001;
    }
    if (strncmp(str, "out", 3) == 0) {
        label[0] = '\0';
        return 0b10010010;
    }

parse_error:
    fprintf(stderr, "Cannot decode instruction, defaulting to `end'\n");
    label[0] = '\0';
    return 0b00000000;
}

int main(int argc, char* argv[argc]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [FILE]\n", argv[0]);
        return 1;
    }

    const char* source_path = argv[1];
    if (strcmp(source_path, "-") == 0) {
        source_path = "/dev/stdin";
    }

    FILE* source_file = fopen(source_path, "r");
    if (source_file == NULL) {
        fprintf(stderr, "Cannot open file '%s'\n", source_path);
        return 1;
    }

    uint8_t program[RAM_SIZE] = {};
    uint8_t address = 0;

    label labels[LABEL_COUNT] = {};
    size_t label_count = 0;
    char parameters[LABEL_COUNT][MAX_LABEL_NAME_LENGTH + 1] = {};

    char* line = NULL;
    size_t line_length = 0;

    while (getline(&line, &line_length, source_file) != -1) {
        /* Remove comments */
        *strchrnul(line, ';') = '\0';
        /* Skip empty lines */
        if (string_empty(line)) {
            continue;
        }
        /* Add labels to label array */
        else if (strchr(line, ':') != NULL) {
            if (label_count >= LABEL_COUNT) {
                fprintf(stderr, "Error: out of labels");
                goto abort;
            }

            get_label(line, labels[label_count].name);
            labels[label_count].address = address;

            fprintf(stderr, "label %zu: %s -> %u\n", label_count, labels[label_count].name, labels[label_count].address);
            label_count++;
        }
        /* Add number literals */
        else if (is_num(line)) {
            program[address] = parse_num(line);
            fprintf(stderr, "address %u - number literal: %u\n", address, program[address]);
            address++;
        }
        /* Parse commands */
        else {
            program[address] = get_cmd(line, parameters[address]);
            fprintf(stderr, "address %u - command\n", address);
            address++;
        }

        if (address > RAM_SIZE) {
            fprintf(stderr, "Error: out of RAM");
            goto abort;
        }
    }

    /* Add addresses in place of labels */
    for (uint8_t i = 0; i < address; i++) {
        uint8_t label_address = get_label_address(label_count, labels, parameters[i]);
        if (label_address != ERROR_ADDRESS) {
            program[i] += label_address;
        }
    }

    /* Print RAM content */
    for (size_t i = 0; i < RAM_SIZE; i++) {
        printf("%02x ", program[i]);
    }
    printf("\n");

abort:
    free(line);
    fclose(source_file);

    return 0;
}
